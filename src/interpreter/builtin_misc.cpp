// Interpreter: Miscellaneous and date/time intrinsic functions, and
// Rnd/Randomize. Internal to the WFC evaluator; not part of the public API.
// Split out of src/evaluator.cpp; see src/interpreter/README.md.

#include "interpreter.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace wfc::detail {
namespace {

#ifdef _WIN32
BOOL CALLBACK collect_font(const LOGFONTA* font, const TEXTMETRICA*,
                           DWORD font_type, LPARAM param) {
    if (font_type == TRUETYPE_FONTTYPE || font_type == 0) {
        auto* const names = reinterpret_cast<std::vector<std::string>*>(param);
        const std::string name = font->lfFaceName;
        if (std::find(names->begin(), names->end(), name) == names->end()) {
            names->push_back(name);
        }
    }
    return TRUE;
}

[[nodiscard]] const std::vector<std::string>& installed_fonts() {
    static const std::vector<std::string> fonts = [] {
        std::vector<std::string> names;
        if (const HDC context = GetDC(nullptr)) {
            LOGFONTA filter{};
            filter.lfCharSet = DEFAULT_CHARSET;
            EnumFontFamiliesExA(context, &filter, collect_font,
                                reinterpret_cast<LPARAM>(&names), 0);
            ReleaseDC(nullptr, context);
        }
        std::sort(names.begin(), names.end());
        return names;
    }();
    return fonts;
}
#endif

// Screen metrics for the built-in Screen class: 0/1 width and height in
// pixels, 2/3 horizontal and vertical DPI, 4 the installed font count.
[[nodiscard]] Integer system_metric(const Integer index) {
#ifdef _WIN32
    if (index == 0) {
        return GetSystemMetrics(SM_CXSCREEN);
    }
    if (index == 1) {
        return GetSystemMetrics(SM_CYSCREEN);
    }
    if (index == 2 || index == 3) {
        int dpi = 96;
        if (const HDC context = GetDC(nullptr)) {
            dpi = GetDeviceCaps(context, index == 2 ? LOGPIXELSX : LOGPIXELSY);
            ReleaseDC(nullptr, context);
        }
        return dpi > 0 ? dpi : 96;
    }
    if (index == 4) {
        return static_cast<Integer>(installed_fonts().size());
    }
    return 0;
#else
    static constexpr Integer defaults[] = {1920, 1080, 96, 96, 0};
    return index >= 0 && index < 5 ? defaults[index] : 0;
#endif
}

// The 0-based `index`th installed font, or empty when out of range.
[[nodiscard]] std::string system_font_name(const Integer index) {
#ifdef _WIN32
    const auto& fonts = installed_fonts();
    if (index >= 0 && static_cast<std::size_t>(index) < fonts.size()) {
        return fonts[static_cast<std::size_t>(index)];
    }
#else
    static_cast<void>(index);
#endif
    return {};
}

}  // namespace

bool Interpreter::is_misc_function_name(const std::string_view name) {
    static const std::unordered_set<std::string> names{"pmt",
                                                       "fv",
                                                       "pv",
                                                       "nper",
                                                       "ipmt",
                                                       "ppmt",
                                                       "npv",
                                                       "irr",
                                                       "sln",
                                                       "syd",
                                                       "ddb",
                                                       "formatnumber",
                                                       "formatcurrency",
                                                       "formatpercent",
                                                       "partition",
                                                       "doevents",
                                                       "imestatus",
                                                       "command",
                                                       "command$",
                                                       "cverr",
                                                       "cvdate",
                                                       "rate",
                                                       "mirr",
                                                       "msgbox",
                                                       "inputbox",
                                                       "createobject",
                                                       "getobject",
                                                       "getsetting",
                                                       "fileattr",
                                                       "filedatetime",
                                                       "getattr",
                                                       "callbyname",
                                                       "shell",
                                                       "getallsettings",
                                                       "objptr",
                                                       "strptr",
                                                       "wfcregexmatches",
                                                       "wfcregexreplace",
                                                       "wfcsys",
                                                       "wfcsysfont",
                                                       "wfcstore"};
    return names.contains(std::string(name));
}

