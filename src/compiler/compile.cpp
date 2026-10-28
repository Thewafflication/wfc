#include "compile.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <sstream>

#include "../interpreter/builtin_class_sources.hpp"
#include "wfc/evaluator.hpp"

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

// The names a source declares with a keyword (`[Public|Private] Enum Name`).
void collect_names(const std::string& source, const std::string& keyword,
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
        if (word == keyword) {
            std::string name;
            words >> name;
            if (!name.empty() && name.front() != '\'') {
                into.insert(name);
            }
        }
        position = end + 1U;
    }
}

void collect_enum_names(const std::string& source,
                        std::set<std::string>& into) {
    collect_names(source, "enum", into);
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
[[nodiscard]] std::string thunk_source(
    const Module& module,
    const std::map<std::string, std::pair<std::string, std::size_t>>&
        default_members) {
    std::string out = "Option Explicit\n";
    // Public fields are members too.
    for (const auto& field : module.globals) {
        if (!field.is_public || field.is_array || field.name == "wfchost") {
            continue;
        }
        Parameter shape;
        shape.type = field.type;
        shape.type_name = field.type_name;
        std::string type;
        bool is_object = false;
        if (!thunk_type(shape, type, is_object)) {
            continue;
        }
        const std::string set = is_object ? "Set " : "";
        const auto default_member = default_members.find(field.type_name);
        if (is_object && default_member != default_members.end() &&
            !default_member->second.first.empty()) {
            // `obj.Items(i)` is the default member of the object in the field.
            const std::string function = "WfcThunk_get_" + field.name;
            const std::string call =
                "Me." + field.name + "." + default_member->second.first;
            const std::size_t most =
                std::min<std::size_t>(default_member->second.second, 3U);
            out += "Public Function " + function +
                   "() As Variant\nDim wfcn As Long\nwfcn = WfcArgCount()\n"
                   "Select Case wfcn\nCase 0\nSet " +
                   function + " = Me." + field.name + "\n";
            for (std::size_t n = 1; n <= most; ++n) {
                out += "Case " + std::to_string(n) + "\n" + function + " = " +
                       call + "(";
                for (std::size_t k = 0; k < n; ++k) {
                    out += (k > 0 ? ", " : "") + std::string("WfcArg(") +
                           std::to_string(k) + ")";
                }
                out += ")\n";
            }
            out += "End Select\nEnd Function\n";
        } else {
            out += "Public Function WfcThunk_get_" + field.name +
                   "() As Variant\n" + set + "WfcThunk_get_" + field.name +
                   " = Me." + field.name + "\nEnd Function\n";
        }
        out += "Public Function WfcThunk_" +
               std::string(is_object ? "set_" : "let_") + field.name +
               "() As Variant\nDim wfcp0 As " + type + "\n" + set +
               "wfcp0 = WfcArg(0)\n" + set + "Me." + field.name +
               " = wfcp0\nEnd Function\n";
    }
    {
        // The default member, for `Print obj` and `obj(i)` of the runtime.
        std::string member = module.default_member;
        VbType returns = VbType::variant;
        std::size_t most = 0;
        std::size_t fewest = 0;
        bool found = false;
        for (const auto& procedure : module.procedures) {
            if (procedure.name.rfind("wfcthunk_", 0) == 0 ||
                procedure.property == PropertyKind::let ||
                procedure.property == PropertyKind::set) {
                continue;
            }
            if (procedure.is_default ||
                (!member.empty() && procedure.name == member)) {
                member = procedure.spelling;
                returns = procedure.return_type;
                most = std::min<std::size_t>(procedure.parameters.size(), 3U);
                for (const auto& parameter : procedure.parameters) {
                    if (!parameter.is_optional && !parameter.is_param_array) {
                        ++fewest;
                    }
                }
                fewest = std::min(fewest, most);
                found = procedure.is_function ||
                        procedure.property == PropertyKind::get;
                break;
            }
        }
        if (!found && !member.empty()) {
            for (const auto& field : module.globals) {
                if (field.name == member && field.is_public &&
                    !field.is_array && field.type != VbType::record) {
                    returns = field.type;
                    found = true;
                }
            }
        }
        if (found) {
            const std::string set = returns == VbType::object ? "Set " : "";
            out +=
                "Public Function WfcThunk_get__default() As Variant\n"
                "Dim wfcn As Long\nwfcn = WfcArgCount()\n"
                "Select Case wfcn\n";
            if (fewest == 0) {
                out += "Case 0\n" + set + "WfcThunk_get__default = Me." +
                       member + "\n";
            }
            for (std::size_t n = std::max<std::size_t>(fewest, 1U); n <= most;
                 ++n) {
                out += "Case " + std::to_string(n) + "\n" + set +
                       "WfcThunk_get__default = Me." + member + "(";
                for (std::size_t k = 0; k < n; ++k) {
                    out += (k > 0 ? ", " : "") + std::string("WfcArg(") +
                           std::to_string(k) + ")";
                }
                out += ")\n";
            }
            out += "End Select\nEnd Function\n";
        }
    }
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
        const bool indexable = !is_let &&
                               (procedure.is_function ||
                                procedure.property == PropertyKind::get) &&
                               (procedure.return_type == VbType::object ||
                                procedure.return_type == VbType::variant ||
                                procedure.return_type == VbType::none);
        if (indexable) {
            // More arguments than parameters: the rest index the result
            // (`obj.Kids(1)` where Kids is a property without parameters).
            const bool object_result = procedure.return_type == VbType::object;
            out += "Select Case wfcn - " + std::to_string(total) +
                   "\nCase 0\n" + call_with(total);
            for (std::size_t extra = 1; extra <= 3; ++extra) {
                std::string list;
                for (std::size_t i = 0; i < total; ++i) {
                    list += (i > 0 ? ", " : "") + std::string("wfcp") +
                            std::to_string(i);
                }
                std::string indexes;
                for (std::size_t k = 0; k < extra; ++k) {
                    indexes += (k > 0 ? ", " : "") + std::string("WfcArg(") +
                               std::to_string(total + k) + ")";
                }
                out += "Case " + std::to_string(extra) + "\n" +
                       (object_result ? "Set " : "") + name + " = Me." +
                       procedure.spelling + "(" + list + ")(" + indexes + ")\n";
            }
            out += "End Select\n";
        } else if (total > 0 || !is_let) {
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

CompileResult compile_project(const std::string_view raw_source,
                              const std::vector<ClassSource>& raw_classes,
                              const Architecture architecture,
                              const ProjectInfo& info) {
    CompileResult result;
    // Conditional compilation: the directives and the inactive branches are
    // blanked before anything reads the text.
    std::string source_text;
    std::vector<ClassSource> classes = raw_classes;
    {
        std::string problem;
        std::size_t problem_at = 0;
        auto processed =
            wfc::preprocess_conditionals(raw_source, problem, problem_at);
        if (!processed.has_value()) {
            result.error = problem;
            return result;
        }
        source_text = std::move(*processed);
        wfc::rewrite_source_forms(source_text);
        for (auto& item : classes) {
            auto text =
                wfc::preprocess_conditionals(item.source, problem, problem_at);
            if (!text.has_value()) {
                result.error = problem;
                return result;
            }
            item.source = std::move(*text);
            wfc::rewrite_source_forms(item.source);
        }
    }
    const std::string_view source = source_text;
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
    options.module_names.insert(info.module_names.begin(),
                                info.module_names.end());
    for (const auto& item : all) {
        options.class_names.insert(lower_case(item.name));
        if (item.is_form) {
            options.form_names.insert(lower_case(item.name));
        }
        collect_enum_names(item.source, options.enum_names);
        collect_names(item.source, "type", options.type_names);
    }
    collect_enum_names(std::string(source), options.enum_names);
    collect_names(std::string(source), "type", options.type_names);
    std::vector<Module> class_modules;
    class_modules.reserve(all.size());
    for (const auto& item : all) {
        ParseOptions class_options = options;
        class_options.is_class = true;
        class_options.is_form = item.is_form;
        class_options.name = lower_case(item.name);
        class_options.display_name = item.name;
        // A form is compiled as a class that holds the form of the engine.
        const std::string text =
            item.is_form ? item.source +
                               "\nPublic wfchost As Variant\n"
                               "Public Sub WfcSetHost(ByVal h As Variant)\n"
                               "wfchost = h\nEnd Sub\n"
                               "Public Function WfcGetHost() As Variant\n"
                               "WfcGetHost = wfchost\nEnd Function\n"
                         : item.source;
        auto parsed_class = parse_module(text, class_options);
        if (!parsed_class.error.empty()) {
            result.error = "class " + item.name + ": " + parsed_class.error;
            result.error_at = parsed_class.error_at;
            return result;
        }
        class_modules.push_back(std::move(parsed_class.module));
    }
    // The members the shared runtime can call get thunks.
    {
        std::map<std::string, std::pair<std::string, std::size_t>>
            default_members;
        for (const auto& module : class_modules) {
            std::string member = module.default_member;
            std::size_t parameters = 0;
            for (const auto& procedure : module.procedures) {
                if (procedure.is_default) {
                    member = procedure.spelling;
                    parameters = procedure.parameters.size();
                }
            }
            default_members[module.name] = {member, parameters};
        }
        for (std::size_t i = 0; i < class_modules.size(); ++i) {
            ParseOptions class_options = options;
            class_options.is_class = true;
            class_options.is_form = all[i].is_form;
            class_options.name = lower_case(all[i].name);
            class_options.display_name = all[i].name;
            auto thunks = parse_module(
                thunk_source(class_modules[i], default_members), class_options);
            if (!thunks.error.empty()) {
                result.error =
                    "class " + all[i].name + " (thunks): " + thunks.error;
                result.error_at = thunks.error_at;
                return result;
            }
            for (auto& procedure : thunks.module.procedures) {
                class_modules[i].procedures.push_back(std::move(procedure));
            }
        }
    }
    // The constants of the type libraries are constants of the module.
    std::string extended(source);
    if (!info.library_constants.empty()) {
        const std::string declared = lower_case(extended);
        extended += '\n';
        for (const auto& [name, value] : info.library_constants) {
            if (declared.find("const " + name) == std::string::npos) {
                extended += "Const " + name + " = " + value + '\n';
            }
        }
    }
    auto parsed = parse_module(extended, options);
    if (!parsed.error.empty()) {
        result.error = parsed.error;
        result.error_at = parsed.error_at;
        return result;
    }
    // A class with `VB_PredeclaredId` has an instance named like the class.
    for (const auto& module : class_modules) {
        if (module.predeclared && !module.is_form) {
            Declaration declaration;
            declaration.name = module.name;
            declaration.type = VbType::object;
            declaration.type_name = module.name;
            declaration.type_given = true;
            declaration.is_new = true;
            declaration.is_public = true;
            declaration.at = Position{1, 1};
            parsed.module.globals.push_back(std::move(declaration));
        }
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
    // Objects every program of the engine has.
    lower_options.host_globals.insert("forms");
    lower_options.host_globals.insert("printer");
    lower_options.host_globals.insert("printers");
    lower_options.host_globals.insert("licenses");
    lower_options.host_globals.insert("calendar");
    lower_options.host_globals.insert("imestatus");
    lower_options.has_forms = !info.form_names.empty();
    for (const auto& item : all) {
        if (item.is_form) {
            lower_options.form_controls[lower_case(item.name)] =
                std::set<std::string>(item.controls.begin(),
                                      item.controls.end());
        }
    }
    lower_options.startup_form = info.startup_form;
    auto lowered = lower_module(parsed.module, class_pointers, lower_options);
    if (!lowered.error.empty()) {
        result.error = lowered.error;
        result.error_at = lowered.error_at;
        return result;
    }
    char* dump_frames = nullptr;
    std::size_t dump_size = 0;
    const bool frames_wanted =
        _dupenv_s(&dump_frames, &dump_size, "WFC_DUMP_FRAMES") == 0 &&
        dump_frames != nullptr;
    std::free(dump_frames);
    if (frames_wanted) {
        // The largest frames, for finding what exhausts the stack.
        std::vector<std::pair<int, std::string>> frames;
        for (const auto& function : lowered.program.functions) {
            frames.emplace_back(function.slot_count, function.name);
        }
        std::sort(frames.rbegin(), frames.rend());
        for (std::size_t i = 0; i < frames.size() && i < 8U; ++i) {
            std::fprintf(stderr, "frame %d slots: %s", frames[i].first,
                         frames[i].second.c_str());
            std::fputc(10, stderr);
        }
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
