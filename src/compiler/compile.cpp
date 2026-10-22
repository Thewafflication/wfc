#include "compile.hpp"

#include <algorithm>
#include <cctype>

#include "../interpreter/builtin_class_sources.hpp"

#include "lower.hpp"
#include "parser.hpp"

namespace wfc::compiler {

BackEndResult generate_code(const ir::Program& program,
                            const Architecture architecture) {
    if (architecture == Architecture::x64) {
        return generate_x64(program);
    }
    if (architecture == Architecture::x86) {
        return generate_x86(program);
    }
    return generate_arm64(program);
}

bool native_back_end_available() {
    return true;
}

CompileResult compile_source(const std::string_view source,
                             const Architecture architecture) {
    return compile_project(source, {}, architecture);
}

namespace {

[[nodiscard]] std::string lower_case(std::string text) {
    for (auto& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

}  // namespace

namespace {

[[nodiscard]] bool identifier_part(const char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

// Whether the lower-case text has `word.` with nothing before the word that
// belongs to a name.
[[nodiscard]] bool mentions_object(const std::string& text,
                                   const std::string_view word) {
    for (std::size_t at = text.find(word); at != std::string::npos;
         at = text.find(word, at + 1U)) {
        if ((at == 0 || !identifier_part(text[at - 1U])) &&
            at + word.size() < text.size() && text[at + word.size()] == '.') {
            return true;
        }
    }
    return false;
}

// Whether the lower-case text has the word on its own.
[[nodiscard]] bool mentions_word(const std::string& text,
                                 const std::string_view word) {
    for (std::size_t at = text.find(word); at != std::string::npos;
         at = text.find(word, at + 1U)) {
        if ((at == 0 || !identifier_part(text[at - 1U])) &&
            (at + word.size() == text.size() ||
             !identifier_part(text[at + word.size()]))) {
            return true;
        }
    }
    return false;
}

// The source of the `App` class with the project's own values.
[[nodiscard]] std::string app_source_for(
    const std::map<std::string, std::string>& properties) {
    std::string source(wfc::detail::kAppSource);
    for (const auto& [property, value] : properties) {
        if (property == "path" && value.empty()) {
            continue;
        }
        const bool numeric = property == "major" || property == "minor" ||
                             property == "revision";
        std::string literal;
        if (numeric) {
            literal = std::to_string(std::atol(value.c_str()));
        } else {
            literal = "\"";
            for (const char c : value) {
                literal += c == '"' ? std::string("\"\"") : std::string(1U, c);
            }
            literal += "\"";
        }
        const std::string marker =
            property == "title"             ? "Title = \"\""
            : property == "exename"         ? "EXEName = \"Project1\""
            : property == "major"           ? "Major = 1"
            : property == "minor"           ? "Minor = 0"
            : property == "revision"        ? "Revision = 0"
            : property == "companyname"     ? "CompanyName = \"\""
            : property == "productname"     ? "ProductName = \"\""
            : property == "filedescription" ? "FileDescription = \"\""
            : property == "comments"        ? "Comments = \"\""
            : property == "path"            ? "Path = CurDir$"
            : property == "legaltrademarks" ? "LegalTrademarks = \"\""
                                            : "LegalCopyright = \"\"";
        const auto at = source.find(marker);
        if (at != std::string::npos) {
            source.replace(at, marker.size(),
                           marker.substr(0, marker.find('=') + 2U) + literal);
        }
    }
    return source;
}

}  // namespace

CompileResult compile_project(const std::string_view source,
                              const std::vector<ClassSource>& classes,
                              const Architecture architecture,
                              const ProjectInfo& info) {
    CompileResult result;
    // Built-in classes are compiled from their VB source when the program uses
    // them; Screen, App and Clipboard are single instances that exist by name.
    std::vector<ClassSource> all = classes;
    struct Singleton final {
        std::string global;
        std::string class_name;
    };
    std::vector<Singleton> singletons;
    {
        std::string text = lower_case(std::string(source));
        for (const auto& item : classes) {
            text += lower_case(item.source);
        }
        const auto defined = [&](const std::string& name) {
            return std::any_of(classes.begin(), classes.end(),
                               [&](const ClassSource& item) {
                                   return lower_case(item.name) == name;
                               });
        };
        if (!defined("collection") && mentions_word(text, "collection")) {
            all.push_back(ClassSource{
                "Collection", std::string(wfc::detail::kCollectionSource)});
        }
        if (!defined("wfcscreen") && mentions_object(text, "screen")) {
            all.push_back(ClassSource{"WfcScreen",
                                      std::string(wfc::detail::kScreenSource)});
            singletons.push_back({"screen", "wfcscreen"});
        }
        if (!defined("wfcapp") && mentions_object(text, "app")) {
            all.push_back(
                ClassSource{"WfcApp", app_source_for(info.app_properties)});
            singletons.push_back({"app", "wfcapp"});
        }
        if (!defined("wfcclipboard") && mentions_object(text, "clipboard")) {
            all.push_back(ClassSource{
                "WfcClipboard", std::string(wfc::detail::kClipboardSource)});
            singletons.push_back({"clipboard", "wfcclipboard"});
        }
    }
    ParseOptions options;
    for (const auto& item : all) {
        options.class_names.insert(lower_case(item.name));
    }
    std::vector<Module> class_modules;
    class_modules.reserve(all.size());
    for (const auto& item : all) {
        ParseOptions class_options = options;
        class_options.is_class = true;
        class_options.name = lower_case(item.name);
        class_options.display_name = item.name;
        auto parsed_class = parse_module(item.source, class_options);
        if (!parsed_class.error.empty()) {
            result.error = "class " + item.name + ": " + parsed_class.error;
            result.error_at = parsed_class.error_at;
            return result;
        }
        class_modules.push_back(std::move(parsed_class.module));
    }
    auto parsed = parse_module(source, options);
    if (!parsed.error.empty()) {
        result.error = parsed.error;
        result.error_at = parsed.error_at;
        return result;
    }
    for (const auto& item : singletons) {
        Declaration declaration;
        declaration.name = item.global;
        declaration.type = VbType::object;
        declaration.type_name = item.class_name;
        declaration.type_given = true;
        declaration.is_new = true;
        declaration.is_public = true;
        declaration.at = Position{1, 1};
        parsed.module.globals.push_back(std::move(declaration));
    }
    std::vector<const Module*> class_pointers;
    for (const auto& module : class_modules) {
        class_pointers.push_back(&module);
    }
    auto lowered = lower_module(parsed.module, class_pointers);
    if (!lowered.error.empty()) {
        result.error = lowered.error;
        result.error_at = lowered.error_at;
        return result;
    }
    auto generated = generate_code(lowered.program, architecture);
    if (!generated.error.empty()) {
        result.error = generated.error;
        return result;
    }
    result.image = std::move(generated.image);
    return result;
}

}  // namespace wfc::compiler