std::optional<Value> Interpreter::evaluate_misc_function(
    const std::string_view name, std::vector<Value>& arguments,
    const std::size_t offset) {
    const auto count = arguments.size();
    const auto arity = [&](const std::size_t low, const std::size_t high) {
        if (count < low || count > high) {
            set_error("WFC0072",
                      "function received the wrong number of arguments",
                      offset);
            return false;
        }
        return true;
    };
    const auto number_at = [&](const std::size_t index,
                               const double fallback) -> std::optional<double> {
        if (index >= count) {
            return fallback;
        }
        const auto& v = arguments[index];
        if (std::holds_alternative<Empty>(v)) {
            return 0.0;
        }
        if (is_number(v) && !std::holds_alternative<Decimal>(v)) {
            return as_double(v);
        }
        if (const auto* flag = std::get_if<bool>(&v)) {
            return *flag ? -1.0 : 0.0;
        }
        return std::nullopt;
    };
    const auto bad_type = [&]() {
        set_error("WFC0073", "Type mismatch", offset);
        return std::nullopt;
    };
    const auto invalid_call = [&]() {
        static_cast<void>(
            raise_runtime(5, "Invalid procedure call or argument", offset));
        return std::nullopt;
    };
    const auto finite_result = [&](const double v) -> std::optional<Value> {
        if (!std::isfinite(v)) {
            static_cast<void>(raise_runtime(6, "Overflow", offset));
            return std::nullopt;
        }
        return Value{v};
    };
    if (name == "doevents" || name == "imestatus") {
        if (!arity(0, 0)) {
            return std::nullopt;
        }
        return Value{Integer{}};
    }
    if (name == "callbyname") {
        // CallByName(object, name, callType, args...) -- REQ-0263.
        if (count < 3U) {
            set_error("WFC0072",
                      "function received the wrong number of arguments",
                      offset);
            return std::nullopt;
        }
        const auto* holder = std::get_if<ObjectInstance>(&arguments[0]);
        const auto* member_text = std::get_if<std::string>(&arguments[1]);
        const auto call_type = number_at(2, 0.0);
        if (member_text == nullptr || !call_type) {
            return bad_type();
        }
        if (holder == nullptr) {
            if (!execute_) {
                return Value{Empty{}};
            }
            static_cast<void>(raise_runtime(
                91, "Object variable or With block variable not set", offset));
            return std::nullopt;
        }
        if (!execute_) {
            return Value{Empty{}};
        }
        std::string member;
        for (const char c : *member_text) {
            member.push_back(ascii_lower(c));
        }
        InstanceData& instance = *holder->data;
        const ClassDef& class_def = class_definitions_.at(instance.class_name);
        std::vector<CallArgument> call_arguments;
        for (std::size_t i = 3; i < count; ++i) {
            call_arguments.push_back(CallArgument{arguments[i], nullptr});
        }
        const int kind = static_cast<int>(*call_type);
        if (kind == 1) {  // vbMethod
            const auto method = class_def.methods.find(member);
            if (method == class_def.methods.end()) {
                static_cast<void>(raise_runtime(
                    438, "Object doesn't support this property or method",
                    offset));
                return std::nullopt;
            }
            return invoke_definition(method->second, member,
                                     std::move(call_arguments), offset,
                                     class_def.source, &instance);
        }
        if (kind == 2) {  // vbGet
            const auto getter = class_def.property_get.find(member);
            if (getter != class_def.property_get.end()) {
                return invoke_definition(getter->second, member,
                                         std::move(call_arguments), offset,
                                         class_def.source, &instance);
            }
            const auto field = instance.fields.variables.find(member);
            if (field != instance.fields.variables.end()) {
                return field->second;
            }
            const auto method = class_def.methods.find(member);
            if (method != class_def.methods.end()) {
                return invoke_definition(method->second, member,
                                         std::move(call_arguments), offset,
                                         class_def.source, &instance);
            }
            static_cast<void>(raise_runtime(
                438, "Object doesn't support this property or method", offset));
            return std::nullopt;
        }
        if ((kind == 4 || kind == 8) && !call_arguments.empty()) {
            const auto& table =
                kind == 4 ? class_def.property_let : class_def.property_set;
            const auto setter = table.find(member);
            if (setter != table.end()) {
                return invoke_definition(setter->second, member,
                                         std::move(call_arguments), offset,
                                         class_def.source, &instance);
            }
            const auto field = instance.fields.variables.find(member);
            if (field != instance.fields.variables.end() &&
                call_arguments.size() == 1U) {
                if (kind == 8) {
                    const auto declared =
                        instance.fields.object_class_names.find(member);
                    if (!assign_object_reference(
                            field->second,
                            declared != instance.fields.object_class_names.end()
                                ? declared->second
                                : std::string{},
                            call_arguments[0].value, offset)) {
                        return std::nullopt;
                    }
                } else {
                    Value value = call_arguments[0].value;
                    if (!instance.fields.variant_variables.contains(member) &&
                        !coerce_numeric_value(value, field->second.index(),
                                              offset)) {
                        return std::nullopt;
                    }
                    field->second = std::move(value);
                }
                return Value{Empty{}};
            }
        }
        static_cast<void>(raise_runtime(
            438, "Object doesn't support this property or method", offset));
        return std::nullopt;
    }
    if (name == "msgbox") {
        if (!arity(1, 5)) {
            return std::nullopt;
        }
        // There is no UI: answer with the dialog's default button.
        std::int64_t style = 0;
        if (count >= 2U && is_number(arguments[1]) &&
            !std::holds_alternative<Decimal>(arguments[1])) {
            style = static_cast<std::int64_t>(as_double(arguments[1]));
        }
        // Result value of the Nth button (0-based) for each button set.
        static const std::array<std::array<Integer, 3>, 6> buttons{{
            {1, 0, 0},  // vbOKOnly
            {1, 2, 0},  // vbOKCancel: OK, Cancel
            {3, 4, 5},  // vbAbortRetryIgnore
            {6, 7, 2},  // vbYesNoCancel
            {6, 7, 0},  // vbYesNo
            {4, 2, 0},  // vbRetryCancel
        }};
        static const std::array<std::size_t, 6> button_counts{1, 2, 3, 3, 2, 2};
        const auto set = static_cast<std::size_t>(style & 7);
        if (set >= buttons.size()) {
            return Value{Integer{1}};
        }
        auto index = static_cast<std::size_t>((style >> 8) & 3);
        if (index >= button_counts[set]) {
            index = 0;
        }
        return Value{buttons[set][index]};
    }
    if (name == "inputbox") {
        if (!arity(1, 7)) {
            return std::nullopt;
        }
        const auto* fallback =
            count >= 3U ? std::get_if<std::string>(&arguments[2]) : nullptr;
        return Value{fallback != nullptr ? *fallback : std::string{}};
    }
    if (name == "createobject" || name == "getobject") {
        if (!arity(name == "createobject" ? 1U : 0U, 2U)) {
            return std::nullopt;
        }
        if (!execute_) {
            return Value{Nothing{}};
        }
        if (name == "createobject") {
            if (const auto* progid =
                    count > 0U ? std::get_if<std::string>(&arguments[0])
                               : nullptr) {
                std::string lowered;
                for (const char c : *progid) {
                    lowered.push_back(ascii_lower(c));
                }
                if (lowered == "scripting.dictionary" &&
                    class_definitions_.contains("wfcdictionary")) {
                    return instantiate_class("wfcdictionary", offset);
                }
                if ((lowered == "vbscript.regexp" ||
                     lowered == "vbscript.regexp.55") &&
                    class_definitions_.contains("wfcregexp")) {
                    return instantiate_class("wfcregexp", offset);
                }
                if (lowered == "scripting.filesystemobject" &&
                    class_definitions_.contains("wfcfilesystemobject")) {
                    return instantiate_class("wfcfilesystemobject", offset);
                }
            }
        }
        if (name == "createobject") {
            if (const auto* progid = std::get_if<std::string>(&arguments[0])) {
                return com_create_object(*progid, offset);
            }
        } else {
            const auto* path =
                count > 0U ? std::get_if<std::string>(&arguments[0]) : nullptr;
            const auto* progid =
                count > 1U ? std::get_if<std::string>(&arguments[1]) : nullptr;
            return com_get_object(path != nullptr ? *path : std::string{},
                                  progid != nullptr ? *progid : std::string{},
                                  offset);
        }
        static_cast<void>(raise_runtime(
            429, "ActiveX component can't create object", offset));
        return std::nullopt;
    }
    if (name == "wfcregexmatches" || name == "wfcregexreplace") {
        const bool replacing = name == "wfcregexreplace";
        if (!arity(replacing ? 6 : 5, replacing ? 6 : 5)) {
            return std::nullopt;
        }
        const auto* pattern = std::get_if<std::string>(&arguments[0]);
        const auto* text = std::get_if<std::string>(&arguments[1]);
        if (pattern == nullptr || text == nullptr) {
            return bad_type();
        }
        const std::size_t flag_base = replacing ? 3U : 2U;
        const auto* replacement =
            replacing ? std::get_if<std::string>(&arguments[2]) : nullptr;
        if (replacing && replacement == nullptr) {
            return bad_type();
        }
        const auto flag = [&](const std::size_t index) {
            const auto* value = std::get_if<bool>(&arguments[index]);
            return value != nullptr && *value;
        };
        const bool ignore_case = flag(flag_base);
        const bool multi_line = flag(flag_base + 1U);
        const bool global = flag(flag_base + 2U);
        if (!execute_) {
            return replacing ? Value{std::string{}} : Value{Empty{}};
        }
        try {
            auto options = std::regex::ECMAScript;
            if (ignore_case) {
                options |= std::regex::icase;
            }
            if (multi_line) {
                // Older MSVC STLs lack the C++17 multiline flag; there ^/$
                // match only at the text ends.
                []<typename Regex>(auto& flags) {
                    if constexpr (requires { Regex::multiline; }) {
                        flags |= Regex::multiline;
                    }
                }.template operator()<std::regex>(options);
            }
            const std::regex expression(*pattern, options);
            if (replacing) {
                return Value{std::regex_replace(
                    *text, expression, *replacement,
                    global ? std::regex_constants::format_default
                           : std::regex_constants::format_first_only)};
            }
            std::vector<Value> found;
            for (auto it = std::sregex_iterator(text->begin(), text->end(),
                                                expression);
                 it != std::sregex_iterator(); ++it) {
                const std::smatch& match = *it;
                std::vector<Value> row;
                row.emplace_back(static_cast<Integer>(match.position(0)));
                row.emplace_back(static_cast<Integer>(match.length(0)));
                for (std::size_t group = 0; group < match.size(); ++group) {
                    row.emplace_back(match[group].str());
                }
                ArrayValue row_array{};
                row_array.elements = std::move(row);
                row_array.is_variant_element = true;
                row_array.element_type_index = Value{Empty{}}.index();
                found.emplace_back(std::move(row_array));
                if (!global) {
                    break;
                }
            }
            ArrayValue result{};
            result.elements = std::move(found);
            result.is_variant_element = true;
            result.element_type_index = Value{Empty{}}.index();
            return Value{std::move(result)};
        } catch (const std::regex_error&) {
            static_cast<void>(raise_runtime(
                5017, "Syntax error in regular expression", offset));
            return std::nullopt;
        }
    }
    if (name == "wfcsys" || name == "wfcsysfont") {
        // Screen metrics for the built-in Screen class.
        if (!arity(1, 1)) {
            return std::nullopt;
        }
        const auto index = whole_value(arguments[0]).value_or(0);
        if (name == "wfcsysfont") {
            return Value{system_font_name(index)};
        }
        return Value{system_metric(index)};
    }
    if (name == "wfcstore") {
        if (arguments.size() < 2U) {
            return bad_type();
        }
        const auto* op = std::get_if<Integer>(&arguments[0]);
        const auto* owner = std::get_if<ObjectInstance>(&arguments[1]);
        if (op == nullptr || owner == nullptr) {
            return bad_type();
        }
        if (!execute_) {
            return Value{Integer{0}};
        }
        auto& slot = owner->data->store;
        if (!slot) {
            slot = std::make_unique<NativeStore>();
        }
        NativeStore& store = *slot;
        const auto canonical = [&](const Value& key) {
            char buffer[40];
            if (const auto* text = std::get_if<std::string>(&key)) {
                std::string result = "s";
                result += store.text_compare ? fold_case(*text) : *text;
                return result;
            }
            if (is_number(key) || std::holds_alternative<bool>(key)) {
                std::snprintf(buffer, sizeof(buffer), "n%.17g",
                              std::holds_alternative<bool>(key)
                                  ? (std::get<bool>(key) ? -1.0 : 0.0)
                                  : as_double(key));
                return std::string(buffer);
            }
            if (const auto* date = std::get_if<DateValue>(&key)) {
                std::snprintf(buffer, sizeof(buffer), "d%.17g", date->serial);
                return std::string(buffer);
            }
            if (const auto* object = std::get_if<ObjectInstance>(&key)) {
                std::snprintf(buffer, sizeof(buffer), "o%p",
                              static_cast<const void*>(object->data.get()));
                return std::string(buffer);
            }
            return std::string("e");
        };
        const auto reindex = [&]() {
            store.index.clear();
            store.index.reserve(store.keys.size());
            for (std::size_t i = 0; i < store.keys.size(); ++i) {
                if (!std::holds_alternative<Empty>(store.keys[i])) {
                    store.index.emplace(canonical(store.keys[i]), i);
                }
            }
            store.dirty = false;
        };
        const auto position_at =
            [&](const std::size_t argument) -> std::optional<std::size_t> {
            if (argument >= arguments.size()) {
                return std::nullopt;
            }
            const auto* position = std::get_if<Integer>(&arguments[argument]);
            if (position == nullptr || *position < 1 ||
                static_cast<std::size_t>(*position) > store.keys.size()) {
                return std::nullopt;
            }
            return static_cast<std::size_t>(*position) - 1U;
        };
        const auto make_array = [&](std::vector<Value> elements) {
            ArrayValue result{};
            result.elements = std::move(elements);
            result.is_variant_element = true;
            result.element_type_index = Value{Empty{}}.index();
            return Value{std::move(result)};
        };
        switch (*op) {
            case 1:
                return Value{static_cast<Integer>(store.keys.size())};
            case 2: {
                if (arguments.size() < 3U) {
                    return bad_type();
                }
                if (store.dirty) {
                    reindex();
                }
                const auto found = store.index.find(canonical(arguments[2]));
                return Value{found == store.index.end()
                                 ? Integer{0}
                                 : static_cast<Integer>(found->second + 1U)};
            }
            case 3: {
                if (arguments.size() < 5U) {
                    return bad_type();
                }
                const auto* at = std::get_if<Integer>(&arguments[4]);
                if (at == nullptr) {
                    return bad_type();
                }
                const bool append = *at < 1 || static_cast<std::size_t>(*at) >
                                                   store.keys.size();
                const bool keyed = !std::holds_alternative<Empty>(arguments[2]);
                if (append) {
                    if (!store.dirty && keyed) {
                        store.index.emplace(canonical(arguments[2]),
                                            store.keys.size());
                    }
                    store.keys.push_back(arguments[2]);
                    store.values.push_back(arguments[3]);
                } else {
                    const auto where = static_cast<std::ptrdiff_t>(*at - 1);
                    store.keys.insert(store.keys.begin() + where, arguments[2]);
                    store.values.insert(store.values.begin() + where,
                                        arguments[3]);
                    store.dirty = true;
                }
                return Value{Integer{0}};
            }
            case 4:
            case 5:
            case 11: {
                const auto at = position_at(2);
                if (!at.has_value()) {
                    return *op == 11 ? Value{false} : Value{Empty{}};
                }
                if (*op == 11) {
                    return Value{is_object_reference(store.values[*at])};
                }
                return *op == 4 ? store.values[*at] : store.keys[*at];
            }
            case 6: {
                const auto at = position_at(2);
                if (!at.has_value() || arguments.size() < 4U) {
                    return bad_type();
                }
                Value old = std::move(store.values[*at]);
                store.values[*at] = arguments[3];
                if (!terminate_if_last_reference(old)) {
                    return std::nullopt;
                }
                return Value{Integer{0}};
            }
            case 7: {
                const auto at = position_at(2);
                if (!at.has_value() || arguments.size() < 4U) {
                    return bad_type();
                }
                store.keys[*at] = arguments[3];
                store.dirty = true;
                return Value{Integer{0}};
            }
            case 8: {
                const auto at = position_at(2);
                if (!at.has_value()) {
                    return bad_type();
                }
                Value old = std::move(store.values[*at]);
                store.keys.erase(store.keys.begin() +
                                 static_cast<std::ptrdiff_t>(*at));
                store.values.erase(store.values.begin() +
                                   static_cast<std::ptrdiff_t>(*at));
                store.dirty = true;
                if (!terminate_if_last_reference(old)) {
                    return std::nullopt;
                }
                return Value{Integer{0}};
            }
            case 9: {
                auto old = std::move(store.values);
                store.values.clear();
                store.keys.clear();
                store.index.clear();
                store.dirty = false;
                for (auto& value : old) {
                    if (!terminate_if_last_reference(value)) {
                        return std::nullopt;
                    }
                }
                return Value{Integer{0}};
            }
            case 10: {
                if (arguments.size() < 3U) {
                    return bad_type();
                }
                const auto* mode = std::get_if<Integer>(&arguments[2]);
                if (mode == nullptr) {
                    return bad_type();
                }
                store.text_compare = *mode != 0;
                store.dirty = true;
                return Value{Integer{0}};
            }
            case 12:
                return make_array(std::vector<Value>(store.values));
            case 13:
                return make_array(std::vector<Value>(store.keys));
            default:
                return bad_type();
        }
    }
    if (name == "objptr" || name == "strptr") {
        // Opaque, stable-per-object "addresses" for code that passes them
        // along.
        if (!arity(1, 1)) {
            return std::nullopt;
        }
        if (!execute_) {
            return Value{Integer{}};
        }
        std::uintptr_t address = 0;
        if (const auto* instance = std::get_if<ObjectInstance>(&arguments[0])) {
            address = reinterpret_cast<std::uintptr_t>(instance->data.get());
        } else if (const auto* text = std::get_if<std::string>(&arguments[0])) {
            address = text->empty()
                          ? 0U
                          : reinterpret_cast<std::uintptr_t>(text->data());
        } else if (name == "objptr") {
            return bad_type();
        }
        return Value{static_cast<Integer>(address & 0x7FFFFFFFU)};
    }
    if (name == "shell") {
        if (!arity(1, 2)) {
            return std::nullopt;
        }
        const auto* command = std::get_if<std::string>(&arguments[0]);
        if (command == nullptr) {
            return bad_type();
        }
        if (!execute_) {
            return Value{0.0};
        }
#ifdef _WIN32
        // Like VB6, start the program and return at once with its process id.
        Integer window_style = 1;  // vbNormalFocus
        if (arguments.size() == 2U) {
            window_style = whole_value(arguments[1]).value_or(1);
        }
        STARTUPINFOA startup{};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESHOWWINDOW;
        switch (window_style) {
            case 0:
                startup.wShowWindow = SW_HIDE;
                break;
            case 2:
                startup.wShowWindow = SW_SHOWMINIMIZED;
                break;
            case 3:
                startup.wShowWindow = SW_SHOWMAXIMIZED;
                break;
            case 4:
                startup.wShowWindow = SW_SHOWNOACTIVATE;
                break;
            case 6:
                startup.wShowWindow = SW_SHOWMINNOACTIVE;
                break;
            default:
                startup.wShowWindow = SW_SHOWNORMAL;
                break;
        }
        PROCESS_INFORMATION process{};
        std::string mutable_command = *command;
        if (CreateProcessA(nullptr, mutable_command.data(), nullptr, nullptr,
                           FALSE, window_style == 0 ? CREATE_NO_WINDOW : 0,
                           nullptr, nullptr, &startup, &process) == 0) {
            static_cast<void>(raise_runtime(53, "File not found", offset));
            return std::nullopt;
        }
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return Value{static_cast<double>(process.dwProcessId)};
#else
        if (std::system(command->c_str()) != 0) {
            static_cast<void>(raise_runtime(53, "File not found", offset));
            return std::nullopt;
        }
        return Value{
            1.0};  // a task id; the command has already run to completion
#endif
    }
    if (name == "getallsettings") {
        if (!arity(2, 2)) {
            return std::nullopt;
        }
        const auto* application = std::get_if<std::string>(&arguments[0]);
        const auto* section = std::get_if<std::string>(&arguments[1]);
        if (application == nullptr || section == nullptr) {
            return bad_type();
        }
        if (!execute_) {
            return Value{Empty{}};
        }
        const std::string prefix = *application + "\x01" + *section + "\x01";
        std::vector<Value> cells;
        for (const auto& [key, value] : settings_) {
            if (key.rfind(prefix, 0) == 0) {
                std::string setting_name = key.substr(prefix.size());
                if (!setting_name.empty() && setting_name.back() == '\x01') {
                    setting_name.pop_back();
                }
                cells.push_back(Value{std::move(setting_name)});
                cells.push_back(Value{value});
            }
        }
        if (cells.empty()) {
            return Value{Empty{}};
        }
        ArrayValue result{};
        result.elements = std::move(cells);
        result.is_variant_element = true;
        result.element_type_index = Value{Empty{}}.index();
        result.dimensions = {
            {0, static_cast<Integer>(result.elements.size() / 2U) - 1}, {0, 1}};
        return Value{std::move(result)};
    }
    if (name == "getsetting") {
        if (!arity(3, 4)) {
            return std::nullopt;
        }
        std::string key;
        for (std::size_t i = 0; i < 3U; ++i) {
            const auto* part = std::get_if<std::string>(&arguments[i]);
            if (part == nullptr) {
                return bad_type();
            }
            key += *part + "\x01";
        }
        const auto found = settings_.find(key);
        if (found != settings_.end()) {
            return Value{found->second};
        }
        const auto* fallback =
            count == 4U ? std::get_if<std::string>(&arguments[3]) : nullptr;
        return Value{fallback != nullptr ? *fallback : std::string{}};
    }
    if (name == "cvdate") {
        if (!arity(1, 1)) {
            return std::nullopt;
        }
        if (const auto* text = std::get_if<std::string>(&arguments[0])) {
            if (const auto parsed = parse_date_text(*text)) {
                return Value{DateValue{*parsed}};
            }
            return bad_type();
        }
        if (std::holds_alternative<DateValue>(arguments[0])) {
            return arguments[0];
        }
        const auto number = number_at(0, 0.0);
        if (!number) {
            return bad_type();
        }
        return Value{DateValue{*number}};
    }
    if (name == "fileattr") {
        if (!arity(2, 2)) {
            return std::nullopt;
        }
        const auto file_number = number_at(0, 0.0);
        const auto kind = number_at(1, 1.0);
        if (!file_number || !kind) {
            return bad_type();
        }
        if (!execute_) {
            return Value{Integer{}};
        }
        auto* const file =
            find_open_file(static_cast<Integer>(*file_number), offset);
        if (file == nullptr) {
            return std::nullopt;
        }
        if (*kind == 2.0) {
            return Value{static_cast<Integer>(*file_number)};
        }
        constexpr Integer modes[] = {0, 1, 2, 8, 32, 4};
        return Value{modes[file->mode]};
    }
    if (name == "filedatetime" || name == "getattr") {
        if (!arity(1, 1)) {
            return std::nullopt;
        }
        const auto* path = std::get_if<std::string>(&arguments[0]);
        if (path == nullptr) {
            return bad_type();
        }
        if (!execute_) {
            return name == "getattr" ? Value{Integer{}} : Value{DateValue{}};
        }
        std::error_code ec;
        const std::filesystem::path target(*path);
        if (!std::filesystem::exists(target, ec)) {
            static_cast<void>(raise_runtime(53, "File not found", offset));
            return std::nullopt;
        }
        if (name == "getattr") {
            const auto permissions =
                std::filesystem::status(target, ec).permissions();
            const bool read_only =
                (permissions & std::filesystem::perms::owner_write) ==
                std::filesystem::perms::none;
            const Integer base =
                std::filesystem::is_directory(target, ec) ? 16 : 32;
            return Value{static_cast<Integer>(base | (read_only ? 1 : 0))};
        }
        const auto stamp = std::filesystem::last_write_time(target, ec);
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
                                 stamp.time_since_epoch())
                                 .count();
        // file_clock's epoch is implementation-defined; report the wall
        // clock "now" shifted by the file's age instead.
        const auto age =
            std::chrono::duration_cast<std::chrono::seconds>(
                std::filesystem::file_time_type::clock::now() - stamp)
                .count();
        static_cast<void>(seconds);
        return Value{DateValue{current_date_serial() -
                               static_cast<double>(age) / 86400.0}};
    }
    if (name == "rate") {
        if (!arity(3, 6)) {
            return std::nullopt;
        }
        const auto n = number_at(0, 0.0);
        const auto payment = number_at(1, 0.0);
        const auto pv = number_at(2, 0.0);
        const auto fv = number_at(3, 0.0);
        const auto type = number_at(4, 0.0);
        const auto guess = number_at(5, 0.1);
        if (!n || !payment || !pv || !fv || !type || !guess) {
            return bad_type();
        }
        if (!execute_) {
            return Value{0.0};
        }
        double r = *guess;
        const auto f = [&](const double rate) {
            if (rate == 0.0) {
                return *pv + *payment * *n + *fv;
            }
            const double g = std::pow(1.0 + rate, *n);
            return *pv * g +
                   *payment * (1.0 + rate * *type) * (g - 1.0) / rate + *fv;
        };
        for (int i = 0; i < 100; ++i) {
            const double value = f(r);
            const double h = 1e-7;
            const double slope = (f(r + h) - value) / h;
            if (slope == 0.0 || !std::isfinite(slope)) {
                break;
            }
            const double next = r - value / slope;
            if (!std::isfinite(next)) {
                break;
            }
            if (std::fabs(next - r) < 1e-10) {
                return finite_result(next);
            }
            r = next;
        }
        return invalid_call();
    }
    if (name == "mirr") {
        if (!arity(3, 3)) {
            return std::nullopt;
        }
        const auto* values = std::get_if<ArrayValue>(&arguments[0]);
        const auto finance = number_at(1, 0.0);
        const auto reinvest = number_at(2, 0.0);
        if (values == nullptr || !finance || !reinvest) {
            return bad_type();
        }
        if (!execute_) {
            return Value{0.0};
        }
        double positive = 0.0;
        double negative = 0.0;
        const auto n = static_cast<double>(values->elements.size());
        for (std::size_t i = 0; i < values->elements.size(); ++i) {
            if (!is_number(values->elements[i])) {
                return bad_type();
            }
            const double v = as_double(values->elements[i]);
            if (v > 0.0) {
                positive += v * std::pow(1.0 + *reinvest,
                                         n - 1.0 - static_cast<double>(i));
            }
            if (v < 0.0) {
                negative +=
                    v / std::pow(1.0 + *finance, static_cast<double>(i));
            }
        }
        if (positive == 0.0 || negative == 0.0) {
            return invalid_call();
        }
        return finite_result(std::pow(positive / -negative, 1.0 / (n - 1.0)) -
                             1.0);
    }
    if (name == "cverr") {
        if (!arity(1, 1)) {
            return std::nullopt;
        }
        const auto number = number_at(0, 0.0);
        if (!number || *number < 0.0 || *number > 65535.0) {
            return invalid_call();
        }
        return Value{ErrorValue{static_cast<std::int32_t>(*number)}};
    }
    if (name == "command" || name == "command$") {
        if (!arity(0, 0)) {
            return std::nullopt;
        }
        return Value{std::string{}};
    }
    // pmt/fv/pv/nper share (rate, x, y[, z[, type]]).
    const auto fv_of = [](const double r, const double n, const double pmt,
                          const double pv, const double type) {
        if (r == 0.0) {
            return -(pv + pmt * n);
        }
        const double growth = std::pow(1.0 + r, n);
        return -(pv * growth + pmt * (1.0 + r * type) * (growth - 1.0) / r);
    };
    const auto pmt_of = [](const double r, const double n, const double pv,
                           const double fv, const double type) {
        if (r == 0.0) {
            return -(pv + fv) / n;
        }
        const double growth = std::pow(1.0 + r, n);
        return (-fv - pv * growth) * r / ((1.0 + r * type) * (growth - 1.0));
    };
    if (name == "pmt" || name == "fv" || name == "pv" || name == "nper") {
        if (!arity(3, 5)) {
            return std::nullopt;
        }
        const auto a = number_at(0, 0.0);
        const auto b = number_at(1, 0.0);
        const auto c = number_at(2, 0.0);
        const auto d = number_at(3, 0.0);
        const auto t = number_at(4, 0.0);
        if (!a || !b || !c || !d || !t) {
            return bad_type();
        }
        if (!execute_) {
            return Value{0.0};
        }
        const double r = *a;
        if (name == "pmt") {
            if (*b == 0.0) {
                return invalid_call();
            }
            return finite_result(pmt_of(r, *b, *c, *d, *t));
        }
        if (name == "fv") {
            return finite_result(fv_of(r, *b, *c, *d, *t));
        }
        if (name == "pv") {
            if (r == 0.0) {
                return finite_result(-(*d + *c * *b));
            }
            const double growth = std::pow(1.0 + r, *b);
            return finite_result(
                (-*d - *c * (1.0 + r * *t) * (growth - 1.0) / r) / growth);
        }
        // nper(rate, pmt, pv, fv, type)
        if (r == 0.0) {
            if (*b == 0.0) {
                return invalid_call();
            }
            return finite_result(-(*c + *d) / *b);
        }
        const double top = *b * (1.0 + r * *t) - *d * r;
        const double bottom = *b * (1.0 + r * *t) + *c * r;
        if (top / bottom <= 0.0) {
            return invalid_call();
        }
        return finite_result(std::log(top / bottom) / std::log(1.0 + r));
    }
    if (name == "ipmt" || name == "ppmt") {
        if (!arity(4, 6)) {
            return std::nullopt;
        }
        const auto r = number_at(0, 0.0);
        const auto per = number_at(1, 0.0);
        const auto n = number_at(2, 0.0);
        const auto pv = number_at(3, 0.0);
        const auto fv = number_at(4, 0.0);
        const auto t = number_at(5, 0.0);
        if (!r || !per || !n || !pv || !fv || !t) {
            return bad_type();
        }
        if (!execute_) {
            return Value{0.0};
        }
        if (*per < 1.0 || *per > *n || *n <= 0.0) {
            return invalid_call();
        }
        const double payment = pmt_of(*r, *n, *pv, *fv, *t);
        double interest = 0.0;
        if (*t == 0.0) {
            interest = fv_of(*r, *per - 1.0, payment, *pv, 0.0) * *r;
        } else if (*per > 1.0) {
            interest = fv_of(*r, *per - 2.0, payment, *pv, 1.0) * *r;
        }
        return finite_result(name == "ipmt" ? interest : payment - interest);
    }
    if (name == "npv" || name == "irr") {
        if (!arity(2, 2U) && !(name == "irr" && count == 1U)) {
            return std::nullopt;
        }
        const ArrayValue* values = nullptr;
        const std::size_t array_index = name == "npv" ? 1U : 0U;
        if (array_index < count) {
            values = std::get_if<ArrayValue>(&arguments[array_index]);
        }
        const auto first = number_at(name == "npv" ? 0U : 1U, 0.1);
        if (values == nullptr || !first) {
            return bad_type();
        }
        if (!execute_) {
            return Value{0.0};
        }
        std::vector<double> flows;
        for (const auto& element : values->elements) {
            if (!is_number(element) ||
                std::holds_alternative<Decimal>(element)) {
                return bad_type();
            }
            flows.push_back(as_double(element));
        }
        const auto npv_at = [&](const double r, const double shift) {
            double total = 0.0;
            for (std::size_t i = 0; i < flows.size(); ++i) {
                total += flows[i] /
                         std::pow(1.0 + r, static_cast<double>(i) + shift);
            }
            return total;
        };
        if (name == "npv") {
            return finite_result(npv_at(*first, 1.0));
        }
        bool has_positive = false, has_negative = false;
        for (const double f : flows) {
            has_positive = has_positive || f > 0.0;
            has_negative = has_negative || f < 0.0;
        }
        if (!has_positive || !has_negative) {
            return invalid_call();
        }
        double r = *first;
        for (int iteration = 0; iteration < 100; ++iteration) {
            const double f = npv_at(r, 0.0);
            const double h = 1e-7;
            const double slope = (npv_at(r + h, 0.0) - f) / h;
            if (slope == 0.0) {
                break;
            }
            const double next = r - f / slope;
            if (!std::isfinite(next) || next <= -1.0) {
                return invalid_call();
            }
            if (std::fabs(next - r) < 1e-10) {
                return finite_result(next);
            }
            r = next;
        }
        return invalid_call();
    }
    if (name == "sln" || name == "syd" || name == "ddb") {
        if (!arity(name == "sln" ? 3U : 4U,
                   name == "ddb" ? 5U : (name == "sln" ? 3U : 4U))) {
            return std::nullopt;
        }
        const auto cost = number_at(0, 0.0);
        const auto salvage = number_at(1, 0.0);
        const auto life = number_at(2, 0.0);
        const auto period = number_at(3, 1.0);
        const auto factor = number_at(4, 2.0);
        if (!cost || !salvage || !life || !period || !factor) {
            return bad_type();
        }
        if (!execute_) {
            return Value{0.0};
        }
        if (*life <= 0.0) {
            return invalid_call();
        }
        if (name == "sln") {
            return finite_result((*cost - *salvage) / *life);
        }
        if (*period < 1.0 || *period > *life) {
            return invalid_call();
        }
        if (name == "syd") {
            return finite_result((*cost - *salvage) * (*life - *period + 1.0) *
                                 2.0 / (*life * (*life + 1.0)));
        }
        double total = 0.0;
        double depreciation = 0.0;
        for (int p = 1; p <= static_cast<int>(std::ceil(*period)); ++p) {
            depreciation = std::min((*cost - total) * *factor / *life,
                                    *cost - *salvage - total);
            if (depreciation < 0.0) {
                depreciation = 0.0;
            }
            total += depreciation;
        }
        return finite_result(depreciation);
    }
    if (name == "formatnumber" || name == "formatcurrency" ||
        name == "formatpercent") {
        if (!arity(1, 5)) {
            return std::nullopt;
        }
        const auto value = number_at(0, 0.0);
        const auto digits = number_at(1, -1.0);
        if (!value || !digits) {
            return bad_type();
        }
        if (!execute_) {
            return Value{std::string{}};
        }
        const bool percent = name == "formatpercent";
        const bool currency = name == "formatcurrency";
        const int places = *digits < 0.0 ? 2 : static_cast<int>(*digits);
        double scaled = percent ? *value * 100.0 : *value;
        const bool negative = scaled < 0.0;
        // Rounded half away from zero on the decimal form, as Format does.
        std::string digits_text = fixed_half_up(std::fabs(scaled), places);
        const auto point = digits_text.find('.');
        std::string whole = digits_text.substr(0, point);
        const std::string fraction =
            point == std::string::npos ? "" : digits_text.substr(point);
        if (count >= 3U && whole == "0") {
            bool include_leading = true;
            if (const auto* flag = std::get_if<bool>(&arguments[2])) {
                include_leading = *flag;
            } else if (const auto leading = number_at(2, -2.0)) {
                include_leading = *leading != 0.0;
            }
            if (!include_leading && !fraction.empty()) {
                whole.clear();
            }
        }
        bool group = true;
        if (count >= 5U) {
            if (const auto* g = std::get_if<bool>(&arguments[4])) {
                group = *g;
            } else if (const auto grouping = number_at(4, -2.0)) {
                group = *grouping != 0.0;
            }
        }
        if (group) {
            std::string grouped;
            for (std::size_t i = 0; i < whole.size(); ++i) {
                if (i > 0 && (whole.size() - i) % 3 == 0) {
                    grouped.push_back(',');
                }
                grouped.push_back(whole[i]);
            }
            whole = grouped;
        }
        std::string text = whole + fraction;
        if (currency) {
            text = "$" + text;
        }
        if (percent) {
            text += "%";
        }
        bool parens = currency;
        if (count >= 4U) {
            if (const auto* p = std::get_if<bool>(&arguments[3])) {
                parens = *p;
            } else if (const auto flag = number_at(3, -2.0)) {
                parens = *flag == -1.0 || *flag == 1.0;
            }
        }
        if (negative && text.find_first_of("123456789") != std::string::npos) {
            text = parens ? "(" + text + ")" : "-" + text;
        }
        return Value{std::move(text)};
    }
    // partition(number, start, stop, interval)
    if (!arity(4, 4)) {
        return std::nullopt;
    }
    const auto number = number_at(0, 0.0);
    const auto start = number_at(1, 0.0);
    const auto stop = number_at(2, 0.0);
    const auto interval = number_at(3, 1.0);
    if (!number || !start || !stop || !interval) {
        return bad_type();
    }
    if (!execute_) {
        return Value{std::string{}};
    }
    if (*start < 0.0 || *stop <= *start || *interval < 1.0) {
        return invalid_call();
    }
    const auto width = std::to_string(static_cast<long long>(*stop) + 1).size();
    const auto pad = [&](const long long v) {
        std::string text = std::to_string(v);
        return std::string(text.size() < width ? width - text.size() : 0U,
                           ' ') +
               text;
    };
    const std::string blank(width, ' ');
    const long long n = static_cast<long long>(std::floor(*number));
    const long long lo = static_cast<long long>(*start);
    const long long hi = static_cast<long long>(*stop);
    const long long step = static_cast<long long>(*interval);
    if (n < lo) {
        return Value{blank + ":" + pad(lo - 1)};
    }
    if (n > hi) {
        return Value{pad(hi + 1) + ":" + blank};
    }
    const long long lower = lo + (n - lo) / step * step;
    const long long upper = std::min(lower + step - 1, hi);
    return Value{pad(lower) + ":" + pad(upper)};
}

