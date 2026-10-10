// Interpreter: Module, class, procedure, UDT, and variable declarations.
// Internal to the WFC evaluator; not part of the public API.
// Split out of src/evaluator.cpp; see src/interpreter/README.md.

#include "interpreter.hpp"

namespace wfc::detail {

[[nodiscard]] bool mentions_option_explicit(const std::string_view text) {
    std::size_t position = 0;
    while (position < text.size()) {
        auto end = text.find('\n', position);
        if (end == std::string_view::npos) {
            end = text.size();
        }
        std::string line;
        for (const char c : text.substr(position, end - position)) {
            if (c != ' ' && c != '\t' && c != '\r') {
                line.push_back(ascii_lower(c));
            }
        }
        position = end + 1;
        if (line == "optionexplicit" || line.rfind("optionexplicit'", 0) == 0) {
            return true;
        }
    }
    return false;
}

void Interpreter::scan_option_explicit() {
    strict_declarations_ = mentions_option_explicit(source_);
    for (const auto& module : class_sources_) {
        strict_declarations_ =
            strict_declarations_ || mentions_option_explicit(module.source);
    }
}

bool Interpreter::strict_here() const {
    if (source_.data() != main_source_.data()) {
        for (const auto& [name, definition] : class_definitions_) {
            if (definition.source.data() == source_.data()) {
                return definition.strict;
            }
        }
        return strict_declarations_;
    }
    if (!per_module_explicit_) {
        return strict_declarations_;
    }
    for (const auto& [begin, end] : explicit_ranges_) {
        if (offset_ >= begin && offset_ < end) {
            return true;
        }
    }
    return false;
}

std::optional<std::size_t> Interpreter::default_type_for(
    const std::string& name) const {
    if (name.empty()) {
        return std::nullopt;
    }
    const char c = ascii_lower(name.front());
    if (c < 'a' || c > 'z') {
        return std::nullopt;
    }
    const auto index = default_types_[static_cast<std::size_t>(c - 'a')];
    if (index == no_default_type) {
        return std::nullopt;
    }
    return index;
}

void Interpreter::apply_implicit_return_type(ProcedureDef& definition,
                                             const std::string& name,
                                             const char suffix) const {
    if (suffix != '\0') {
        definition.return_type_index = type_character_index(suffix);
        definition.return_is_variant = false;
    } else if (const auto default_type = default_type_for(name)) {
        definition.return_type_index = *default_type;
        definition.return_is_variant = false;
    } else {
        definition.return_type_index = Value{Empty{}}.index();
        definition.return_is_variant = true;
    }
}

bool Interpreter::apply_deftype_line(const std::string_view line) {
    std::size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) {
        ++i;
    }
    if (line.size() < i + 6U || ascii_lower(line[i]) != 'd' ||
        ascii_lower(line[i + 1U]) != 'e' || ascii_lower(line[i + 2U]) != 'f') {
        return false;
    }
    std::size_t j = i + 3U;
    std::string word;
    while (j < line.size() &&
           std::isalpha(static_cast<unsigned char>(line[j])) != 0) {
        word.push_back(ascii_lower(line[j++]));
    }
    std::size_t type_index = no_default_type;
    if (word == "int") {
        type_index = Value{Int16{}}.index();
    } else if (word == "lng") {
        type_index = Value{Integer{}}.index();
    } else if (word == "sng") {
        type_index = Value{0.0f}.index();
    } else if (word == "dbl") {
        type_index = Value{0.0}.index();
    } else if (word == "cur") {
        type_index = Value{Currency{}}.index();
    } else if (word == "str") {
        type_index = Value{std::string{}}.index();
    } else if (word == "bool") {
        type_index = Value{false}.index();
    } else if (word == "byte") {
        type_index = Value{Byte{}}.index();
    } else if (word == "dec") {
        type_index = Value{Decimal{}}.index();
    } else if (word == "date") {
        type_index = Value{DateValue{}}.index();
    } else if (word != "var" && word != "obj") {
        return false;
    }
    if (j >= line.size() || (line[j] != ' ' && line[j] != '\t')) {
        return false;
    }
    while (j < line.size()) {
        while (j < line.size() &&
               (line[j] == ' ' || line[j] == '\t' || line[j] == ',')) {
            ++j;
        }
        if (j >= line.size() ||
            std::isalpha(static_cast<unsigned char>(line[j])) == 0) {
            break;
        }
        const char first = ascii_lower(line[j++]);
        char last = first;
        std::size_t k = j;
        while (k < line.size() && line[k] == ' ') {
            ++k;
        }
        if (k < line.size() && line[k] == '-') {
            ++k;
            while (k < line.size() && line[k] == ' ') {
                ++k;
            }
            if (k < line.size() &&
                std::isalpha(static_cast<unsigned char>(line[k])) != 0) {
                last = ascii_lower(line[k]);
                j = k + 1U;
            }
        }
        for (char c = first; c <= last; ++c) {
            default_types_[static_cast<std::size_t>(c - 'a')] = type_index;
        }
    }
    return true;
}

void Interpreter::scan_deftypes() {
    const auto scan = [this](const std::string_view text) {
        std::size_t position = 0;
        while (position < text.size()) {
            auto end = text.find('\n', position);
            if (end == std::string_view::npos) {
                end = text.size();
            }
            static_cast<void>(
                apply_deftype_line(text.substr(position, end - position)));
            position = end + 1;
        }
    };
    scan(source_);
    for (const auto& module : class_sources_) {
        scan(module.source);
    }
}

void Interpreter::scan_module_names() {
    const std::string_view text = source_;
    std::size_t position = 0;
    while (position < text.size()) {
        auto end = text.find('\n', position);
        if (end == std::string_view::npos) {
            end = text.size();
        }
        std::string line;
        for (const char c : text.substr(position, end - position)) {
            if (c != '\r') {
                line.push_back(c);
            }
        }
        position = end + 1;
        std::string lowered;
        for (const char c : line) {
            lowered.push_back(ascii_lower(c));
        }
        const auto marker = lowered.find("attribute vb_name");
        if (marker != 0) {
            continue;
        }
        const auto first = line.find('"');
        const auto last = line.rfind('"');
        if (first != std::string::npos && last > first) {
            std::string name;
            for (const char c : line.substr(first + 1, last - first - 1)) {
                name.push_back(ascii_lower(c));
            }
            module_names_.insert(std::move(name));
        }
    }
}

void Interpreter::register_predeclared_instances() {
    auto& module = module_scope();
    for (const auto& cls : class_sources_) {
        bool predeclared = false;
        std::size_t position = 0;
        const std::string_view text = cls.source;
        while (position < text.size() && !predeclared) {
            auto end = text.find('\n', position);
            if (end == std::string_view::npos) {
                end = text.size();
            }
            std::string line;
            for (const char c : text.substr(position, end - position)) {
                if (c != ' ' && c != '\t' && c != '\r') {
                    line.push_back(ascii_lower(c));
                }
            }
            position = end + 1U;
            predeclared = line == "attributevb_predeclaredid=true";
        }
        std::string key;
        for (const char c : cls.name) {
            key.push_back(ascii_lower(c));
        }
        if (!predeclared || !class_definitions_.contains(key) ||
            module.variables.contains(key)) {
            continue;
        }
        module.variables.emplace(key, Value{Nothing{}});
        module.object_variables.insert(key);
        module.object_class_names.emplace(key, key);
        module.auto_new_variables.insert(key);
    }
}

void Interpreter::scan_enum_names() {
    scan_enum_names_in(source_);
    for (const auto& module : class_sources_) {
        scan_enum_names_in(module.source);
    }
}

void Interpreter::scan_enum_names_in(const std::string_view text) {
    std::size_t position = 0;
    while (position < text.size()) {
        auto end = text.find('\n', position);
        if (end == std::string_view::npos) {
            end = text.size();
        }
        std::string line;
        for (const char c : text.substr(position, end - position)) {
            line.push_back(ascii_lower(c));
        }
        position = end + 1;
        std::size_t i = 0;
        const auto skip_space = [&] {
            while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) {
                ++i;
            }
        };
        const auto word = [&](const std::string_view w) {
            if (line.compare(i, w.size(), w) == 0 &&
                (i + w.size() == line.size() ||
                 !is_identifier_part(line[i + w.size()]))) {
                i += w.size();
                return true;
            }
            return false;
        };
        skip_space();
        if (word("public") || word("private")) {
            skip_space();
        }
        if (!word("enum")) {
            continue;
        }
        skip_space();
        std::string name;
        while (i < line.size() && is_identifier_part(line[i])) {
            name.push_back(line[i++]);
        }
        if (!name.empty()) {
            module_names_.insert(name);  // `EnumName.Member`
            enum_names_.push_back(std::move(name));
        }
    }
}

