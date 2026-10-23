#include "compile.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>

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

// The names of the Enums a source declares (`[Public|Private] Enum Name`).
void collect_enum_names(const std::string& source,
                        std::set<std::string>& into) {
    std::size_t position = 0;
    while (position < source.size()) {
        auto end = source.find('\n', position);
        if (end == std::string::npos) {
            end = source.size();
        }
        std::istringstream words(
            lower_case(source.substr(position, end - position)));
        std::string word;
        words >> word;
        if (word == "public" || word == "private" || word == "global") {
            words >> word;
        }
        if (word == "enum") {
            std::string name;
            words >> name;
            if (!name.empty() && name.front() != '\'') {
                into.insert(name);
            }
        }
        position = end + 1U;
    }
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

// The Visual Basic name of a type, for a parameter of a thunk.
[[nodiscard]] bool thunk_type(const Parameter& parameter, std::string& text,
                              bool& is_object) {
    is_object = false;
    switch (parameter.type) {
        case VbType::boolean:
            text = "Boolean";
            return true;
        case VbType::byte:
            text = "Byte";
            return true;
        case VbType::integer:
            text = "Integer";
            return true;
        case VbType::long_integer:
            text = "Long";
            return true;
        case VbType::single_precision:
            text = "Single";
            return true;
        case VbType::double_precision:
            text = "Double";
            return true;
        case VbType::string:
            text = "String";
            return true;
        case VbType::none:
        case VbType::variant:
            text = "Variant";
            return true;
        case VbType::object:
            text = parameter.type_name;
            is_object = true;
            return !text.empty();
        case VbType::record:
            return false;
    }
    return false;
}

// Visual Basic source for the thunk of each member of a class: a function the
// shared runtime calls with the arguments of a late-bound call (WfcArg), which
// calls the member with them and hands by-reference arguments back
// (WfcStoreArg). compile_project parses it as part of the class.
[[nodiscard]] std::string thunk_source(const Module& module) {
    std::string out = "Option Explicit\n";
    for (const auto& procedure : module.procedures) {
        if (procedure.name.rfind("wfcthunk_", 0) == 0) {
            continue;
        }
        const bool is_let = procedure.property == PropertyKind::let ||
                            procedure.property == PropertyKind::set;
        const std::string kind =
            procedure.property == PropertyKind::get   ? "get"
            : procedure.property == PropertyKind::let ? "let"
            : procedure.property == PropertyKind::set ? "set"
                                                      : "call";
        std::vector<std::string> types;
        std::vector<bool> objects;
        bool usable = procedure.return_type != VbType::record;
        std::size_t required = 0;
        for (const auto& parameter : procedure.parameters) {
            std::string text;
            bool is_object = false;
            usable = usable && !parameter.is_array &&
                     thunk_type(parameter, text, is_object);
            types.push_back(text);
            objects.push_back(is_object);
            if (!parameter.is_optional) {
                ++required;
            }
        }
        if (!usable) {
            continue;
        }
        const std::string name = "WfcThunk_" + kind + "_" + procedure.name;
        const std::size_t total = procedure.parameters.size();
        out += "Public Function " + name + "() As Variant\n";
        out += "Dim wfcn As Long\n";
        for (std::size_t i = 0; i < total; ++i) {
            out += "Dim wfcp" + std::to_string(i) + " As " + types[i] + "\n";
        }
        out += "wfcn = WfcArgCount()\n";
        for (std::size_t i = 0; i < total; ++i) {
            out += "If wfcn > " + std::to_string(i) + " Then " +
                   (objects[i] ? "Set " : "") + "wfcp" + std::to_string(i) +
                   " = WfcArg(" + std::to_string(i) + ")\n";
        }
        const auto call_with = [&](const std::size_t given) {
            std::string list;
            const std::size_t shown = is_let ? given - 1 : given;
            for (std::size_t i = 0; i < shown; ++i) {
                list += (i > 0 ? ", " : "") + std::string("wfcp") +
                        std::to_string(i);
            }
            const std::string target =
                "Me." + procedure.spelling + "(" + list + ")";
            if (is_let) {
                const std::string value = "wfcp" + std::to_string(given - 1);
                return std::string(procedure.property == PropertyKind::set
                                       ? "Set "
                                       : "") +
                       target + " = " + value + "\n";
            }
            if (procedure.is_function ||
                procedure.property == PropertyKind::get) {
                const bool object_result =
                    procedure.return_type == VbType::object;
                return std::string(object_result ? "Set " : "") + name + " = " +
                       target + "\n";
            }
            return "Call " + target + "\n";
        };
        out += "Select Case wfcn\n";
        for (std::size_t given = required; given < total; ++given) {
            if (is_let && given == 0) {
                continue;
            }
            out += "Case " + std::to_string(given) + "\n" + call_with(given);
        }
        if (required > 0) {
            out +=
                "Case Is < " + std::to_string(required) + "\nErr.Raise 450\n";
        }
        out += "Case Else\n";
        if (total > 0 || !is_let) {
            out += call_with(total);
        }
        out += "End Select\n";
        for (std::size_t i = 0; i < total; ++i) {
            if (!procedure.parameters[i].by_value) {
                out += "If wfcn > " + std::to_string(i) + " Then WfcStoreArg " +
                       std::to_string(i) + ", wfcp" + std::to_string(i) + "\n";
            }
        }
        out += "End Function\n";
    }
    return out;
}

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
        collect_enum_names(item.source, options.enum_names);
    }
    collect_enum_names(std::string(source), options.enum_names);
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
        // The members the shared runtime can call get thunks.
        auto thunks =
            parse_module(thunk_source(parsed_class.module), class_options);
        if (!thunks.error.empty()) {
            result.error = "class " + item.name + " (thunks): " + thunks.error;
            result.error_at = thunks.error_at;
            return result;
        }
        for (auto& procedure : thunks.module.procedures) {
            parsed_class.module.procedures.push_back(std::move(procedure));
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
    LowerOptions lower_options;
    lower_options.host_globals.insert(info.form_names.begin(),
                                      info.form_names.end());
    lower_options.has_forms = !info.form_names.empty();
    lower_options.startup_form = info.startup_form;
    auto lowered = lower_module(parsed.module, class_pointers, lower_options);
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