bool Interpreter::is_date_function_name(const std::string_view name) {
    static const std::unordered_set<std::string> names{
        "now",       "date",      "date$",     "time",        "time$",
        "timer",     "year",      "month",     "day",         "hour",
        "minute",    "second",    "weekday",   "dateserial",  "timeserial",
        "datevalue", "timevalue", "dateadd",   "datediff",    "datepart",
        "isdate",    "cdate",     "monthname", "weekdayname", "formatdatetime"};
    return names.contains(std::string(name));
}

double Interpreter::current_date_serial() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    return date_serial(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday) +
           static_cast<double>(local.tm_hour * 3600 + local.tm_min * 60 +
                               local.tm_sec) /
               86400.0;
}

std::optional<Value> Interpreter::evaluate_date_function(
    const std::string_view name, std::vector<Value>& arguments,
    const std::size_t offset) {
    const auto count = arguments.size();
    const auto arity = [&](const std::size_t low, const std::size_t high) {
        if (count < low || count > high) {
            set_error("WFC0072",
                      "function received the wrong number of arguments",
                      offset);
            return false;
        }
        return true;
    };
    const auto mismatch = [&]() {
        set_error("WFC0073", "Type mismatch", offset);
        return std::nullopt;
    };
    // Any Date/number/parseable-String argument, as a serial.
    const auto serial_at =
        [&](const std::size_t index) -> std::optional<double> {
        const auto& v = arguments[index];
        if (const auto* d = std::get_if<DateValue>(&v)) {
            return d->serial;
        }
        if (is_number(v) && !std::holds_alternative<Decimal>(v)) {
            return as_double(v);
        }
        if (const auto* t = std::get_if<std::string>(&v)) {
            return parse_date_text(*t);
        }
        return std::nullopt;
    };
    const auto long_at =
        [&](const std::size_t index) -> std::optional<std::int64_t> {
        const auto& v = arguments[index];
        if (const auto* i = std::get_if<Integer>(&v)) {
            return *i;
        }
        if (const auto* i = std::get_if<Int16>(&v)) {
            return *i;
        }
        if (is_number(v) && !std::holds_alternative<Decimal>(v)) {
            return static_cast<std::int64_t>(std::nearbyint(as_double(v)));
        }
        return std::nullopt;
    };
    static const char* const month_names[] = {
        "January", "February", "March",     "April",   "May",      "June",
        "July",    "August",   "September", "October", "November", "December"};
    static const char* const day_names[] = {"Sunday",    "Monday",   "Tuesday",
                                            "Wednesday", "Thursday", "Friday",
                                            "Saturday"};

    if (name == "now" || name == "date" || name == "date$" || name == "time" ||
        name == "time$" || name == "timer") {
        if (!arity(0, 0)) {
            return std::nullopt;
        }
        if (name == "timer") {
            if (!execute_) {
                return Value{0.0f};
            }
            const double now = current_date_serial();
            return Value{static_cast<float>((now - std::floor(now)) * 86400.0)};
        }
        if (name.back() == '$') {
            if (!execute_) {
                return Value{std::string{}};
            }
            const auto parts = split_date(current_date_serial());
            char text[32];
            if (name[0] == 'd') {
                std::snprintf(text, sizeof(text), "%02d-%02d-%04d",
                              static_cast<int>(parts.month),
                              static_cast<int>(parts.day),
                              static_cast<int>(parts.year));
            } else {
                std::snprintf(text, sizeof(text), "%02d:%02d:%02d",
                              static_cast<int>(parts.hour),
                              static_cast<int>(parts.minute),
                              static_cast<int>(parts.second));
            }
            return Value{std::string{text}};
        }
        if (!execute_) {
            return Value{DateValue{}};
        }
        const double now = current_date_serial();
        if (name == "now") {
            return Value{DateValue{now}};
        }
        if (name[0] == 'd') {
            return Value{DateValue{std::floor(now)}};
        }
        return Value{DateValue{now - std::floor(now)}};
    }
    if (name == "year" || name == "month" || name == "day" || name == "hour" ||
        name == "minute" || name == "second") {
        if (!arity(1, 1)) {
            return std::nullopt;
        }
        const auto serial = serial_at(0);
        if (!serial.has_value()) {
            return mismatch();
        }
        const auto parts = split_date(*serial);
        const auto value = name == "year"     ? parts.year
                           : name == "month"  ? parts.month
                           : name == "day"    ? parts.day
                           : name == "hour"   ? parts.hour
                           : name == "minute" ? parts.minute
                                              : parts.second;
        return Value{static_cast<Integer>(value)};
    }
    if (name == "weekday") {
        if (!arity(1, 2)) {
            return std::nullopt;
        }
        const auto serial = serial_at(0);
        const auto first =
            count == 2U ? long_at(1) : std::optional<std::int64_t>{1};
        if (!serial.has_value() || !first.has_value()) {
            return mismatch();
        }
        const std::int64_t first_day = *first == 0 ? 1 : *first;
        if (first_day < 1 || first_day > 7) {
            set_error("WFC0101", "Invalid procedure call or argument", offset);
            return std::nullopt;
        }
        return Value{static_cast<Integer>(
            (split_date(*serial).weekday - first_day + 7) % 7 + 1)};
    }
    if (name == "dateserial") {
        if (!arity(3, 3)) {
            return std::nullopt;
        }
        const auto y = long_at(0);
        const auto m = long_at(1);
        const auto d = long_at(2);
        if (!y || !m || !d) {
            return mismatch();
        }
        std::int64_t year = *y;
        if (year >= 0 && year <= 99) {
            year += year < 30 ? 2000 : 1900;
        }
        return Value{DateValue{date_serial(year, *m, *d)}};
    }
    if (name == "timeserial") {
        if (!arity(3, 3)) {
            return std::nullopt;
        }
        const auto h = long_at(0);
        const auto m = long_at(1);
        const auto sec = long_at(2);
        if (!h || !m || !sec) {
            return mismatch();
        }
        const double total =
            static_cast<double>(*h * 3600 + *m * 60 + *sec) / 86400.0;
        return Value{DateValue{total - std::floor(total)}};
    }
    if (name == "datevalue" || name == "timevalue") {
        if (!arity(1, 1)) {
            return std::nullopt;
        }
        const auto serial = serial_at(0);
        if (!serial.has_value()) {
            return mismatch();
        }
        return Value{DateValue{name == "datevalue"
                                   ? std::floor(*serial)
                                   : *serial - std::floor(*serial)}};
    }
    if (name == "cdate") {
        if (!arity(1, 1)) {
            return std::nullopt;
        }
        if (std::holds_alternative<Null>(arguments[0])) {
            set_error("WFC0104", "Invalid use of Null", offset);
            return std::nullopt;
        }
        const auto serial = serial_at(0);
        if (!serial.has_value()) {
            return mismatch();
        }
        if (*serial < -657435.0 || *serial >= 2958466.0) {
            set_error("WFC0009", "numeric overflow",
                      offset);  // outside year 100..9999
            return std::nullopt;
        }
        return Value{DateValue{*serial}};
    }
    if (name == "isdate") {
        if (!arity(1, 1)) {
            return std::nullopt;
        }
        const auto& v = arguments[0];
        return Value{std::holds_alternative<DateValue>(v) ||
                     (std::holds_alternative<std::string>(v) &&
                      serial_at(0).has_value())};
    }
    if (name == "monthname") {
        if (!arity(1, 2)) {
            return std::nullopt;
        }
        const auto m = long_at(0);
        if (!m) {
            return mismatch();
        }
        if (*m < 1 || *m > 12) {
            set_error("WFC0101", "Invalid procedure call or argument", offset);
            return std::nullopt;
        }
        std::string text = month_names[*m - 1];
        if (count == 2U && std::holds_alternative<bool>(arguments[1]) &&
            std::get<bool>(arguments[1])) {
            text.resize(3);
        }
        return Value{std::move(text)};
    }
    if (name == "weekdayname") {
        if (!arity(1, 3)) {
            return std::nullopt;
        }
        const auto w = long_at(0);
        const auto first =
            count == 3U ? long_at(2) : std::optional<std::int64_t>{1};
        if (!w || !first) {
            return mismatch();
        }
        const std::int64_t first_day = *first == 0 ? 1 : *first;
        if (*w < 1 || *w > 7 || first_day < 1 || first_day > 7) {
            set_error("WFC0101", "Invalid procedure call or argument", offset);
            return std::nullopt;
        }
        std::string text = day_names[(*w - 1 + first_day - 1) % 7];
        if (count >= 2U && std::holds_alternative<bool>(arguments[1]) &&
            std::get<bool>(arguments[1])) {
            text.resize(3);
        }
        return Value{std::move(text)};
    }
    if (name == "formatdatetime") {
        if (!arity(1, 2)) {
            return std::nullopt;
        }
        const auto serial = serial_at(0);
        const auto style =
            count == 2U ? long_at(1) : std::optional<std::int64_t>{0};
        if (!serial || !style) {
            return mismatch();
        }
        const auto parts = split_date(*serial);
        char buffer[64];
        switch (*style) {
            case 0:
                return Value{render_date(*serial)};
            case 1:
                return Value{std::string(day_names[parts.weekday - 1]) + ", " +
                             month_names[parts.month - 1] + " " +
                             std::to_string(parts.day) + ", " +
                             std::to_string(parts.year)};
            case 2:
                return Value{render_date_part(parts)};
            case 3:
                return Value{render_time_part(parts)};
            case 4:
                std::snprintf(buffer, sizeof(buffer), "%02lld:%02lld",
                              static_cast<long long>(parts.hour),
                              static_cast<long long>(parts.minute));
                return Value{std::string(buffer)};
            default:
                set_error("WFC0101", "Invalid procedure call or argument",
                          offset);
                return std::nullopt;
        }
    }
    // DateAdd / DateDiff / DatePart: interval first.
    if (name == "dateadd" || name == "datediff" || name == "datepart") {
        if (!(name == "dateadd"    ? arity(3, 3)
              : name == "datediff" ? arity(3, 5)
                                   : arity(2, 4))) {
            return std::nullopt;
        }
        const auto* interval_text = std::get_if<std::string>(&arguments[0]);
        if (interval_text == nullptr) {
            return mismatch();
        }
        std::string interval;
        for (const char c : *interval_text) {
            interval.push_back(ascii_lower(c));
        }
        static const std::unordered_set<std::string> intervals{
            "yyyy", "q", "m", "y", "d", "w", "ww", "h", "n", "s"};
        if (!intervals.contains(interval)) {
            set_error("WFC0101", "Invalid procedure call or argument", offset);
            return std::nullopt;
        }
        if (name == "dateadd") {
            const auto amount = long_at(1);
            const auto serial = serial_at(2);
            if (!amount || !serial) {
                return mismatch();
            }
            const auto n = *amount;
            if (interval == "yyyy" || interval == "q" || interval == "m") {
                const auto parts = split_date(*serial);
                const std::int64_t months = interval == "yyyy" ? n * 12
                                            : interval == "q"  ? n * 3
                                                               : n;
                std::int64_t total =
                    parts.year * 12 + (parts.month - 1) + months;
                const std::int64_t year =
                    total >= 0 ? total / 12 : -((11 - total) / 12);
                const std::int64_t month = total - year * 12 + 1;
                const std::int64_t day =
                    std::min(parts.day, days_in_month(year, month));
                const double time_of_day = *serial - std::floor(*serial);
                return Value{
                    DateValue{date_serial(year, month, day) + time_of_day}};
            }
            double delta = 0.0;
            if (interval == "d" || interval == "y" || interval == "w") {
                delta = static_cast<double>(n);
            } else if (interval == "ww") {
                delta = static_cast<double>(n * 7);
            } else if (interval == "h") {
                delta = static_cast<double>(n) / 24.0;
            } else if (interval == "n") {
                delta = static_cast<double>(n) / 1440.0;
            } else {
                delta = static_cast<double>(n) / 86400.0;
            }
            return Value{DateValue{*serial + delta}};
        }
        if (name == "datediff") {
            const auto first = serial_at(1);
            const auto second = serial_at(2);
            if (!first || !second) {
                return mismatch();
            }
            const auto a = split_date(*first);
            const auto b = split_date(*second);
            std::int64_t result{};
            if (interval == "yyyy") {
                result = b.year - a.year;
            } else if (interval == "m") {
                result = (b.year * 12 + b.month) - (a.year * 12 + a.month);
            } else if (interval == "q") {
                result = (b.year * 4 + (b.month - 1) / 3) -
                         (a.year * 4 + (a.month - 1) / 3);
            } else if (interval == "d" || interval == "y") {
                result = static_cast<std::int64_t>(std::floor(*second) -
                                                   std::floor(*first));
            } else if (interval == "w") {
                result = static_cast<std::int64_t>(std::floor(*second) -
                                                   std::floor(*first)) /
                         7;
            } else if (interval == "ww") {
                std::int64_t first_day = 1;
                if (const auto fd =
                        arguments.size() > 3U ? long_at(3) : std::nullopt) {
                    first_day = *fd == 0 ? 1 : *fd;
                }
                const auto week_start = [first_day](const double v) {
                    return std::floor(v) -
                           static_cast<double>(
                               (split_date(v).weekday - first_day + 7) % 7);
                };
                result = static_cast<std::int64_t>(
                    (week_start(*second) - week_start(*first)) / 7.0);
            } else if (interval == "h") {
                result = static_cast<std::int64_t>(
                    std::floor(*second * 24.0 + 1e-9) -
                    std::floor(*first * 24.0 + 1e-9));
            } else if (interval == "n") {
                result = static_cast<std::int64_t>(
                    std::floor(*second * 1440.0 + 1e-7) -
                    std::floor(*first * 1440.0 + 1e-7));
            } else {
                result =
                    static_cast<std::int64_t>(std::llround(*second * 86400.0) -
                                              std::llround(*first * 86400.0));
            }
            return Value{static_cast<Integer>(result)};
        }
        const auto serial = serial_at(1);
        if (!serial) {
            return mismatch();
        }
        const auto parts = split_date(*serial);
        std::int64_t result{};
        const auto day_of_year =
            static_cast<std::int64_t>(std::floor(*serial) -
                                      date_serial(parts.year, 1, 1)) +
            1;
        if (interval == "yyyy") {
            result = parts.year;
        } else if (interval == "q") {
            result = (parts.month - 1) / 3 + 1;
        } else if (interval == "m") {
            result = parts.month;
        } else if (interval == "y") {
            result = day_of_year;
        } else if (interval == "d") {
            result = parts.day;
        } else if (interval == "w") {
            result = parts.weekday;
        } else if (interval == "ww") {
            std::int64_t first_day = 1;
            std::int64_t first_week = 1;
            if (const auto fd =
                    arguments.size() > 2U ? long_at(2) : std::nullopt) {
                first_day = *fd == 0 ? 1 : *fd;
            }
            if (const auto fw =
                    arguments.size() > 3U ? long_at(3) : std::nullopt) {
                first_week = *fw == 0 ? 1 : *fw;
            }
            if (first_day < 1 || first_day > 7 || first_week < 1 ||
                first_week > 3) {
                set_error("WFC0101", "Invalid procedure call or argument",
                          offset);
                return std::nullopt;
            }
            result = vb_week_of_year(*serial, first_day, first_week);
        } else if (interval == "h") {
            result = parts.hour;
        } else if (interval == "n") {
            result = parts.minute;
        } else {
            result = parts.second;
        }
        return Value{static_cast<Integer>(result)};
    }
    set_error("WFC0071", "unsupported function", offset);
    return std::nullopt;
}

std::uint32_t Interpreter::rnd_step(const std::uint32_t state) noexcept {
    constexpr std::uint32_t multiplier = 0x43FD43FDU;
    constexpr std::uint32_t increment = 0x00C39EC3U;
    constexpr std::uint32_t modulus_mask = 0x00FFFFFFU;  // 2^24 - 1
    return static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(state) * multiplier + increment) &
        modulus_mask);
}

double Interpreter::rnd_value(const std::uint32_t state) noexcept {
    return static_cast<double>(state) / 16777216.0;  // state / 2^24
}

std::uint32_t Interpreter::seed_from_number(const double value) noexcept {
    const auto bits = std::bit_cast<std::uint64_t>(value);
    auto folded = static_cast<std::uint32_t>(bits ^ (bits >> 32U));
    folded ^= folded >> 16U;
    return folded & 0x00FFFFFFU;
}

double Interpreter::entropy_seed() {
    return static_cast<double>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
}

}  // namespace wfc::detail