bool Interpreter::scan_procedure_parameters(ProcedureDef& definition) {
    skip_horizontal_whitespace();
    if (!consume('(')) {
        set_error("WFC0005",
                  "expected opening parenthesis after procedure name", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    if (consume(')')) {
        return true;
    }
    // REQ-0206: once an Optional parameter has been seen, every
    // parameter after it must itself be Optional or the trailing
    // ParamArray -- a required parameter cannot follow.
    bool seen_optional = false;
    while (true) {
        skip_horizontal_whitespace();
        const auto modifier_offset = offset_;
        ProcedureParameter parameter;
        bool is_param_array = false;
        bool is_optional = false;
        if (consume_keyword("paramarray")) {
            is_param_array = true;
            skip_horizontal_whitespace();
        } else if (consume_keyword("optional")) {
            is_optional = true;
            skip_horizontal_whitespace();
        }
        if (is_param_array) {
            // ParamArray is always effectively ByVal: each call builds
            // it a fresh array, so there is no caller variable for a
            // ByRef write-back to reach.
            parameter.by_val = true;
        } else if (consume_keyword("byval")) {
            parameter.by_val = true;
            skip_horizontal_whitespace();
        } else if (consume_keyword("byref")) {
            skip_horizontal_whitespace();
        }
        if (!is_optional && !is_param_array && seen_optional) {
            set_error(
                "WFC0140",
                "a required parameter cannot follow an Optional parameter",
                modifier_offset);
            return false;
        }
        const auto parameter_name_offset = offset_;
        char type_character{};
        auto name = parse_identifier(&type_character);
        if (!name.has_value()) {
            set_error("WFC0011", "expected parameter name",
                      parameter_name_offset);
            return false;
        }
        if (is_reserved_identifier(*name)) {
            set_error("WFC0017", "reserved keyword cannot be a parameter name",
                      parameter_name_offset);
            return false;
        }
        parameter.name = std::move(*name);
        skip_horizontal_whitespace();
        if (is_param_array) {
            if (type_character != '\0' || !consume('(')) {
                set_error(
                    "WFC0141",
                    "ParamArray parameter must be declared as an array: name()",
                    offset_);
                return false;
            }
            skip_horizontal_whitespace();
            if (!consume(')')) {
                set_error(
                    "WFC0141",
                    "ParamArray parameter must be declared as an array: name()",
                    offset_);
                return false;
            }
            skip_horizontal_whitespace();
            const auto element_type_offset = offset_;
            const auto matched_as = consume_keyword("as");
            if (matched_as) {
                skip_horizontal_whitespace();
            }
            const auto type_result =
                matched_as ? parse_type_keyword() : std::nullopt;
            if (matched_as && !type_result.has_value()) {
                set_error(
                    "WFC0141",
                    "ParamArray requires an element type: As Integer, As Long, "
                    "As Double, As Single, As Currency, As String, As Boolean, "
                    "or As Variant",
                    element_type_offset);
                return false;
            }
            if (!matched_as || type_result->is_variant) {
                // REQ-0248: an untyped / `As Variant` ParamArray is a
                // Variant array.
                parameter.is_variant = true;
                parameter.type_index = Value{Empty{}}.index();
            } else {
                parameter.type_index = type_result->default_value.index();
            }
            parameter.is_param_array = true;
        } else if (type_character == '\0' && !at_end() && current() == '(') {
            // A plain (non-ParamArray) array parameter, `name() As
            // Type` (REQ-0211): always effectively ByRef -- an array is
            // a reference-like value in real VB6, and this evaluator
            // has no other way to alias the caller's array -- so an
            // explicit `ByVal` here is rejected rather than silently
            // ignored.
            advance();
            skip_horizontal_whitespace();
            if (!consume(')')) {
                set_error(
                    "WFC0149",
                    "array parameter must be declared as an array: name()",
                    offset_);
                return false;
            }
            skip_horizontal_whitespace();
            const auto element_type_offset = offset_;
            const auto matched_as = consume_keyword("as");
            if (matched_as) {
                skip_horizontal_whitespace();
            }
            // REQ-0215: `name() As Object` is not one of
            // parse_type_keyword's own scalars (it only knows the
            // eight `Dim`-style keywords, `Variant` included), so it
            // is checked separately first, mirroring the same
            // separate check a generic `As Object` scalar parameter
            // already uses.
            bool matched_object = matched_as && consume_keyword("object");
            std::string array_class_name;
            if (matched_as && !matched_object) {
                // REQ-0267: `name() As ClassOrUdt`.
                const auto class_probe = offset_;
                char class_type_character{};
                auto class_name = parse_identifier(&class_type_character);
                if (class_name.has_value() && class_type_character == '\0' &&
                    class_definitions_.contains(*class_name)) {
                    matched_object = true;
                    array_class_name = std::move(*class_name);
                } else {
                    offset_ = class_probe;
                }
            }
            const auto type_result = (matched_as && !matched_object)
                                         ? parse_type_keyword()
                                         : std::nullopt;
            if (!matched_as || (!matched_object && !type_result.has_value())) {
                set_error("WFC0149",
                          "array parameter requires an explicit element type: "
                          "As Integer, As "
                          "Long, As Double, As Single, As Currency, As String, "
                          "As Boolean, As "
                          "Object, or As Variant",
                          element_type_offset);
                return false;
            }
            if (parameter.by_val) {
                set_error("WFC0149", "array parameters must be passed ByRef",
                          modifier_offset);
                return false;
            }
            if (is_optional) {
                set_error("WFC0149", "array parameters cannot be Optional",
                          modifier_offset);
                return false;
            }
            parameter.is_array_parameter = true;
            if (matched_object) {
                parameter.is_object_array_parameter = true;
                parameter.class_name = array_class_name;
            } else if (type_result->is_variant) {
                parameter.is_variant_array_parameter = true;
            } else {
                parameter.type_index = type_result->default_value.index();
            }
        } else if (type_character != '\0') {
            if (!validate_type_character(type_character,
                                         parameter_name_offset)) {
                return false;
            }
            parameter.type_index = type_character_index(type_character);
        } else if (consume_keyword("as")) {
            skip_horizontal_whitespace();
            const auto type_offset = offset_;
            // REQ-0228: `As Object`/`As SomeClassName` are now accepted
            // for any Sub/Function/Property parameter (not just
            // Property Set's own single value parameter, previously
            // the only caller that allowed `As Object` here), via the
            // same resolver a class-typed field/return type already
            // uses.
            const auto type_result = parse_scalar_object_or_class_type();
            if (!type_result.has_value()) {
                set_error("WFC0012",
                          "expected As Integer, As Long, As Double, As Single, "
                          "As Currency, As "
                          "String, As Boolean, As Object, or As Variant",
                          type_offset);
                return false;
            }
            if (type_result->is_object) {
                parameter.is_object_reference = true;
                parameter.type_index = type_result->type_index;
                parameter.class_name = std::move(type_result->class_name);
            } else {
                parameter.type_index = type_result->type_index;
                parameter.is_variant = type_result->is_variant;
            }
        } else {
            // A bare parameter with no As clause and no type character
            // is implicitly Variant, matching real VB6.
            if (const auto default_type = default_type_for(parameter.name)) {
                parameter.type_index = *default_type;
            } else {
                parameter.type_index = Value{Empty{}}.index();
                parameter.is_variant = true;
            }
        }

        if (is_optional) {
            parameter.is_optional = true;
            seen_optional = true;
            skip_horizontal_whitespace();
            if (consume('=')) {
                skip_horizontal_whitespace();
                const auto default_offset = offset_;
                const bool enclosing_constant_expression = constant_expression_;
                constant_expression_ = true;
                auto default_value = parse_expression();
                constant_expression_ = enclosing_constant_expression;
                if (!default_value.has_value()) {
                    return false;
                }
                if (!parameter.is_variant) {
                    if (!coerce_numeric_value(*default_value,
                                              parameter.type_index,
                                              default_offset)) {
                        return false;
                    }
                    if (default_value->index() != parameter.type_index) {
                        set_error("WFC0016", "default value type mismatch",
                                  default_offset);
                        return false;
                    }
                }
                parameter.has_default = true;
                parameter.default_value = std::move(*default_value);
            }
        }

        definition.parameters.push_back(std::move(parameter));
        skip_horizontal_whitespace();
        if (consume(')')) {
            break;
        }
        if (is_param_array) {
            set_error("WFC0141", "ParamArray must be the last parameter",
                      offset_);
            return false;
        }
        if (!consume(',')) {
            set_error("WFC0005", "expected closing parenthesis", offset_);
            return false;
        }
    }
    return true;
}

bool Interpreter::class_member_name_used(const ClassDef& class_def,
                                         const std::string& name) {
    return class_def.fields.contains(name) ||
           class_def.methods.contains(name) ||
           class_def.property_get.contains(name) ||
           class_def.property_let.contains(name) ||
           class_def.property_set.contains(name);
}

bool Interpreter::scan_class_body(ClassDef& class_def) {
    while (true) {
        skip_program_leading_trivia();
        if (at_end()) {
            return true;
        }
        const auto line_offset = offset_;

        if (consume_keyword("implements")) {
            skip_horizontal_whitespace();
            const auto interface_name_offset = offset_;
            char interface_type_character{};
            auto interface_name = parse_identifier(&interface_type_character);
            if (!interface_name.has_value() ||
                interface_type_character != '\0') {
                set_error("WFC0011", "expected interface name after Implements",
                          interface_name_offset);
                return false;
            }
            if (!class_definitions_.contains(*interface_name)) {
                set_error("WFC0134", "unknown class name",
                          interface_name_offset);
                return false;
            }
            class_def.implements.push_back(std::move(*interface_name));
            continue;
        }

        if (consume_keyword("dim")) {
            if (!scan_class_field_declaration(class_def, /*is_private=*/true)) {
                return false;
            }
            continue;
        }

        {
            // `[Private|Public] Type ... End Type` in a class module: the
            // type itself was registered up front; skip its block here.
            const auto before_type = offset_;
            static_cast<void>(consume_keyword("private") ||
                              consume_keyword("public"));
            skip_horizontal_whitespace();
            if (consume_keyword("type")) {
                while (!at_end()) {
                    skip_rest_of_line();
                    skip_program_leading_trivia();
                    const auto line_start = offset_;
                    if (consume_keyword("end")) {
                        skip_horizontal_whitespace();
                        if (consume_keyword("type")) {
                            break;
                        }
                    }
                    offset_ = line_start;
                }
                skip_rest_of_line();
                continue;
            }
            offset_ = before_type;
        }
        if (consume_keyword("option") || consume_keyword("attribute")) {
            // `Option Explicit` (and friends) and file-level `Attribute`
            // lines at the top of a class module file; accepted and ignored.
            skip_rest_of_line();
            continue;
        }
        {
            // REQ-0260: class-level `[Public|Private|Friend] Const|Enum`.
            const auto before_constant = offset_;
            const bool constant_is_private = consume_keyword("private");
            if (!constant_is_private) {
                static_cast<void>(consume_keyword("public") ||
                                  consume_keyword("friend"));
            }
            skip_horizontal_whitespace();
            const bool is_const = consume_keyword("const");
            const bool is_enum = !is_const && consume_keyword("enum");
            if (is_const || is_enum) {
                scopes_.emplace_back();
                const bool parsed_ok = is_const
                                           ? parse_constant_declaration()
                                           : parse_enum_statement(line_offset);
                Scope captured = std::move(scopes_.back());
                scopes_.pop_back();
                if (!parsed_ok) {
                    return false;
                }
                for (auto& [constant_name, constant_value] :
                     captured.variables) {
                    // A Public Enum/Const in a class module is visible
                    // program-wide.
                    if (!constant_is_private) {
                        global_class_constants_.emplace(constant_name,
                                                        constant_value);
                        module_names_.insert(
                            current_class_scan_name_);  // `Cls.Member`
                    }
                    class_def.constants[constant_name] =
                        std::move(constant_value);
                }
                if (!consume_statement_end() && !is_enum) {
                    return false;
                }
                continue;
            }
            offset_ = before_constant;
        }
        bool is_private = false;
        bool has_visibility_keyword = false;
        if (consume_keyword("private")) {
            is_private = true;
            has_visibility_keyword = true;
            skip_horizontal_whitespace();
        } else if (consume_keyword("public") || consume_keyword("friend")) {
            has_visibility_keyword = true;
            skip_horizontal_whitespace();
        }
        if (consume_keyword("static")) {
            pending_static_procedure_ = true;
            skip_horizontal_whitespace();
        }

        if (consume_keyword("declare")) {
            // `Private Declare Function|Sub Name Lib "dll" [Alias "x"] ...`
            // in a class module: an external routine local to the class.
            skip_horizontal_whitespace();
            static_cast<void>(consume_keyword("ptrsafe"));
            skip_horizontal_whitespace();
            const bool declared_function = consume_keyword("function");
            if (!declared_function && !consume_keyword("sub")) {
                set_error("WFC0127", "expected Function or Sub after Declare",
                          offset_);
                return false;
            }
            skip_horizontal_whitespace();
            char declare_type_character{};
            auto declared = parse_identifier(&declare_type_character);
            if (!declared.has_value() ||
                class_member_name_used(class_def, *declared)) {
                set_error("WFC0128", "duplicate or reserved class member name",
                          line_offset);
                return false;
            }
            ProcedureDef external;
            external.is_function = declared_function;
            external.is_external = true;
            external.is_private = is_private;
            external.return_type_index = Value{Integer{}}.index();
            external.return_is_variant = true;
            const auto line_end =
                std::min(source_.find('\n', offset_), source_.size());
            std::string rest;
            for (std::size_t k = offset_; k < line_end; ++k) {
                rest.push_back(ascii_lower(source_[k]));
            }
            if (const auto alias_at = rest.find(" alias ");
                alias_at != std::string::npos) {
                const auto open = rest.find('"', alias_at);
                const auto close = open == std::string::npos
                                       ? std::string::npos
                                       : rest.find('"', open + 1U);
                if (close != std::string::npos) {
                    external.external_name =
                        rest.substr(open + 1U, close - open - 1U);
                }
            }
            skip_rest_of_line();
            class_def.methods.emplace(std::move(*declared),
                                      std::move(external));
            continue;
        }

        if (consume_keyword("event")) {
            skip_horizontal_whitespace();
            const auto event_name_offset = offset_;
            char event_type_character{};
            auto event_name = parse_identifier(&event_type_character);
            if (!event_name.has_value() || event_type_character != '\0') {
                set_error("WFC0011", "expected event name", event_name_offset);
                return false;
            }
            if (class_def.events.contains(*event_name) ||
                class_member_name_used(class_def, *event_name)) {
                set_error("WFC0128", "duplicate or reserved class member name",
                          event_name_offset);
                return false;
            }
            ProcedureDef definition;
            if (!scan_procedure_parameters(definition) ||
                !consume_statement_end()) {
                return false;
            }
            class_def.events.emplace(std::move(*event_name),
                                     std::move(definition));
            continue;
        }
        if (consume_keyword("property")) {
            if (!scan_class_property_declaration(class_def, is_private,
                                                 line_offset)) {
                return false;
            }
            continue;
        }
        bool is_function = false;
        bool matched_sub_or_function = true;
        if (consume_keyword("sub")) {
            is_function = false;
        } else if (consume_keyword("function")) {
            is_function = true;
        } else {
            matched_sub_or_function = false;
        }
        if (matched_sub_or_function) {
            if (!scan_class_procedure_declaration(class_def, is_function,
                                                  is_private, line_offset)) {
                return false;
            }
            continue;
        }
        if (has_visibility_keyword) {
            if (!scan_class_field_declaration(class_def, is_private)) {
                return false;
            }
            continue;
        }
        set_error("WFC0127",
                  "expected a class member declaration (Dim, Public, Private, "
                  "Sub, Function, "
                  "or Property)",
                  line_offset);
        return false;
    }
}

bool Interpreter::scan_class_field_declaration(ClassDef& class_def,
                                               const bool is_private) {
    bool more = true;
    while (more) {
        if (!scan_class_field_declarator(class_def, is_private, more)) {
            return false;
        }
    }
    return true;
}

bool Interpreter::scan_class_field_declarator(ClassDef& class_def,
                                              const bool is_private,
                                              bool& more) {
    skip_horizontal_whitespace();
    const bool with_events = consume_keyword("withevents");
    skip_horizontal_whitespace();
    const auto name_offset = offset_;
    char type_character{};
    auto name = parse_identifier(&type_character);
    if (!name.has_value() || type_character != '\0') {
        set_error("WFC0011", "expected field name", name_offset);
        return false;
    }
    if (is_reserved_member_name(*name) ||
        class_member_name_used(class_def, *name)) {
        set_error("WFC0128", "duplicate or reserved class member name",
                  name_offset);
        return false;
    }
    skip_horizontal_whitespace();
    ClassFieldDef field;
    field.is_private = is_private;
    field.with_events = with_events;
    if (!at_end() && current() == '(') {
        advance();
        skip_horizontal_whitespace();
        field.is_array = true;
        if (!at_end() && current() == ')') {
            advance();
        } else {
            auto bounds = parse_fixed_array_bounds(name_offset);
            if (!bounds.has_value()) {
                return false;
            }
            field.dimensions = std::move(*bounds);
        }
        skip_horizontal_whitespace();
    }
    if (consume_keyword("as")) {
        skip_horizontal_whitespace();
        field.auto_new = consume_keyword("new");
        skip_horizontal_whitespace();
        const auto type_offset = offset_;
        const auto type_result = parse_scalar_object_or_class_type();
        if (!type_result.has_value() ||
            (field.auto_new && !type_result->is_object)) {
            set_error("WFC0012",
                      "expected As Integer, As Long, As Double, As Single, As "
                      "Currency, "
                      "As String, As Boolean, As Object, As Variant, or a "
                      "known class "
                      "name",
                      type_offset);
            return false;
        }
        field.type_index = type_result->type_index;
        field.is_variant = type_result->is_variant;
        field.is_object = type_result->is_object;
        field.class_name = type_result->class_name;
        skip_horizontal_whitespace();
        if (!field.is_array && !field.is_variant && !field.is_object &&
            field.type_index == Value{std::string{}}.index() && consume('*')) {
            skip_horizontal_whitespace();
            std::size_t length = 0;
            std::size_t digits = 0;
            while (!at_end() &&
                   std::isdigit(static_cast<unsigned char>(current())) != 0) {
                length =
                    length * 10U + static_cast<std::size_t>(current() - '0');
                advance();
                ++digits;
            }
            if (digits == 0 || length == 0 || length > 65535U) {
                set_error("WFC0012", "expected a fixed string length", offset_);
                return false;
            }
            field.fixed_length = length;
        }
    } else {
        // A bare field declaration with no As clause is implicitly
        // Variant, matching a bare module-level Dim.
        field.type_index = Value{Empty{}}.index();
        field.is_variant = true;
    }
    // A field declaration is a plain statement line, not a block
    // opener -- unlike Sub/Function/Property (which always need a body
    // to follow, so consume_block_line_end's "a line break is
    // mandatory" requirement is right for them), a class's last field
    // can legally be its source's very last line with no trailing line
    // break.
    skip_horizontal_whitespace();
    more = consume(',');
    if (!more && !consume_statement_end()) {
        return false;
    }
    class_def.field_order.push_back(*name);
    class_def.fields.emplace(std::move(*name), std::move(field));
    return true;
}

bool Interpreter::scan_class_property_declaration(
    ClassDef& class_def, const bool is_private, const std::size_t line_offset) {
    skip_horizontal_whitespace();
    enum class Accessor { get, let, set };
    Accessor accessor;
    if (consume_keyword("get")) {
        accessor = Accessor::get;
    } else if (consume_keyword("let")) {
        accessor = Accessor::let;
    } else if (consume_keyword("set")) {
        accessor = Accessor::set;
    } else {
        set_error("WFC0129", "expected Get, Let, or Set after Property",
                  offset_);
        return false;
    }
    skip_horizontal_whitespace();
    const auto name_offset = offset_;
    char type_character{};
    auto name = parse_identifier(&type_character);
    if (!name.has_value() || type_character != '\0') {
        set_error("WFC0118", "expected property name", name_offset);
        return false;
    }
    if (is_reserved_member_name(*name) || class_def.fields.contains(*name) ||
        class_def.methods.contains(*name)) {
        set_error("WFC0128", "duplicate or reserved class member name",
                  name_offset);
        return false;
    }
    auto& accessor_table = accessor == Accessor::get   ? class_def.property_get
                           : accessor == Accessor::let ? class_def.property_let
                                                       : class_def.property_set;
    if (accessor_table.contains(*name)) {
        set_error("WFC0128", "duplicate Property accessor for this name",
                  name_offset);
        return false;
    }

    ProcedureDef definition;
    definition.is_function = accessor == Accessor::get;
    definition.is_private = is_private;
    definition.static_locals = std::exchange(pending_static_procedure_, false);
    if (!scan_procedure_parameters(definition)) {
        return false;
    }
    if (accessor == Accessor::get) {
        // Property Get may take zero or more index parameters
        // (REQ-0205's indexed properties -- `Property Get
        // Name(index [, ...]) As Type`, accessed as
        // `obj.Name(args)` exactly like a method call); zero
        // parameters is the plain, non-indexed form accessed as
        // `obj.Name` with no parentheses at all.
        skip_horizontal_whitespace();
        if (!consume_keyword("as")) {
            apply_implicit_return_type(definition, *name);
        } else {
            const auto type_offset = offset_;
            const auto type_result = parse_scalar_object_or_class_type();
            if (!type_result.has_value()) {
                set_error("WFC0012",
                          "expected As Integer, As Long, As Double, As Single, "
                          "As Currency, "
                          "As String, As Boolean, As Object, As Variant, or a "
                          "known class "
                          "name",
                          type_offset);
                return false;
            }
            definition.return_type_index = type_result->type_index;
            definition.return_is_variant = type_result->is_variant;
            definition.return_is_object = type_result->is_object;
            definition.return_class_name = type_result->class_name;
            const auto array_marker =
                parse_function_array_return_marker(*type_result, type_offset);
            if (!array_marker.has_value()) {
                return false;
            }
            definition.return_is_array = *array_marker;
        }
    } else {
        // Property Let/Set's last parameter is always the value
        // being assigned; any parameters before it are index
        // parameters (REQ-0205), so at least one parameter (the
        // value alone, for the plain non-indexed form) is
        // required, but there is no upper bound.
        if (definition.parameters.empty()) {
            set_error("WFC0131",
                      "Property Let/Set requires at least one parameter",
                      name_offset);
            return false;
        }
        if (accessor == Accessor::set &&
            !definition.parameters.back().is_object_reference &&
            !definition.parameters.back().is_variant) {
            set_error(
                "WFC0132",
                "Property Set's last parameter (the value) must be declared As "
                "Object",
                name_offset);
            return false;
        }
    }
    if (!consume_loop_header_end()) {
        return false;
    }
    definition.body_start = offset_;
    if (!skip_to_matching_end("property", definition.body_end)) {
        set_error("WFC0133", "expected End Property", line_offset);
        return false;
    }
    definition.declaration_end = offset_;
    accessor_table.emplace(std::move(*name), std::move(definition));
    return true;
}

bool Interpreter::scan_class_procedure_declaration(
    ClassDef& class_def, const bool is_function, const bool is_private,
    const std::size_t line_offset) {
    skip_horizontal_whitespace();
    const auto name_offset = offset_;
    char type_character{};
    auto name = parse_identifier(&type_character);
    if (!name.has_value() || (type_character != '\0' && !is_function)) {
        set_error("WFC0118", "expected procedure name", name_offset);
        return false;
    }
    if (is_reserved_member_name(*name) ||
        class_member_name_used(class_def, *name)) {
        set_error("WFC0128", "duplicate or reserved class member name",
                  name_offset);
        return false;
    }
    ProcedureDef definition;
    definition.is_function = is_function;
    definition.is_private = is_private;
    definition.static_locals = std::exchange(pending_static_procedure_, false);
    if (!scan_procedure_parameters(definition)) {
        return false;
    }
    if (is_function) {
        skip_horizontal_whitespace();
        if (!consume_keyword("as")) {
            apply_implicit_return_type(definition, *name, type_character);
        } else {
            const auto type_offset = offset_;
            const auto type_result = parse_scalar_object_or_class_type();
            if (!type_result.has_value()) {
                set_error("WFC0012",
                          "expected As Integer, As Long, As Double, As Single, "
                          "As Currency, "
                          "As String, As Boolean, As Object, As Variant, or a "
                          "known class name",
                          type_offset);
                return false;
            }
            definition.return_type_index = type_result->type_index;
            definition.return_is_variant = type_result->is_variant;
            definition.return_is_object = type_result->is_object;
            definition.return_class_name = type_result->class_name;
            const auto array_marker =
                parse_function_array_return_marker(*type_result, type_offset);
            if (!array_marker.has_value()) {
                return false;
            }
            definition.return_is_array = *array_marker;
        }
    }
    if (!consume_loop_header_end()) {
        return false;
    }
    definition.body_start = offset_;
    if (!skip_to_matching_end(is_function ? "function" : "sub",
                              definition.body_end)) {
        set_error(is_function ? "WFC0120" : "WFC0121",
                  is_function ? "expected End Function" : "expected End Sub",
                  line_offset);
        return false;
    }
    definition.declaration_end = offset_;
    // Class_Initialize/Class_Terminate are the lifecycle hooks New
    // (instantiate_class) and drain_scope_instances/
    // terminate_if_last_reference invoke automatically -- both
    // always call with zero arguments, so a declaration with
    // parameters (or written as a Function) could never actually
    // run correctly and is rejected up front instead.
    if ((*name == "class_initialize" || *name == "class_terminate") &&
        (definition.is_function || !definition.parameters.empty())) {
        set_error(
            "WFC0139",
            "Class_Initialize/Class_Terminate must be a parameterless Sub",
            name_offset);
        return false;
    }
    class_def.methods.emplace(std::move(*name), std::move(definition));
    return true;
}

Value Interpreter::copy_if_udt(Value value) {
    if (const auto* instance = std::get_if<ObjectInstance>(&value)) {
        if (is_udt_class(instance->data->class_name)) {
            return Value{ObjectInstance{clone_udt(*instance->data)}};
        }
    } else if (std::holds_alternative<ArrayValue>(value)) {
        deep_copy_udt_values(value);  // an array of UDTs copies its elements
    }
    return value;
}

bool Interpreter::is_udt_class(const std::string& class_name) const {
    const auto found = class_definitions_.find(class_name);
    return found != class_definitions_.end() && found->second.is_udt;
}

std::shared_ptr<InstanceData> Interpreter::clone_udt(
    const InstanceData& source) {
    auto copy = std::make_shared<InstanceData>();
    copy->class_name = source.class_name;
    copy->fields = source.fields;
    for (auto& [name, value] : copy->fields.variables) {
        deep_copy_udt_values(value);
    }
    return copy;
}

void Interpreter::deep_copy_udt_values(Value& value) {
    if (const auto* nested = std::get_if<ObjectInstance>(&value)) {
        if (is_udt_class(nested->data->class_name)) {
            value = Value{ObjectInstance{clone_udt(*nested->data)}};
        }
    } else if (auto* array = std::get_if<ArrayValue>(&value)) {
        for (auto& element : array->elements) {
            deep_copy_udt_values(element);
        }
    }
}

bool Interpreter::assign_udt(Value& target, const Value& source,
                             const std::size_t offset) {
    if (!execute_) {
        return true;  // dry run: operands are placeholders
    }
    const auto* destination = std::get_if<ObjectInstance>(&target);
    const auto* origin = std::get_if<ObjectInstance>(&source);
    if (destination == nullptr || origin == nullptr ||
        destination->data->class_name != origin->data->class_name) {
        set_error("WFC0016", "assignment type mismatch", offset);
        return false;
    }
    if (execute_ && destination->data != origin->data) {
        destination->data->fields = clone_udt(*origin->data)->fields;
    }
    return true;
}

void Interpreter::scan_udt_types() {
    scan_udt_types_in(source_);
    // A class module may declare its own (private) types too.
    const auto class_count = class_sources_.size();
    for (std::size_t index = 0; index < class_count; ++index) {
        const std::string_view class_text = class_sources_[index].source;
        scan_udt_types_in(class_text);
    }
}

void Interpreter::scan_udt_types_in(const std::string_view text) {
    std::size_t position = 0;
    std::string current_name;
    std::string body;
    while (position < text.size()) {
        auto end = text.find('\n', position);
        if (end == std::string_view::npos) {
            end = text.size();
        }
        std::string line;
        for (const char c : text.substr(position, end - position)) {
            if (c == '\'') {
                break;
            }
            if (c != '\r') {
                line.push_back(c);
            }
        }
        position = end + 1;
        std::size_t i = 0;
        const auto skip_space = [&] {
            while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) {
                ++i;
            }
        };
        const auto word = [&](const std::string_view w) {
            if (line.size() - i < w.size()) {
                return false;
            }
            for (std::size_t k = 0; k < w.size(); ++k) {
                if (ascii_lower(line[i + k]) != w[k]) {
                    return false;
                }
            }
            if (i + w.size() < line.size() &&
                is_identifier_part(line[i + w.size()])) {
                return false;
            }
            i += w.size();
            return true;
        };
        skip_space();
        if (current_name.empty()) {
            if (word("public") || word("private")) {
                skip_space();
            }
            if (!word("type")) {
                continue;
            }
            skip_space();
            while (i < line.size() && is_identifier_part(line[i])) {
                current_name.push_back(line[i++]);
            }
            body.clear();
            continue;
        }
        if (word("end")) {
            skip_space();
            if (word("type")) {
                const bool known =
                    std::find(udt_names_.begin(), udt_names_.end(),
                              current_name) != udt_names_.end();
                if (!known) {
                    udt_sources_.push_back(std::move(body));
                    class_sources_.push_back(
                        {current_name, udt_sources_.back()});
                    udt_names_.push_back(current_name);
                }
                body.clear();
                current_name.clear();
                continue;
            }
        }
        if (i >= line.size()) {
            continue;
        }
        body += "Public " + line.substr(i) + "\n";
    }
}

void Interpreter::scan_builtin_classes() {
    const auto mentions = [](const std::string_view text) {
        constexpr std::string_view word = "collection";
        for (std::size_t i = 0; i + word.size() <= text.size(); ++i) {
            std::size_t k = 0;
            while (k < word.size() && ascii_lower(text[i + k]) == word[k]) {
                ++k;
            }
            if (k == word.size() &&
                (i == 0 || !is_identifier_part(text[i - 1])) &&
                (i + k == text.size() || !is_identifier_part(text[i + k]))) {
                return true;
            }
        }
        return false;
    };
    bool needed = mentions(source_);
    for (const auto& module : class_sources_) {
        if (module.name.size() == 10 && mentions(module.name)) {
            return;  // user-defined Collection
        }
        needed = needed || mentions(module.source);
    }
    if (needed) {
        class_sources_.push_back({"Collection", kCollectionSource});
    }
    const auto text_mentions = [](const std::string_view text,
                                  const std::string_view needle,
                                  const bool whole_word) {
        for (std::size_t i = 0; i + needle.size() <= text.size(); ++i) {
            std::size_t k = 0;
            while (k < needle.size() && ascii_lower(text[i + k]) == needle[k]) {
                ++k;
            }
            if (k == needle.size() &&
                (!whole_word || ((i == 0 || !is_identifier_part(text[i - 1])) &&
                                 (i + k == text.size() ||
                                  !is_identifier_part(text[i + k]))))) {
                return true;
            }
        }
        return false;
    };
    const auto any_source_mentions = [&](const std::string_view needle,
                                         const bool whole_word) {
        bool found = text_mentions(source_, needle, whole_word);
        for (const auto& module : class_sources_) {
            found = found || text_mentions(module.source, needle, whole_word);
        }
        return found;
    };
    const auto user_defines = [&](const std::string_view name) {
        for (const auto& module : class_sources_) {
            if (module.name.size() == name.size()) {
                bool same = true;
                for (std::size_t i = 0; i < name.size(); ++i) {
                    same = same && ascii_lower(module.name[i]) == name[i];
                }
                if (same) {
                    return true;
                }
            }
        }
        return false;
    };
    // Each built-in is registered under an internal `Wfc` name (what
    // CreateObject builds) and, unless the program defines its own class of
    // that name, the public type name too
    // (`Dim d As New Dictionary`, `As Scripting.Dictionary`).
    if (any_source_mentions("scripting.dictionary", false) ||
        any_source_mentions("dictionary", true)) {
        const bool alias = !user_defines("dictionary");
        class_sources_.push_back({"WfcDictionary", kDictionarySource});
        if (alias) {
            class_sources_.push_back({"Dictionary", kDictionarySource});
        }
    }
    if (any_source_mentions("scripting.filesystemobject", false) ||
        any_source_mentions("filesystemobject", true)) {
        const bool alias = !user_defines("filesystemobject");
        class_sources_.push_back({"WfcTextStream", kTextStreamSource});
        class_sources_.push_back({"WfcFile", kFileObjectSource});
        class_sources_.push_back({"WfcFolder", kFolderObjectSource});
        if (!user_defines("collection")) {
            // Folder.Files / Folder.SubFolders return Collections.
            class_sources_.push_back({"Collection", kCollectionSource});
        }
        class_sources_.push_back(
            {"WfcFileSystemObject", kFileSystemObjectSource});
        if (alias) {
            class_sources_.push_back(
                {"FileSystemObject", kFileSystemObjectSource});
        }
        const auto add_alias = [&](const std::string_view type_name,
                                   const std::string_view lowered_type,
                                   const std::string_view source) {
            if (!user_defines(lowered_type) &&
                (any_source_mentions(
                     std::string("as ") + std::string(lowered_type), true) ||
                 any_source_mentions(
                     std::string("scripting.") + std::string(lowered_type),
                     true))) {
                class_sources_.push_back({std::string(type_name), source});
            }
        };
        add_alias("Folder", "folder", kFolderObjectSource);
        add_alias("File", "file", kFileObjectSource);
        add_alias("TextStream", "textstream", kTextStreamSource);
    }
    if (any_source_mentions("vbscript.regexp", false) ||
        any_source_mentions("vbscript_regexp_55.regexp", false) ||
        any_source_mentions("regexp", true)) {
        const bool alias = !user_defines("regexp");
        class_sources_.push_back({"WfcRegExp", kRegExpSource});
        class_sources_.push_back(
            {"WfcMatchCollection", kMatchCollectionSource});
        class_sources_.push_back({"WfcMatch", kMatchSource});
        class_sources_.push_back({"WfcSubMatches", kSubMatchesSource});
        if (alias) {
            class_sources_.push_back({"RegExp", kRegExpSource});
        }
    }
    const auto mentions_app = [](const std::string_view text) {
        for (std::size_t i = 0; i + 4U <= text.size(); ++i) {
            if (ascii_lower(text[i]) == 'a' &&
                ascii_lower(text[i + 1U]) == 'p' &&
                ascii_lower(text[i + 2U]) == 'p' && text[i + 3U] == '.' &&
                (i == 0 || !is_identifier_part(text[i - 1U]))) {
                return true;
            }
        }
        return false;
    };
    bool needs_app = mentions_app(source_);
    for (const auto& module : class_sources_) {
        needs_app = needs_app || mentions_app(module.source);
    }
    const auto mentions_object = [](const std::string_view text,
                                    const std::string_view word) {
        for (std::size_t i = 0; i + word.size() < text.size(); ++i) {
            if (text[i + word.size()] != '.' ||
                (i != 0 && is_identifier_part(text[i - 1U]))) {
                continue;
            }
            bool same = true;
            for (std::size_t k = 0; same && k < word.size(); ++k) {
                same = ascii_lower(text[i + k]) == word[k];
            }
            if (same) {
                return true;
            }
        }
        return false;
    };
    const auto needs_object = [&](const std::string_view word) {
        bool needed = mentions_object(source_, word);
        for (const auto& module : class_sources_) {
            needed = needed || mentions_object(module.source, word);
        }
        return needed;
    };
    if (needs_object("clipboard")) {
        class_sources_.push_back({"WfcClipboard", kClipboardSource});
    }
    if (needs_object("screen")) {
        class_sources_.push_back({"WfcScreen", kScreenSource});
    }
    if (needs_app) {
        // Project metadata replaces the default value of each `App` property.
        std::string app_source(kAppSource);
        for (const auto& [property, value] : app_properties_) {
            const bool numeric = property == "major" || property == "minor" ||
                                 property == "revision";
            std::string literal;
            if (numeric) {
                literal = std::to_string(std::atol(value.c_str()));
            } else {
                literal = "\"";
                for (const char c : value) {
                    literal +=
                        c == '"' ? std::string("\"\"") : std::string(1U, c);
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
                : property == "legaltrademarks" ? "LegalTrademarks = \"\""
                                                : "LegalCopyright = \"\"";
            const auto at = app_source.find(marker);
            if (at != std::string::npos) {
                app_source.replace(
                    at, marker.size(),
                    marker.substr(0, marker.find('=') + 2U) + literal);
            }
        }
        app_source_storage_ = std::move(app_source);
        class_sources_.push_back({"WfcApp", app_source_storage_});
    }
}

void Interpreter::scan_default_member(ClassDef& class_def) {
    const std::string_view text = class_def.source;
    std::size_t position = 0;
    while (position < text.size()) {
        auto end = text.find('\n', position);
        if (end == std::string_view::npos) {
            end = text.size();
        }
        std::string line;
        for (const char c : text.substr(position, end - position)) {
            if (c != ' ' && c != '\t' && c != '\r') {
                line.push_back(ascii_lower(c));
            }
        }
        position = end + 1;
        constexpr std::string_view prefix = "attribute";
        constexpr std::string_view suffix = ".vb_usermemid=0";
        if (line.size() > prefix.size() + suffix.size() &&
            line.compare(0, prefix.size(), prefix) == 0 &&
            line.compare(line.size() - suffix.size(), suffix.size(), suffix) ==
                0) {
            class_def.default_member = line.substr(
                prefix.size(), line.size() - prefix.size() - suffix.size());
            return;
        }
    }
}

bool Interpreter::scan_classes() {
    for (const auto& class_source : class_sources_) {
        std::string lowered_name;
        lowered_name.reserve(class_source.name.size());
        for (const char character : class_source.name) {
            lowered_name.push_back(ascii_lower(character));
        }
        if (lowered_name.empty() || is_reserved_identifier(lowered_name) ||
            class_definitions_.contains(lowered_name)) {
            set_error("WFC0126", "duplicate or reserved class name", offset_);
            return false;
        }
        ClassDef class_def;
        class_def.source = class_source.source;
        class_def.display_name = class_source.name;
        class_def.strict = mentions_option_explicit(class_source.source);
        class_definitions_.emplace(std::move(lowered_name),
                                   std::move(class_def));
    }
    for (const auto& udt_name : udt_names_) {
        std::string lowered;
        for (const char c : udt_name) {
            lowered.push_back(ascii_lower(c));
        }
        class_definitions_.at(lowered).is_udt = true;
    }
    for (auto& [lowered_name, class_def] : class_definitions_) {
        const auto saved_source = source_;
        const auto saved_offset = offset_;
        source_ = class_def.source;
        offset_ = 0;
        current_class_scan_name_ = lowered_name;
        const bool scanned_ok = scan_class_body(class_def);
        source_ = saved_source;
        offset_ = saved_offset;
        if (!scanned_ok) {
            return false;
        }
        scan_default_member(class_def);
    }
    return check_interface_completeness();
}

// REQ-0284: a class that `Implements` another class must define an
// `Interface_Member` counterpart for every public member of that interface;
// VB6 reports this as a compile error ("Class must implement ...").
bool Interpreter::check_interface_completeness() {
    for (const auto& [class_key, class_def] : class_definitions_) {
        for (const auto& interface_key : class_def.implements) {
            const auto found = class_definitions_.find(interface_key);
            if (found == class_definitions_.end() || found->second.is_udt) {
                continue;
            }
            const auto& interface_def = found->second;
            const auto missing = [&](const auto& members, const auto& own) {
                for (const auto& [member, definition] : members) {
                    if (definition.is_private || member.starts_with("class_")) {
                        continue;
                    }
                    if (!own.contains(interface_key + "_" + member)) {
                        return interface_def.display_name + "_" + member;
                    }
                }
                return std::string{};
            };
            for (const auto& name :
                 {missing(interface_def.methods, class_def.methods),
                  missing(interface_def.property_get, class_def.property_get),
                  missing(interface_def.property_let, class_def.property_let),
                  missing(interface_def.property_set,
                          class_def.property_set)}) {
                if (!name.empty()) {
                    set_error("WFC0154",
                              "class '" + class_def.display_name +
                                  "' must implement '" + name + "'",
                              offset_);
                    return false;
                }
            }
        }
    }
    return true;
}

bool Interpreter::scan_procedures() {
    const auto saved_offset = offset_;
    offset_ = 0;
    while (true) {
        skip_program_leading_trivia();
        if (at_end()) {
            break;
        }
        const auto line_offset = offset_;
        // A leading `Public`/`Private` is accepted (real VB6 source
        // commonly writes one) but not enforced -- REQ-0206 makes
        // Private meaningful for a *class* member's dot-accessibility,
        // but this evaluator has only one standard module, so nothing
        // exists for a module-level Private procedure to be hidden
        // from. If neither the modifier nor a bare Sub/Function follows,
        // this is some other statement; restore and skip the line as
        // scan_procedures already does for anything it doesn't
        // recognize.
        const auto pre_modifier_offset = offset_;
        if (consume_keyword("public") || consume_keyword("private") ||
            consume_keyword("friend")) {
            skip_horizontal_whitespace();
        }
        bool static_procedure = false;
        if (consume_keyword("static")) {
            static_procedure = true;
            skip_horizontal_whitespace();
        }
        bool is_function = false;
        bool is_declare = false;
        if (consume_keyword("declare")) {
            is_declare = true;
            skip_horizontal_whitespace();
            static_cast<void>(consume_keyword("ptrsafe"));
            skip_horizontal_whitespace();
        }
        std::string
            property_prefix;  // "" for Sub/Function, else "wfclet_" / "wfcset_"
        bool is_property = false;
        if (!is_declare && consume_keyword("property")) {
            skip_horizontal_whitespace();
            is_property = true;
            if (consume_keyword("get")) {
                is_function = true;
            } else if (consume_keyword("let")) {
                property_prefix = "wfclet_";
            } else if (consume_keyword("set")) {
                property_prefix = "wfcset_";
            } else {
                offset_ = pre_modifier_offset;
                skip_rest_of_line();
                continue;
            }
        } else if (consume_keyword("sub")) {
            is_function = false;
        } else if (consume_keyword("function")) {
            is_function = true;
        } else {
            offset_ = pre_modifier_offset;
            skip_rest_of_line();
            continue;
        }
        if (is_declare) {
            // REQ-0266: `Declare Function|Sub Name Lib "dll" ...` has no
            // body; register the name so a call reports error 453.
            skip_horizontal_whitespace();
            char declare_type_character{};
            auto declared = parse_identifier(&declare_type_character);
            if (declared.has_value() && !is_reserved_identifier(*declared) &&
                !procedures_.contains(*declared)) {
                ProcedureDef external;
                external.is_function = is_function;
                external.is_external = true;
                external.return_type_index = Value{Integer{}}.index();
                external.return_is_variant = true;
                // `Alias "Export"` names the DLL entry point that is bound.
                const auto line_end =
                    std::min(source_.find('\n', offset_), source_.size());
                std::string rest;
                for (std::size_t k = offset_; k < line_end; ++k) {
                    rest.push_back(ascii_lower(source_[k]));
                }
                const auto alias_at = rest.find(" alias ");
                if (alias_at != std::string::npos) {
                    const auto open = rest.find('"', alias_at);
                    const auto close = open == std::string::npos
                                           ? std::string::npos
                                           : rest.find('"', open + 1U);
                    if (close != std::string::npos) {
                        external.external_name =
                            rest.substr(open + 1U, close - open - 1U);
                    }
                }
                procedures_.emplace(*declared, std::move(external));
            }
            skip_rest_of_line();
            continue;
        }

        skip_horizontal_whitespace();
        const auto name_offset = offset_;
        char type_character{};
        auto name = parse_identifier(&type_character);
        if (!name.has_value() || (type_character != '\0' && !is_function)) {
            offset_ = saved_offset;
            set_error("WFC0118", "expected procedure name", name_offset);
            return false;
        }
        const std::string procedure_key = property_prefix + *name;
        if (is_reserved_identifier(*name) ||
            procedures_.contains(procedure_key)) {
            offset_ = saved_offset;
            set_error("WFC0119", "duplicate or reserved procedure name",
                      name_offset);
            return false;
        }

        ProcedureDef definition;
        definition.is_function = is_function;
        definition.static_locals = static_procedure;
        if (!scan_procedure_parameters(definition)) {
            offset_ = saved_offset;
            return false;
        }
        if (is_function) {
            skip_horizontal_whitespace();
            if (!consume_keyword("as")) {
                apply_implicit_return_type(definition, *name, type_character);
            } else {
                const auto type_offset = offset_;
                const auto type_result = parse_scalar_object_or_class_type();
                if (!type_result.has_value()) {
                    offset_ = saved_offset;
                    set_error("WFC0012",
                              "expected As Integer, As Long, As Double, As "
                              "Single, As Currency, "
                              "As String, As Boolean, As Object, As Variant, "
                              "or a known class name",
                              type_offset);
                    return false;
                }
                definition.return_type_index = type_result->type_index;
                definition.return_is_variant = type_result->is_variant;
                definition.return_is_object = type_result->is_object;
                definition.return_class_name = type_result->class_name;
                const auto array_marker = parse_function_array_return_marker(
                    *type_result, type_offset);
                if (!array_marker.has_value()) {
                    offset_ = saved_offset;
                    return false;
                }
                definition.return_is_array = *array_marker;
            }
        }
        if (!consume_loop_header_end()) {
            offset_ = saved_offset;
            return false;
        }
        definition.body_start = offset_;

        if (!skip_to_matching_end(is_property   ? "property"
                                  : is_function ? "function"
                                                : "sub",
                                  definition.body_end)) {
            offset_ = saved_offset;
            set_error(is_function ? "WFC0120" : "WFC0121",
                      is_property   ? "expected End Property"
                      : is_function ? "expected End Function"
                                    : "expected End Sub",
                      line_offset);
            return false;
        }
        definition.declaration_end = offset_;
        procedures_.emplace(procedure_key, std::move(definition));
    }
    offset_ = saved_offset;
    return true;
}

bool Interpreter::parse_option_statement(const std::size_t statement_offset) {
    if (!allow_declarations_) {
        set_error("WFC0068", "Option directives are only valid at module level",
                  statement_offset);
        return false;
    }
    skip_horizontal_whitespace();
    if (module_body_started_) {
        set_error("WFC0066", "Option directives must precede module statements",
                  statement_offset);
        return false;
    }
    if (consume_keyword("private")) {
        // `Option Private Module`: every module is private to the project
        // already.
        skip_horizontal_whitespace();
        static_cast<void>(consume_keyword("module"));
        return true;
    }
    if (consume_keyword("explicit")) {
        if (option_explicit_) {
            set_error("WFC0067", "duplicate Option Explicit", statement_offset);
            return false;
        }
        option_explicit_ = true;
        return true;
    }
    if (consume_keyword("compare")) {
        if (option_compare_set_) {
            set_error("WFC0069", "duplicate Option Compare", statement_offset);
            return false;
        }
        skip_horizontal_whitespace();
        if (consume_keyword("binary") || consume_keyword("database")) {
            option_compare_text_ = false;
        } else if (consume_keyword("text")) {
            option_compare_text_ = true;
        } else {
            set_error("WFC0070", "expected Binary or Text after Option Compare",
                      offset_);
            return false;
        }
        option_compare_set_ = true;
        return true;
    }
    if (consume_keyword("base")) {
        if (option_base_set_) {
            set_error("WFC0152", "duplicate Option Base", statement_offset);
            return false;
        }
        skip_horizontal_whitespace();
        if (consume('0')) {
            option_base_one_ = false;
        } else if (consume('1')) {
            option_base_one_ = true;
        } else {
            set_error("WFC0153", "expected 0 or 1 after Option Base", offset_);
            return false;
        }
        option_base_set_ = true;
        return true;
    }
    set_error("WFC0065", "expected Explicit, Compare, or Base after Option",
              offset_);
    return false;
}

bool Interpreter::parse_type_statement_skip(
    const std::size_t statement_offset) {
    skip_rest_of_line();
    while (true) {
        skip_program_leading_trivia();
        if (at_end()) {
            set_error("WFC0025", "expected End Type", statement_offset);
            return false;
        }
        if (consume_keyword("end")) {
            skip_horizontal_whitespace();
            if (consume_keyword("type")) {
                return true;
            }
        }
        skip_rest_of_line();
    }
}

bool Interpreter::parse_enum_statement(const std::size_t statement_offset) {
    skip_horizontal_whitespace();
    char type_character{};
    const auto name_offset = offset_;
    auto enum_name = parse_identifier(&type_character);
    if (!enum_name.has_value()) {
        set_error("WFC0011", "expected Enum name", name_offset);
        return false;
    }
    if (!consume_block_line_end()) {
        return false;
    }
    Integer next_value = 0;
    while (true) {
        skip_program_leading_trivia();
        if (at_end()) {
            set_error("WFC0025", "expected End Enum", statement_offset);
            return false;
        }
        if (consume_keyword("end")) {
            skip_horizontal_whitespace();
            if (!consume_keyword("enum")) {
                set_error("WFC0025", "expected Enum after End", offset_);
                return false;
            }
            return true;
        }
        const auto member_offset = offset_;
        auto member = parse_identifier(&type_character);
        if (!member.has_value() || type_character != '\0' ||
            is_reserved_identifier(*member)) {
            set_error("WFC0011", "expected Enum member name", member_offset);
            return false;
        }
        skip_horizontal_whitespace();
        if (consume('=')) {
            skip_horizontal_whitespace();
            constant_expression_ = true;
            auto value = parse_expression();
            constant_expression_ = false;
            if (!value.has_value()) {
                return false;
            }
            if (!coerce_numeric_value(*value, Value{Integer{}}.index(),
                                      member_offset) ||
                !std::holds_alternative<Integer>(*value)) {
                set_error("WFC0016", "Enum member value must be a Long",
                          member_offset);
                return false;
            }
            next_value = std::get<Integer>(*value);
        }
        if (execute_) {
            if (current_scope().variables.contains(*member)) {
                set_error("WFC0013",
                          "duplicate variable or constant declaration",
                          member_offset);
                return false;
            }
            current_scope().variables.emplace(*member, Value{next_value});
            current_scope().constants.insert(*member);
        }
        ++next_value;
        if (!consume_block_line_end()) {
            return false;
        }
    }
}

auto Interpreter::scan_statement_paren_group(
    const std::size_t open_offset) const -> ParenGroup {
    ParenGroup group;
    std::size_t depth = 0;
    bool in_string = false;
    bool top_level_comma = false;
    bool empty = true;
    bool balanced = false;
    std::size_t look = open_offset;
    for (; look < source_.size(); ++look) {
        const char c = source_[look];
        if (c == '"') {
            in_string = !in_string;
        }
        if (in_string) {
            continue;
        }
        if (c == '(') {
            ++depth;
        } else if (c == ')') {
            if (--depth == 0) {
                balanced = true;
                ++look;
                break;
            }
        } else if (c == ',' && depth == 1) {
            top_level_comma = true;
        } else if (c == '\r' || c == '\n') {
            break;
        } else if (depth == 1 && c != ' ' && c != '\t') {
            empty = false;
        }
    }
    if (balanced) {
        while (look < source_.size() &&
               (source_[look] == ' ' || source_[look] == '\t')) {
            ++look;
        }
        group.ends_statement = look >= source_.size() ||
                               source_[look] == '\r' || source_[look] == '\n' ||
                               source_[look] == ':' || source_[look] == '\'';
        group.is_list = top_level_comma || empty;
    }
    return group;
}

bool Interpreter::parse_static_declaration(const std::size_t statement_offset) {
    while (true) {
        if (!parse_single_static_declaration(statement_offset)) {
            return false;
        }
        skip_horizontal_whitespace();
        if (at_end() || current() != ',') {
            return true;
        }
        advance();
    }
}

bool Interpreter::parse_single_static_declaration(
    const std::size_t statement_offset) {
    if (current_procedure_def_ == nullptr) {
        set_error("WFC0144",
                  "Static is only valid inside a Sub, Function, or Property",
                  statement_offset);
        return false;
    }
    skip_horizontal_whitespace();
    const auto identifier_offset = offset_;
    char type_character{};
    auto identifier = parse_identifier(&type_character);
    if (!identifier.has_value()) {
        set_error("WFC0011", "expected variable name", identifier_offset);
        return false;
    }
    if (is_reserved_identifier(*identifier)) {
        set_error("WFC0017", "reserved keyword cannot be a variable name",
                  identifier_offset);
        return false;
    }
    if (!validate_type_character(type_character, identifier_offset)) {
        return false;
    }
    skip_horizontal_whitespace();

    // REQ-0231: `Static arr(<bounds>) As Type` -- always fixed-size,
    // 1-D or multi-dimensional (REQ-0201/REQ-0210's own bound
    // grammar, reused via `parse_fixed_array_bounds`). Unlike `Dim`,
    // `Static` has no dynamic (bound-less/comma-only) array form at
    // all: real VB6 requires a `Static` array's bounds to be fixed at
    // declaration time, with no `ReDim` counterpart ever possible for
    // one, so `Static arr()` is rejected outright rather than parsed
    // as an unallocated dynamic array the way `Dim arr()` is.
    bool is_array = false;
    Integer array_lower = 0;
    Integer array_upper = 0;
    std::vector<std::pair<Integer, Integer>> array_dimensions;
    if (!at_end() && current() == '(') {
        is_array = true;
        advance();
        skip_horizontal_whitespace();
        if (!at_end() && current() == ')') {
            set_error(
                "WFC0149",
                "a Static array must have fixed bounds (Static arr(n) As Type)",
                offset_);
            return false;
        }
        auto parsed_dimensions = parse_fixed_array_bounds(identifier_offset);
        if (!parsed_dimensions.has_value()) {
            return false;
        }
        array_dimensions = std::move(*parsed_dimensions);
        if (array_dimensions.size() == 1U) {
            array_lower = array_dimensions.front().first;
            array_upper = array_dimensions.front().second;
            array_dimensions.clear();
        }
        skip_horizontal_whitespace();
    }

    Value element_default;
    bool is_variant = false;
    bool is_object = false;
    std::string declared_class_name;
    if (type_character != '\0') {
        if (consume_keyword("as")) {
            set_error("WFC0012",
                      "type-declaration character cannot be combined with As",
                      offset_);
            return false;
        }
        if (type_character == '$') {
            element_default = std::string{};
        } else if (type_character == '#') {
            element_default = 0.0;
        } else if (type_character == '!') {
            element_default = 0.0f;
        } else if (type_character == '@') {
            element_default = Currency{};
        } else if (type_character == '%') {
            element_default = Int16{};
        } else {
            element_default = Integer{};
        }
    } else if (!consume_keyword("as")) {
        if (at_end() || current() == '\r' || current() == '\n' ||
            current() == ':' || current() == '\'') {
            element_default = Empty{};
            is_variant = true;
        } else {
            set_error("WFC0012",
                      "expected As Integer, As Long, As Double, As Single, As "
                      "Currency, As "
                      "String, As Boolean, As Object, or As Variant",
                      offset_);
            return false;
        }
    } else {
        skip_horizontal_whitespace();
        const auto type_offset = offset_;
        // REQ-0231: `Static o As Object`/`As SomeClassName`, reusing
        // the same class-name resolver a class-typed field/return
        // type/parameter already uses (REQ-0203/REQ-0228), extending
        // `Static` beyond the fixed-scalar/`Variant` forms it
        // originally supported alone (REQ-0206's Scope).
        const auto type_result = parse_scalar_object_or_class_type();
        if (!type_result.has_value()) {
            set_error("WFC0012",
                      "expected As Integer, As Long, As Double, As Single, As "
                      "Currency, As "
                      "String, As Boolean, As Object, or As Variant",
                      type_offset);
            return false;
        }
        if (type_result->is_object) {
            element_default = Nothing{};
            is_object = true;
            declared_class_name = type_result->class_name;
        } else if (type_result->is_variant) {
            element_default = Empty{};
            is_variant = true;
        } else {
            element_default = zero_value_for_index(type_result->type_index);
        }
    }

    if (is_array && (is_variant || is_object)) {
        set_error("WFC0149",
                  "a Static array's element type must be a fixed scalar type, "
                  "not Variant or "
                  "Object",
                  identifier_offset);
        return false;
    }

    Value initial_value;
    if (is_array) {
        const auto element_type_index = element_default.index();
        if (!array_dimensions.empty()) {
            std::size_t total_size = 1U;
            for (const auto& dimension : array_dimensions) {
                total_size *= static_cast<std::size_t>(dimension.second -
                                                       dimension.first) +
                              1U;
            }
            initial_value =
                ArrayValue{std::vector<Value>(total_size, element_default),
                           /*lower_bound=*/0,
                           /*is_dynamic=*/false,
                           /*is_allocated=*/true,
                           element_type_index,
                           array_dimensions};
        } else {
            const auto size =
                static_cast<std::size_t>(array_upper - array_lower) + 1U;
            initial_value =
                ArrayValue{std::vector<Value>(size, std::move(element_default)),
                           array_lower,
                           /*is_dynamic=*/false, /*is_allocated=*/true,
                           element_type_index};
        }
    } else {
        initial_value = std::move(element_default);
    }

    if (current_scope().variables.contains(*identifier)) {
        if (!allow_declarations_) {
            return true;  // REQ-0271: re-executed declaration inside a block
        }
        set_error("WFC0013", "duplicate variable declaration",
                  identifier_offset);
        return false;
    }
    auto& statics =
        current_instance() != nullptr
            ? current_instance()->static_scopes[current_procedure_def_]
            : current_procedure_def_->statics;
    if (!statics.variables.contains(*identifier)) {
        statics.variables.emplace(*identifier, std::move(initial_value));
        if (is_variant) {
            statics.variant_variables.insert(*identifier);
        }
        if (is_object) {
            statics.object_variables.insert(*identifier);
            if (!declared_class_name.empty()) {
                statics.object_class_names.emplace(*identifier,
                                                   declared_class_name);
            }
        }
    }
    current_scope().variables.emplace(*identifier,
                                      statics.variables.at(*identifier));
    if (is_variant) {
        current_scope().variant_variables.insert(*identifier);
    }
    if (is_object) {
        current_scope().object_variables.insert(*identifier);
        if (!declared_class_name.empty()) {
            current_scope().object_class_names.emplace(*identifier,
                                                       declared_class_name);
        }
    }
    current_scope().static_variable_names.insert(*identifier);
    return true;
}

std::optional<std::vector<std::pair<Integer, Integer>>>
Interpreter::parse_fixed_array_bounds(const std::size_t identifier_offset) {
    std::vector<std::pair<Integer, Integer>> dimensions;
    while (true) {
        const auto first_offset = offset_;
        auto first_bound = parse_expression();
        if (!first_bound.has_value()) {
            return std::nullopt;
        }
        const auto first_long = coerce_long(*first_bound, first_offset);
        if (!first_long.has_value()) {
            return std::nullopt;
        }
        Integer dimension_lower = 0;
        Integer dimension_upper = 0;
        skip_horizontal_whitespace();
        if (consume_keyword("to")) {
            skip_horizontal_whitespace();
            const auto second_offset = offset_;
            auto second_bound = parse_expression();
            if (!second_bound.has_value()) {
                return std::nullopt;
            }
            const auto second_long = coerce_long(*second_bound, second_offset);
            if (!second_long.has_value()) {
                return std::nullopt;
            }
            dimension_lower = *first_long;
            dimension_upper = *second_long;
        } else {
            // REQ-0226: a bound-less dimension (`<bound>`, no `<lower>
            // To`) takes its lower bound from `Option Base` -- `0`
            // unless `Option Base 1` was declared for this module.
            dimension_lower = option_base_one_ ? 1 : 0;
            dimension_upper = *first_long;
        }
        if (dimension_lower > dimension_upper) {
            set_error("WFC0117",
                      "array lower bound must not exceed the upper bound",
                      identifier_offset);
            return std::nullopt;
        }
        dimensions.emplace_back(dimension_lower, dimension_upper);
        skip_horizontal_whitespace();
        if (!at_end() && current() == ',') {
            advance();
            skip_horizontal_whitespace();
            continue;
        }
        break;
    }
    if (!consume(')')) {
        set_error("WFC0005", "expected closing parenthesis", offset_);
        return std::nullopt;
    }
    return dimensions;
}

bool Interpreter::parse_declaration() {
    while (true) {
        if (!parse_single_declaration()) {
            return false;
        }
        skip_horizontal_whitespace();
        if (at_end() || current() != ',') {
            return true;
        }
        advance();
    }
}

bool Interpreter::parse_single_declaration() {
    udt_array_ = false;
    fixed_string_length_ = 0;
    skip_horizontal_whitespace();
    const auto identifier_offset = offset_;
    char type_character{};
    auto identifier = parse_identifier(&type_character);
    if (!identifier.has_value()) {
        set_error("WFC0011", "expected variable name", identifier_offset);
        return false;
    }
    const bool repeat_in_block =
        !allow_declarations_ && current_scope().variables.contains(*identifier);
    if (is_reserved_identifier(*identifier)) {
        set_error("WFC0017", "reserved keyword cannot be a variable name",
                  identifier_offset);
        return false;
    }
    if (!validate_type_character(type_character, identifier_offset)) {
        return false;
    }

    skip_horizontal_whitespace();
    bool is_array = false;
    bool is_dynamic_array = false;
    Integer array_lower = 0;
    Integer array_upper = 0;
    // Populated only when the declaration writes two or more
    // comma-separated bounds (a fixed-size multi-dimensional array,
    // REQ-0210); stays empty for the ordinary 1-D forms, which
    // continue to use array_lower/array_upper alone.
    std::vector<std::pair<Integer, Integer>> array_dimensions;
    // Only meaningful when `is_dynamic_array`: a comma-only
    // declaration's pre-declared dimension count (REQ-0219), e.g.
    // `Dim arr(,) As Type` is 2, `Dim arr(,,) As Type` is 3; stays 0
    // for the plain `Dim arr()` form, which leaves the count
    // unconstrained until the first `ReDim` decides it.
    std::size_t dynamic_dimension_count = 0;
    if (!at_end() && current() == '(') {
        is_array = true;
        advance();
        skip_horizontal_whitespace();
        if (!at_end() && current() == ')') {
            // `Dim identifier()` with no bound: a dynamic array,
            // unallocated until its first `ReDim` (REQ-0207). WFC0116
            // previously rejected this form outright; retired now that
            // dynamic arrays are supported.
            is_dynamic_array = true;
            advance();
            skip_horizontal_whitespace();
        } else {
            // `Dim identifier(,)`, `Dim identifier(,,)`, ... (REQ-0219):
            // a dynamic array whose dimension count is fixed in advance
            // (one more than the comma count), still unallocated until
            // its first `ReDim`. Falls through to the ordinary bound
            // parsing below when the parenthesized content is not
            // comma-only (the common case).
            const auto comma_check_offset = offset_;
            std::size_t comma_count = 0;
            while (!at_end() && current() == ',') {
                advance();
                skip_horizontal_whitespace();
                ++comma_count;
            }
            if (comma_count > 0 && !at_end() && current() == ')') {
                is_dynamic_array = true;
                dynamic_dimension_count = comma_count + 1U;
                advance();
                skip_horizontal_whitespace();
            } else {
                offset_ = comma_check_offset;
            }
        }
        if (is_dynamic_array) {
            // No bounds to parse; fall through to the shared `As Type`
            // handling below.
        } else {
            auto parsed_dimensions =
                parse_fixed_array_bounds(identifier_offset);
            if (!parsed_dimensions.has_value()) {
                return false;
            }
            array_dimensions = std::move(*parsed_dimensions);
            if (array_dimensions.size() == 1U) {
                array_lower = array_dimensions.front().first;
                array_upper = array_dimensions.front().second;
                array_dimensions.clear();
            }
        }
        skip_horizontal_whitespace();
    }

    Value element_default;
    bool is_variant = false;
    bool is_object = false;
    std::string declared_class_name;
    if (type_character != '\0') {
        if (consume_keyword("as")) {
            set_error("WFC0012",
                      "type-declaration character cannot be combined with As",
                      offset_);
            return false;
        }
        if (type_character == '$') {
            element_default = std::string{};
        } else if (type_character == '#') {
            element_default = 0.0;
        } else if (type_character == '!') {
            element_default = 0.0f;
        } else if (type_character == '@') {
            element_default = Currency{};
        } else if (type_character == '%') {
            element_default = Int16{};
        } else {
            element_default = Integer{};
        }
    } else if (!consume_keyword("as")) {
        // A bare `Dim x` with no As clause and no type-declaration
        // character implicitly declares a Variant, matching real VB6.
        // Arrays require an explicit element type in this evaluator
        // (Variant-element arrays are outside the current array scope).
        if (const auto default_type = default_type_for(*identifier);
            default_type.has_value() &&
            (at_end() || current() == '\r' || current() == '\n' ||
             current() == ':' || current() == '\'' || current() == ',')) {
            element_default = zero_value_for_index(*default_type);
        } else if (at_end() || current() == '\r' || current() == '\n' ||
                   current() == ':' || current() == '\'' || current() == ',') {
            element_default = Empty{};
            is_variant = true;
        } else {
            set_error("WFC0012",
                      "expected As Integer, As Long, As Double, As Single, As "
                      "Currency, As "
                      "String, As Boolean, As Object, or As Variant",
                      offset_);
            return false;
        }
    } else {
        skip_horizontal_whitespace();
        bool eager_new = false;
        bool lazy_new = false;
        if (consume_keyword("long")) {
            element_default = Integer{};
        } else if (consume_keyword("integer")) {
            element_default = Int16{};
        } else if (consume_keyword("double")) {
            element_default = 0.0;
        } else if (consume_keyword("single")) {
            element_default = 0.0f;
        } else if (consume_keyword("date")) {
            element_default = DateValue{};
        } else if (consume_keyword("byte")) {
            element_default = Byte{};
        } else if (consume_keyword("decimal")) {
            element_default = Decimal{};
        } else if (consume_keyword("currency")) {
            element_default = Currency{};
        } else if (consume_keyword("string")) {
            element_default = std::string{};
            skip_horizontal_whitespace();
            if (!at_end() && current() == '*') {
                advance();
                skip_horizontal_whitespace();
                const auto length_offset = offset_;
                auto length = parse_expression();
                if (!length.has_value()) {
                    return false;
                }
                const auto size_value = whole_value(*length);
                const Integer* const size = size_value ? &*size_value : nullptr;
                if (size == nullptr || *size < 1 || *size > 65526) {
                    set_error("WFC0012",
                              "fixed String length must be 1 to 65526",
                              length_offset);
                    return false;
                }
                fixed_string_length_ = static_cast<std::size_t>(*size);
                element_default = std::string(fixed_string_length_, ' ');
            }
        } else if (consume_keyword("boolean")) {
            element_default = false;
        } else if (consume_keyword("object")) {
            element_default = Nothing{};
            is_object = true;
        } else if (consume_keyword("variant")) {
            element_default = Empty{};
            is_variant = true;
        } else if (!is_array && consume_keyword("new")) {
            // `As New ClassName` eagerly instantiates the class right
            // here (a documented simplification of VB6's lazy
            // auto-instantiation, which only creates the instance on
            // first use; see REQ-0203's Scope).
            skip_horizontal_whitespace();
            const auto class_name_offset = offset_;
            char class_type_character{};
            auto class_name = parse_identifier(&class_type_character);
            if (!class_name.has_value() || class_type_character != '\0' ||
                !class_definitions_.contains(*class_name)) {
                set_error("WFC0134", "unknown class name", class_name_offset);
                return false;
            }
            declared_class_name = std::move(*class_name);
            is_object = true;
            if (is_udt_class(declared_class_name) || is_array) {
                eager_new = true;
            } else {
                lazy_new = true;
                element_default = Nothing{};
            }
        } else {
            // A bare identifier here, if it names a known class, is a
            // fixed `As ClassName` declaration (initialized to
            // Nothing, like `As Object`, but Set-checked against this
            // specific class -- see parse_set_statement).
            const auto class_name_offset = offset_;
            const auto saved_offset = offset_;
            char class_type_character{};
            auto class_name = parse_identifier(&class_type_character);
            if (class_name.has_value() && class_type_character == '\0' &&
                class_definitions_.contains(*class_name)) {
                declared_class_name = std::move(*class_name);
                element_default = Nothing{};
                is_object = true;
                if (is_udt_class(declared_class_name)) {
                    if (is_array) {
                        udt_array_ = true;
                    } else {
                        eager_new = true;
                    }
                }
            } else {
                offset_ = saved_offset;
                set_error("WFC0012",
                          "expected As Integer, As Long, As Double, "
                          "As Single, As Currency, As String, As Boolean, "
                          "As Object, As Variant, or a known class name",
                          class_name_offset);
                return false;
            }
        }

        if (eager_new && repeat_in_block) {
            element_default = Nothing{};
        } else if (eager_new) {
            auto instance =
                instantiate_class(declared_class_name, identifier_offset);
            if (!instance.has_value()) {
                return false;
            }
            element_default = std::move(*instance);
        }

        if (lazy_new && !repeat_in_block) {
            pending_lazy_new_ = true;
        }
        if (!declared_class_name.empty() && !is_array) {
            // An array's declared_class_name (REQ-0214) is threaded
            // straight into its own ArrayValue.element_class_name
            // below instead, since `object_class_names` is keyed by
            // variable name for a whole scalar/object variable's own
            // Set-target class check, the same reasoning
            // is_variant/is_object already follow for
            // variant_variables/object_variables above.
            current_scope().object_class_names.emplace(*identifier,
                                                       declared_class_name);
        }
    }

    Value initial_value;
    if (is_array) {
        const auto element_type_index = element_default.index();
        if (is_dynamic_array) {
            ArrayValue array_value{/*elements=*/{},
                                   /*lower_bound=*/0,
                                   /*is_dynamic=*/true,
                                   /*is_allocated=*/false,
                                   element_type_index,
                                   /*dimensions=*/{},
                                   is_variant,
                                   is_object,
                                   declared_class_name};
            array_value.dynamic_dimension_count = dynamic_dimension_count;
            array_value.dimension_count_declared = dynamic_dimension_count > 0U;
            initial_value = std::move(array_value);
        } else if (!array_dimensions.empty()) {
            std::size_t total_size = 1U;
            for (const auto& dimension : array_dimensions) {
                total_size *= static_cast<std::size_t>(dimension.second -
                                                       dimension.first) +
                              1U;
            }
            initial_value =
                ArrayValue{std::vector<Value>(total_size, element_default),
                           /*lower_bound=*/0,
                           /*is_dynamic=*/false,
                           /*is_allocated=*/true,
                           element_type_index,
                           array_dimensions,
                           is_variant,
                           is_object,
                           declared_class_name};
        } else {
            const auto size =
                static_cast<std::size_t>(array_upper - array_lower) + 1U;
            initial_value =
                ArrayValue{std::vector<Value>(size, std::move(element_default)),
                           array_lower,
                           /*is_dynamic=*/false,
                           /*is_allocated=*/true,
                           element_type_index,
                           /*dimensions=*/{},
                           is_variant,
                           is_object,
                           declared_class_name};
        }
    } else {
        initial_value = std::move(element_default);
    }
    if (is_array && fixed_string_length_ != 0U) {
        if (auto* const fixed_array = std::get_if<ArrayValue>(&initial_value)) {
            fixed_array->element_fixed_length = fixed_string_length_;
        }
    }
    if (udt_array_ && !repeat_in_block) {
        udt_array_ = false;
        if (auto* const array = std::get_if<ArrayValue>(&initial_value)) {
            for (auto& element : array->elements) {
                auto instance =
                    instantiate_class(declared_class_name, identifier_offset);
                if (!instance.has_value()) {
                    return false;
                }
                element = std::move(*instance);
            }
        }
    }

    const auto [entry, inserted] = current_scope().variables.emplace(
        *identifier, std::move(initial_value));
    (void)entry;
    if (!inserted) {
        if (repeat_in_block) {
            return true;  // REQ-0271: re-executed declaration inside a block
        }
        set_error("WFC0013", "duplicate variable declaration",
                  identifier_offset);
        return false;
    }
    if (fixed_string_length_ != 0U && !is_array) {
        current_scope().fixed_string_lengths[*identifier] =
            fixed_string_length_;
    }
    if (pending_lazy_new_) {
        pending_lazy_new_ = false;
        current_scope().auto_new_variables.insert(*identifier);
    }
    // A Variant/Object-*element* array (REQ-0212) does not itself go in
    // variant_variables/object_variables: those sets govern a whole
    // scalar/object variable's own retyping-on-assignment and
    // Set-only rules, which do not apply to the array *variable*
    // itself (a whole-array assignment like `arr1 = arr2` is an
    // ordinary same-type value copy, matching every other array kind's
    // existing REQ-0201 simplification) -- only to its elements,
    // checked directly against ArrayValue.is_variant_element/
    // is_object_element wherever an element is read or written.
    if (is_variant && !is_array) {
        current_scope().variant_variables.insert(*identifier);
    }
    if (is_object && !is_array) {
        current_scope().object_variables.insert(*identifier);
    }
    return true;
}

bool Interpreter::parse_redim_statement() {
    skip_horizontal_whitespace();
    const bool preserve = consume_keyword("preserve");
    if (preserve) {
        skip_horizontal_whitespace();
    }
    while (true) {
        if (!parse_redim_declarator(preserve)) {
            return false;
        }
        skip_horizontal_whitespace();
        if (!consume(',')) {
            return true;
        }
        skip_horizontal_whitespace();
    }
}

bool Interpreter::parse_redim_declarator(const bool preserve) {
    const auto identifier_offset = offset_;
    char type_character{};
    auto identifier = parse_identifier(&type_character);
    if (!identifier.has_value()) {
        set_error("WFC0011", "expected array name", identifier_offset);
        return false;
    }
    std::vector<std::string> member_path;  // `ReDim obj.field(...)`
    skip_horizontal_whitespace();
    while (!at_end() && current() == '.') {
        advance();
        skip_horizontal_whitespace();
        char member_type_character{};
        auto member = parse_identifier(&member_type_character);
        if (!member.has_value()) {
            set_error("WFC0011", "expected member name after '.'", offset_);
            return false;
        }
        member_path.push_back(std::move(*member));
        skip_horizontal_whitespace();
    }
    if (at_end() || current() != '(') {
        set_error("WFC0145", "ReDim requires an array bound", offset_);
        return false;
    }
    advance();
    skip_horizontal_whitespace();
    if (!at_end() && current() == ')') {
        set_error("WFC0145", "ReDim requires an array bound", offset_);
        return false;
    }
    std::vector<std::pair<Integer, Integer>> new_dimensions;
    while (true) {
        const auto first_offset = offset_;
        auto first_bound = parse_expression();
        if (!first_bound.has_value()) {
            return false;
        }
        const auto first_long = coerce_long(*first_bound, first_offset);
        if (!first_long.has_value()) {
            return false;
        }
        skip_horizontal_whitespace();
        Integer dimension_lower = 0;
        Integer dimension_upper = 0;
        if (consume_keyword("to")) {
            skip_horizontal_whitespace();
            const auto second_offset = offset_;
            auto second_bound = parse_expression();
            if (!second_bound.has_value()) {
                return false;
            }
            const auto second_long = coerce_long(*second_bound, second_offset);
            if (!second_long.has_value()) {
                return false;
            }
            dimension_lower = *first_long;
            dimension_upper = *second_long;
        } else {
            // REQ-0226: matches Dim's own bound-less-dimension rule.
            dimension_lower = option_base_one_ ? 1 : 0;
            dimension_upper = *first_long;
        }
        if (execute_ && dimension_lower > dimension_upper) {
            return raise_runtime(9, "Subscript out of range",
                                 identifier_offset);
        }
        new_dimensions.emplace_back(dimension_lower, dimension_upper);
        skip_horizontal_whitespace();
        if (!at_end() && current() == ',') {
            advance();
            skip_horizontal_whitespace();
            continue;
        }
        break;
    }
    if (!consume(')')) {
        set_error("WFC0005", "expected closing parenthesis", offset_);
        return false;
    }
    // `ReDim a(n) As Type`: the type must agree with the array's own; for a
    // Variant it is the new array's element type.
    std::optional<ResolvedType> redim_type;
    skip_horizontal_whitespace();
    if (consume_keyword("as")) {
        skip_horizontal_whitespace();
        const auto type_offset = offset_;
        redim_type = parse_scalar_object_or_class_type();
        if (!redim_type.has_value()) {
            set_error("WFC0012", "expected a type after As", type_offset);
            return false;
        }
    }

    if (!execute_) {
        return true;
    }

    auto variable_lookup = find_variable(*identifier);
    if (variable_lookup.value == nullptr && member_path.empty() &&
        !strict_here() && !in_with_identifier(*identifier)) {
        // Without Option Explicit, ReDim declares the (Variant) array too.
        current_scope().variables.emplace(*identifier, Value{Empty{}});
        current_scope().variant_variables.insert(*identifier);
        variable_lookup = find_variable(*identifier);
    }
    Value* redim_target = variable_lookup.value;
    for (const auto& member : member_path) {
        auto* holder = redim_target != nullptr
                           ? std::get_if<ObjectInstance>(redim_target)
                           : nullptr;
        if (holder == nullptr) {
            set_error("WFC0136", "member access requires an object reference",
                      identifier_offset);
            return false;
        }
        const auto field = holder->data->fields.variables.find(member);
        if (field == holder->data->fields.variables.end()) {
            set_error("WFC0135", "unknown member", identifier_offset);
            return false;
        }
        redim_target = &field->second;
    }
    if (redim_target != nullptr &&
        !std::holds_alternative<ArrayValue>(*redim_target) &&
        (std::holds_alternative<Empty>(*redim_target) ||
         (member_path.empty() &&
          variable_lookup.scope->variant_variables.contains(*identifier)))) {
        // A Variant (or Empty) becomes a dynamic array.
        ArrayValue created{};
        created.is_dynamic = true;
        created.is_allocated = false;
        if (redim_type.has_value() && !redim_type->is_variant) {
            if (redim_type->is_object) {
                created.is_object_element = true;
                created.element_class_name = redim_type->class_name;
                created.element_type_index = Value{Nothing{}}.index();
            } else {
                created.element_type_index = redim_type->type_index;
            }
        } else {
            created.is_variant_element = true;
            created.element_type_index = Value{Empty{}}.index();
        }
        *redim_target = std::move(created);
    }
    struct RedimVariable {
        Value* value;
    };
    const RedimVariable variable{redim_target};
    if (variable.value == nullptr ||
        !std::holds_alternative<ArrayValue>(*variable.value) ||
        !std::get<ArrayValue>(*variable.value).is_dynamic) {
        set_error("WFC0145",
                  "ReDim requires a previously declared dynamic array (Dim "
                  "identifier())",
                  identifier_offset);
        return false;
    }
    auto& array = std::get<ArrayValue>(*variable.value);

    // REQ-0219: a dynamic array's dimension count, once fixed (by a
    // comma-only `Dim` or an earlier `ReDim`), is authoritative for
    // every later `ReDim`; only an array that has never been
    // allocated *and* was declared with the plain `Dim identifier()`
    // form (no pre-declared count) lets this first `ReDim` decide it.
    std::size_t required_dimension_count = new_dimensions.size();
    if (array.dimension_count_declared) {
        required_dimension_count =
            array.dynamic_dimension_count;  // `Dim a(,)` fixed it
    } else if (array.is_allocated && preserve) {
        required_dimension_count =
            array.dimensions.empty() ? 1U : array.dimensions.size();
    }
    if (new_dimensions.size() != required_dimension_count) {
        set_error("WFC0115",
                  "ReDim dimension count does not match the array's declared "
                  "dimension count",
                  identifier_offset);
        return false;
    }

    // REQ-0219: `ReDim Preserve` on a multi-dimensional array may only
    // resize the last dimension; every earlier dimension must keep
    // its exact current bounds, matching real VB6's own restriction
    // (a 1-D array has no "earlier dimension" to check, so this loop
    // never runs for one).
    if (preserve && array.is_allocated && new_dimensions.size() > 1U) {
        for (std::size_t dimension = 0; dimension + 1U < new_dimensions.size();
             ++dimension) {
            if (new_dimensions[dimension] != array.dimensions[dimension]) {
                set_error("WFC0151",
                          "ReDim Preserve may only change a multi-dimensional "
                          "array's last "
                          "dimension",
                          identifier_offset);
                return false;
            }
        }
    }

    std::size_t total_size = 1U;
    for (const auto& dimension : new_dimensions) {
        total_size *=
            static_cast<std::size_t>(dimension.second - dimension.first) + 1U;
    }
    std::vector<Value> new_elements(
        total_size, array.element_fixed_length != 0U
                        ? Value{std::string(array.element_fixed_length, ' ')}
                        : array_element_default(array.element_type_index));

    if (preserve && array.is_allocated && !array.elements.empty()) {
        // Every dimension except the last keeps identical bounds
        // (validated above, or there is only one dimension), so the
        // "outer" stride is the same in the old and new arrays; only
        // the last dimension's absolute-index overlap needs
        // computing -- generalizing the original 1-D-only overlap
        // logic (which allowed the single dimension's bounds to
        // shift, not just grow/shrink) to the last dimension of any
        // dimension count, 1-D included.
        const std::pair<Integer, Integer> old_last =
            array.dimensions.empty()
                ? std::pair<Integer, Integer>{array.lower_bound,
                                              array.lower_bound +
                                                  static_cast<Integer>(
                                                      array.elements.size()) -
                                                  1}
                : array.dimensions.back();
        const auto& new_last = new_dimensions.back();
        const Integer overlap_lower = std::max(old_last.first, new_last.first);
        const Integer overlap_upper =
            std::min(old_last.second, new_last.second);
        if (overlap_lower <= overlap_upper) {
            const auto old_last_size =
                static_cast<std::size_t>(old_last.second - old_last.first) + 1U;
            const auto new_last_size =
                static_cast<std::size_t>(new_last.second - new_last.first) + 1U;
            const auto outer_size = array.elements.size() / old_last_size;
            for (std::size_t outer_index = 0; outer_index < outer_size;
                 ++outer_index) {
                for (Integer absolute_index = overlap_lower;
                     absolute_index <= overlap_upper; ++absolute_index) {
                    const auto old_flat = outer_index * old_last_size +
                                          static_cast<std::size_t>(
                                              absolute_index - old_last.first);
                    const auto new_flat = outer_index * new_last_size +
                                          static_cast<std::size_t>(
                                              absolute_index - new_last.first);
                    new_elements[new_flat] = array.elements[old_flat];
                }
            }
        }
    }

    if (array.is_object_element && is_udt_class(array.element_class_name)) {
        // REQ-0241/0253: every slot of a UDT array owns its own instance.
        for (auto& slot : new_elements) {
            if (std::holds_alternative<Nothing>(slot)) {
                auto instance = instantiate_class(array.element_class_name,
                                                  identifier_offset);
                if (!instance.has_value()) {
                    return false;
                }
                slot = std::move(*instance);
            }
        }
    }

    // Objects that did not survive the resize are released; ones carried
    // into the new storage are still referenced, so they are left alone.
    for (auto& old_element : array.elements) {
        if (!terminate_if_last_reference(old_element)) {
            return false;
        }
    }
    array.elements = std::move(new_elements);
    if (new_dimensions.size() == 1U) {
        array.lower_bound = new_dimensions.front().first;
        array.dimensions.clear();
    } else {
        array.lower_bound = 0;
        array.dimensions = new_dimensions;
    }
    array.is_allocated = true;
    array.dynamic_dimension_count = new_dimensions.size();
    return true;
}

bool Interpreter::parse_erase_statement() {
    std::vector<std::pair<std::string, std::size_t>> targets;
    while (true) {
        skip_horizontal_whitespace();
        const auto identifier_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value()) {
            set_error("WFC0011", "expected array name", identifier_offset);
            return false;
        }
        targets.emplace_back(std::move(*identifier), identifier_offset);
        skip_horizontal_whitespace();
        if (!at_end() && current() == ',') {
            advance();
            continue;
        }
        break;
    }
    if (!execute_) {
        return true;
    }
    for (const auto& target : targets) {
        const auto variable = find_variable(target.first);
        if (variable.value == nullptr ||
            !std::holds_alternative<ArrayValue>(*variable.value)) {
            set_error("WFC0146", "Erase requires an array argument",
                      target.second);
            return false;
        }
        if (!terminate_if_last_reference(*variable.value)) {
            return false;  // release the objects the array holds
        }
        auto& array = std::get<ArrayValue>(*variable.value);
        if (array.is_dynamic) {
            // REQ-0219: a multi-dimensional dynamic array's
            // `dimensions` must be cleared too, not just `elements` --
            // otherwise a later index read/write would still see the
            // old (now-stale) per-dimension bounds as "in range" while
            // `elements` is empty, indexing past the end of an empty
            // vector. If the array was ever allocated,
            // `dynamic_dimension_count` is locked in from its
            // about-to-be-cleared shape first, so a later `ReDim`
            // still enforces the same dimension count Erase does not
            // let the array forget, matching real VB6; if it was
            // never allocated at all (Erase on an untouched `Dim
            // arr()`/`Dim arr(,)`), any already-pre-declared count is
            // left exactly as it was.
            if (array.is_allocated) {
                array.dynamic_dimension_count =
                    array_expected_dimension_count(array);
            }
            array.dimensions.clear();
            array.elements.clear();
            array.lower_bound = 0;
            array.is_allocated = false;
        } else {
            std::fill(array.elements.begin(), array.elements.end(),
                      array_element_default(array.element_type_index));
        }
    }
    return true;
}

bool Interpreter::parse_constant_declaration() {
    while (true) {
        if (!parse_single_constant_declaration()) {
            return false;
        }
        skip_horizontal_whitespace();
        if (at_end() || current() != ',') {
            return true;
        }
        advance();
    }
}

bool Interpreter::parse_single_constant_declaration() {
    skip_horizontal_whitespace();
    const auto identifier_offset = offset_;
    char type_character{};
    auto identifier = parse_identifier(&type_character);
    if (!identifier.has_value()) {
        set_error("WFC0011", "expected constant name", identifier_offset);
        return false;
    }
    if (is_reserved_identifier(*identifier)) {
        set_error("WFC0017", "reserved keyword cannot be a constant name",
                  identifier_offset);
        return false;
    }
    if (!validate_type_character(type_character, identifier_offset)) {
        return false;
    }
    const bool repeat_constant =
        !allow_declarations_ && current_scope().variables.contains(*identifier);
    if (!repeat_constant && current_scope().variables.contains(*identifier)) {
        set_error("WFC0013", "duplicate variable or constant declaration",
                  identifier_offset);
        return false;
    }

    // REQ-0227: a `Const` with neither a type-declaration character
    // nor an `As Type` clause infers its type from the initializer's
    // own value, the same real-VB6 asymmetry that makes an untyped
    // `Const` type-inferred while an untyped `Dim` instead defaults
    // to `Variant` (parse_declaration's own bare-`Dim` branch, above).
    // `has_explicit_type` stays false in that case, and `expected_type`
    // is set from the parsed value below instead of checked against it.
    bool has_explicit_type = type_character != '\0';
    std::size_t expected_type{};
    skip_horizontal_whitespace();
    if (type_character != '\0') {
        if (consume_keyword("as")) {
            set_error("WFC0012",
                      "type-declaration character cannot be combined with As",
                      offset_);
            return false;
        }
        expected_type = type_character_index(type_character);
    } else if (consume_keyword("as")) {
        has_explicit_type = true;
        skip_horizontal_whitespace();
        if (consume_keyword("long")) {
            expected_type = Value{Integer{}}.index();
        } else if (consume_keyword("integer")) {
            expected_type = Value{Int16{}}.index();
        } else if (consume_keyword("double")) {
            expected_type = Value{0.0}.index();
        } else if (consume_keyword("single")) {
            expected_type = Value{0.0f}.index();
        } else if (consume_keyword("date")) {
            expected_type = Value{DateValue{}}.index();
        } else if (consume_keyword("byte")) {
            expected_type = Value{Byte{}}.index();
        } else if (consume_keyword("decimal")) {
            expected_type = Value{Decimal{}}.index();
        } else if (consume_keyword("currency")) {
            expected_type = Value{Currency{}}.index();
        } else if (consume_keyword("string")) {
            expected_type = Value{std::string{}}.index();
        } else if (consume_keyword("boolean")) {
            expected_type = Value{false}.index();
        } else {
            set_error("WFC0012",
                      "expected As Integer, As Long, As Double, As Single, As "
                      "Currency, As "
                      "String, or As Boolean",
                      offset_);
            return false;
        }
    }

    skip_horizontal_whitespace();
    if (!consume('=')) {
        set_error("WFC0014", "expected constant initializer", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    constant_expression_ = true;
    auto value = parse_expression();
    constant_expression_ = false;
    if (!value.has_value()) {
        return false;
    }
    if (has_explicit_type) {
        if (!coerce_numeric_value(*value, expected_type, identifier_offset)) {
            return false;
        }
        if (value->index() != expected_type) {
            set_error("WFC0016", "constant initializer type mismatch",
                      identifier_offset);
            return false;
        }
    }

    if (repeat_constant) {
        return true;
    }
    current_scope().variables.emplace(*identifier, std::move(*value));
    current_scope().constants.insert(std::move(*identifier));
    return true;
}

std::size_t Interpreter::udt_byte_size(const Value& value) const {
    if (std::holds_alternative<Integer>(value)) {
        return 4;
    }
    if (std::holds_alternative<Int16>(value) ||
        std::holds_alternative<bool>(value)) {
        return 2;
    }
    if (std::holds_alternative<Byte>(value)) {
        return 1;
    }
    if (std::holds_alternative<float>(value)) {
        return 4;
    }
    if (std::holds_alternative<double>(value) ||
        std::holds_alternative<Currency>(value) ||
        std::holds_alternative<DateValue>(value)) {
        return 8;
    }
    if (const auto* array = std::get_if<ArrayValue>(&value)) {
        std::size_t total = 0;
        for (const auto& element : array->elements) {
            total += udt_byte_size(element);
        }
        return total;
    }
    if (const auto* instance = std::get_if<ObjectInstance>(&value)) {
        const auto class_iterator =
            class_definitions_.find(instance->data->class_name);
        if (class_iterator == class_definitions_.end()) {
            return 0;
        }
        std::size_t total = 0;
        for (const auto& name : class_iterator->second.field_order) {
            const auto field = instance->data->fields.variables.find(name);
            if (field == instance->data->fields.variables.end()) {
                continue;
            }
            if (const auto* text = std::get_if<std::string>(&field->second)) {
                total +=
                    instance->data->fields.fixed_string_lengths.contains(name)
                        ? text->size()
                        : 2U + text->size();
            } else {
                total += udt_byte_size(field->second);
            }
        }
        return total;
    }
    if (const auto* text = std::get_if<std::string>(&value)) {
        return text->size();
    }
    return 16;  // Variant / Decimal
}

bool Interpreter::parse_property_declaration_skip(
    const std::size_t statement_offset) {
    skip_horizontal_whitespace();
    std::string prefix;
    if (consume_keyword("get")) {
    } else if (consume_keyword("let")) {
        prefix = "wfclet_";
    } else if (consume_keyword("set")) {
        prefix = "wfcset_";
    } else {
        set_error("WFC0010", "expected Get, Let or Set after Property",
                  offset_);
        return false;
    }
    skip_horizontal_whitespace();
    const auto name_offset = offset_;
    char type_character{};
    auto name = parse_identifier(&type_character);
    const auto definition =
        name.has_value() ? procedures_.find(prefix + *name) : procedures_.end();
    if (definition == procedures_.end()) {
        set_error("WFC0118", "expected procedure name", name_offset);
        return false;
    }
    static_cast<void>(statement_offset);
    offset_ = definition->second.declaration_end;
    return true;
}

bool Interpreter::parse_procedure_declaration_skip(
    const std::size_t statement_offset) {
    if (!allow_declarations_) {
        set_error("WFC0027",
                  "declarations are not supported in conditional blocks",
                  statement_offset);
        return false;
    }
    skip_horizontal_whitespace();
    const auto name_offset = offset_;
    char type_character{};
    auto name = parse_identifier(&type_character);
    if (!name.has_value()) {
        set_error("WFC0118", "expected procedure name", name_offset);
        return false;
    }
    const auto definition = procedures_.find(*name);
    if (definition == procedures_.end()) {
        set_error("WFC0118", "expected procedure name", name_offset);
        return false;
    }
    offset_ = definition->second.declaration_end;
    return true;
}

}  // namespace wfc::detail
