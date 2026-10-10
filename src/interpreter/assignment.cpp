// Interpreter: Assignment, Set, Let, array elements, and Mid statements.
// Internal to the WFC evaluator; not part of the public API.
// Split out of src/evaluator.cpp; see src/interpreter/README.md.

#include "interpreter.hpp"

namespace wfc::detail {

std::optional<bool> Interpreter::parse_mid_statement(
    const std::size_t statement_offset) {
    const auto start = offset_;
    if (!consume_keyword("mid")) {
        return std::nullopt;
    }
    static_cast<void>(consume('$'));
    skip_horizontal_whitespace();
    if (!consume('(')) {
        offset_ = start;
        return std::nullopt;
    }
    skip_horizontal_whitespace();
    const auto variable_offset = offset_;
    char type_character{};
    auto name = parse_identifier(&type_character);
    if (!name.has_value()) {
        offset_ = start;
        return std::nullopt;
    }
    const auto variable = find_variable(*name);
    if (variable.value == nullptr) {
        set_error("WFC0015", "undeclared variable", variable_offset);
        return false;
    }
    skip_horizontal_whitespace();
    const auto long_argument = [&](Integer& out) -> bool {
        skip_horizontal_whitespace();
        const auto argument_offset = offset_;
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        if (!coerce_numeric_value(*value, Value{Integer{}}.index(),
                                  argument_offset) ||
            !std::holds_alternative<Integer>(*value)) {
            set_error("WFC0073", "Mid requires Long arguments",
                      argument_offset);
            return false;
        }
        out = std::get<Integer>(*value);
        return true;
    };
    Integer position{};
    Integer length = -1;
    if (!consume(',') || !long_argument(position)) {
        if (error_.diagnostic.empty()) {
            set_error("WFC0014", "expected start in Mid statement", offset_);
        }
        return false;
    }
    skip_horizontal_whitespace();
    if (consume(',') && !long_argument(length)) {
        return false;
    }
    skip_horizontal_whitespace();
    if (!consume(')')) {
        set_error("WFC0005", "expected closing parenthesis", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    if (!consume('=')) {
        set_error("WFC0014", "expected assignment operator", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    auto replacement = parse_expression();
    if (!replacement.has_value()) {
        return false;
    }
    if (!execute_) {
        return true;
    }
    auto* const target = std::get_if<std::string>(variable.value);
    const auto* text = std::get_if<std::string>(&*replacement);
    if (target == nullptr || text == nullptr) {
        set_error("WFC0016", "Mid statement requires String operands",
                  statement_offset);
        return false;
    }
    if (position < 1 || length < -1) {
        return raise_runtime(5, "Invalid procedure call or argument",
                             statement_offset);
    }
    const auto begin = static_cast<std::size_t>(position - 1);
    if (!is_ascii_text(*target) || !is_ascii_text(*text)) {
        auto target_units = to_utf16_units(*target);
        const auto text_units = to_utf16_units(*text);
        if (begin >= target_units.size()) {
            return true;
        }
        std::size_t unit_count =
            std::min(text_units.size(), target_units.size() - begin);
        if (length >= 0) {
            unit_count = std::min(unit_count, static_cast<std::size_t>(length));
        }
        target_units.replace(begin, unit_count, text_units, 0, unit_count);
        *target = from_utf16_units(target_units);
        return true;
    }
    if (begin >= target->size()) {
        return true;
    }
    std::size_t count = std::min(text->size(), target->size() - begin);
    if (length >= 0) {
        count = std::min(count, static_cast<std::size_t>(length));
    }
    target->replace(begin, count, *text, 0, count);
    return true;
}

bool Interpreter::parse_lvalue_path(LValue& result) {
    const auto variable_offset = offset_;
    char type_character{};
    auto name = parse_identifier(&type_character);
    if (!name.has_value()) {
        set_error("WFC0011", "expected variable name", variable_offset);
        return false;
    }
    const auto variable = find_variable(*name);
    if (variable.value == nullptr) {
        set_error("WFC0015", "undeclared variable", variable_offset);
        return false;
    }
    result.ptr = variable.value;
    result.variant = variable.scope->variant_variables.contains(*name);
    if (const auto fixed = variable.scope->fixed_string_lengths.find(*name);
        fixed != variable.scope->fixed_string_lengths.end()) {
        result.fixed = fixed->second;
    }
    while (true) {
        skip_horizontal_whitespace();
        if (at_end()) {
            return true;
        }
        if (current() == '(' && result.ptr != nullptr &&
            std::holds_alternative<ArrayValue>(*result.ptr)) {
            auto& array = std::get<ArrayValue>(*result.ptr);
            advance();
            auto indices =
                parse_index_list(array_expected_dimension_count(array));
            if (!indices.has_value()) {
                return false;
            }
            if (execute_) {
                const auto flat_offset = array_flat_offset(array, *indices);
                if (!flat_offset.has_value()) {
                    return false;
                }
                result.ptr = &array.elements[*flat_offset];
                result.fixed = 0;
                result.variant = false;
            } else {
                result.ptr = nullptr;
            }
            continue;
        }
        if (current() == '.') {
            advance();
            skip_horizontal_whitespace();
            char field_type_character{};
            const auto field_offset = offset_;
            auto field_name = parse_identifier(&field_type_character);
            if (!field_name.has_value()) {
                set_error("WFC0011", "expected member name after '.'",
                          field_offset);
                return false;
            }
            if (!execute_ || result.ptr == nullptr) {
                result.ptr = nullptr;
                continue;
            }
            auto* instance = std::get_if<ObjectInstance>(result.ptr);
            if (instance == nullptr) {
                set_error("WFC0136",
                          "member access requires an object reference",
                          field_offset);
                return false;
            }
            const auto field =
                instance->data->fields.variables.find(*field_name);
            if (field == instance->data->fields.variables.end()) {
                set_error("WFC0135", "unknown member", field_offset);
                return false;
            }
            result.ptr = &field->second;
            const auto fixed =
                instance->data->fields.fixed_string_lengths.find(*field_name);
            result.fixed =
                fixed != instance->data->fields.fixed_string_lengths.end()
                    ? fixed->second
                    : 0U;
            continue;
        }
        return true;
    }
}

bool Interpreter::append_statement_eligible(const std::string& identifier) {
    std::size_t i = offset_;
    for (const char expected : identifier) {
        if (i >= source_.size() || ascii_lower(source_[i]) != expected) {
            return false;
        }
        ++i;
    }
    if (i < source_.size() &&
        (is_identifier_part(source_[i]) || source_[i] == '.')) {
        return false;
    }
    while (i < source_.size() && (source_[i] == ' ' || source_[i] == '\t')) {
        ++i;
    }
    if (i >= source_.size() || source_[i] != '&') {
        return false;
    }
    ++i;
    if (i < source_.size() && source_[i] == '=') {
        return false;
    }
    const auto* const class_def = current_class_def();
    bool in_string = false;
    while (i < source_.size() && source_[i] != '\r' && source_[i] != '\n') {
        const char ch = source_[i];
        if (ch == '"') {
            in_string = !in_string;
            ++i;
            continue;
        }
        if (in_string) {
            ++i;
            continue;
        }
        if (ch == ':' || ch == '\'') {
            break;
        }
        if (ch == '=' || ch == '<' || ch == '>') {
            return false;
        }
        if (is_identifier_start(ch)) {
            const bool member_access = i > 0 && source_[i - 1] == '.';
            std::string word;
            while (i < source_.size() && is_identifier_part(source_[i])) {
                word.push_back(ascii_lower(source_[i]));
                ++i;
            }
            if (member_access) {
                continue;
            }
            static const std::set<std::string, std::less<>> excluded = {
                "and",        "or",           "xor",       "eqv",   "imp",
                "like",       "is",           "else",      "not",   "input",
                "rnd",        "then",         "to",        "step",  "typeof",
                "new",        "set",          "let",       "get",   "put",
                "callbyname", "createobject", "getobject", "shell", "inputb",
                "inputbox"};
            if (excluded.contains(word) || procedures_.contains(word) ||
                (class_def != nullptr && class_def->methods.contains(word))) {
                return false;
            }
            continue;
        }
        ++i;
    }
    return !in_string;
}

auto Interpreter::try_append_assignment(const std::string& identifier,
                                        std::string& target) -> AppendOutcome {
    const char* const key = source_.data() + offset_;
    auto cached = append_eligibility_.find(key);
    if (cached == append_eligibility_.end()) {
        cached = append_eligibility_
                     .emplace(key, append_statement_eligible(identifier))
                     .first;
    }
    if (!cached->second) {
        return AppendOutcome::not_applicable;
    }
    offset_ += identifier.size();
    skip_horizontal_whitespace();
    static_cast<void>(consume('&'));
    std::string appended;
    while (true) {
        skip_horizontal_whitespace();
        const auto operand_offset = offset_;
        auto operand = parse_additive();
        if (!operand.has_value()) {
            return AppendOutcome::failed;
        }
        if (!resolve_default_value(*operand, operand_offset)) {
            return AppendOutcome::failed;
        }
        if (is_object_reference(*operand) ||
            std::holds_alternative<ArrayValue>(*operand)) {
            set_error("WFC0020",
                      "concatenation requires String or Long operands",
                      operand_offset);
            return AppendOutcome::failed;
        }
        if (const auto* text = std::get_if<std::string>(&*operand)) {
            appended += *text;
        } else if (!std::holds_alternative<Null>(*operand)) {
            appended += render(*operand);
        }
        skip_horizontal_whitespace();
        if (!consume('&')) {
            break;
        }
    }
    const auto target_size = target.size();
    target += appended;
    merge_byte_halves(target, target_size);
    return AppendOutcome::done;
}

bool Interpreter::paren_followed_by_dot(
    const std::size_t open_offset) const noexcept {
    std::size_t depth = 0;
    bool in_string = false;
    for (std::size_t i = open_offset; i < source_.size(); ++i) {
        const char ch = source_[i];
        if (ch == '\r' || ch == '\n') {
            return false;
        }
        if (ch == '"') {
            in_string = !in_string;
        } else if (!in_string) {
            if (ch == '(') {
                ++depth;
            } else if (ch == ')') {
                if (--depth == 0) {
                    std::size_t k = i + 1;
                    while (k < source_.size() &&
                           (source_[k] == ' ' || source_[k] == '\t')) {
                        ++k;
                    }
                    return k < source_.size() && source_[k] == '.';
                }
            }
        }
    }
    return false;
}

bool Interpreter::parse_assignment(std::string identifier,
                                   const char type_character) {
    const auto identifier_offset =
        offset_ - identifier.size() - (type_character == '\0' ? 0U : 1U);
    const auto variable = find_variable(identifier);
    if (variable.value == nullptr) {
        // An unqualified write to a sibling Property Let of the class
        // currently executing -- the assignment counterpart of the
        // unqualified Property Get read/sibling method call
        // current_instance/current_class_def already give expressions
        // (see REQ-0203's parse_primary_base/parse_call_statement).
        if (type_character == '\0') {
            if (auto* const instance = current_instance()) {
                if (const auto* const class_def = current_class_def()) {
                    const auto letter_iterator =
                        class_def->property_let.find(identifier);
                    if (letter_iterator != class_def->property_let.end()) {
                        return invoke_property_let_or_set(
                            *instance, *class_def, letter_iterator->second,
                            identifier, identifier_offset);
                    }
                }
            }
        }
        if (type_character == '\0') {
            const auto letter = procedures_.find("wfclet_" + identifier);
            if (letter != procedures_.end()) {
                std::vector<CallArgument> arguments;
                skip_horizontal_whitespace();
                if (!at_end() && current() == '(') {
                    auto parsed = parse_call_argument_list();
                    if (!parsed.has_value()) {
                        return false;
                    }
                    arguments = std::move(*parsed);
                }
                skip_horizontal_whitespace();
                if (!consume('=')) {
                    set_error("WFC0014", "expected assignment operator",
                              offset_);
                    return false;
                }
                skip_horizontal_whitespace();
                auto value = parse_expression();
                if (!value.has_value()) {
                    return false;
                }
                arguments.push_back(CallArgument{std::move(*value), nullptr});
                return invoke_definition(letter->second, identifier,
                                         std::move(arguments),
                                         identifier_offset, source_, nullptr)
                    .has_value();
            }
        }
        if (!strict_here() && !in_with_identifier(identifier) &&
            !procedures_.contains(identifier)) {
            // REQ-0265: without Option Explicit an assignment declares
            // the variable implicitly (a Variant, or the suffix's type).
            Value initial = Value{Empty{}};
            bool variant = true;
            if (type_character != '\0') {
                initial =
                    zero_value_for_index(type_character_index(type_character));
                variant = false;
            } else if (const auto default_type = default_type_for(identifier)) {
                initial = zero_value_for_index(*default_type);
                variant = false;
            }
            current_scope().variables.emplace(identifier, std::move(initial));
            if (variant) {
                current_scope().variant_variables.insert(identifier);
            }
            return parse_assignment(std::move(identifier), type_character);
        }
        set_error("WFC0015", "undeclared variable", identifier_offset);
        return false;
    }
    if (!type_character_matches(*variable.value, type_character,
                                identifier_offset)) {
        return false;
    }
    if (variable.scope->constants.contains(identifier)) {
        set_error("WFC0062", "cannot assign to constant", identifier_offset);
        return false;
    }
    if (variable.scope->object_variables.contains(identifier)) {
        const auto declared =
            variable.scope->object_class_names.find(identifier);
        if (declared != variable.scope->object_class_names.end() &&
            is_udt_class(declared->second)) {
            skip_horizontal_whitespace();
            if (!consume('=')) {
                set_error("WFC0014", "expected assignment operator", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            auto source = parse_expression();
            if (!source.has_value()) {
                return false;
            }
            return assign_udt(*variable.value, *source, identifier_offset);
        }
        // `obj = value` assigns the class's default property (a Property
        // Let marked as the default member).
        if (declared != variable.scope->object_class_names.end()) {
            const auto declared_class =
                class_definitions_.find(declared->second);
            if (declared_class != class_definitions_.end() &&
                !declared_class->second.default_member.empty()) {
                const auto& class_def = declared_class->second;
                const auto letter =
                    class_def.property_let.find(class_def.default_member);
                if (letter != class_def.property_let.end()) {
                    if (!execute_) {
                        skip_horizontal_whitespace();
                        if (!consume('=')) {
                            set_error("WFC0014", "expected assignment operator",
                                      offset_);
                            return false;
                        }
                        skip_horizontal_whitespace();
                        return parse_expression().has_value();
                    }
                    if (const auto* instance =
                            std::get_if<ObjectInstance>(variable.value)) {
                        return invoke_property_let_or_set(
                            *instance->data, class_def, letter->second,
                            class_def.default_member, identifier_offset);
                    }
                    return raise_runtime(
                        91, "Object variable or With block variable not set",
                        identifier_offset);
                }
            }
        }
        set_error("WFC0108", "object assignment requires Set",
                  identifier_offset);
        return false;
    }

    skip_horizontal_whitespace();
    if (!consume('=')) {
        set_error("WFC0014", "expected assignment operator", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    if (execute_) {
        if (auto* const target = std::get_if<std::string>(variable.value);
            target != nullptr &&
            !variable.scope->fixed_string_lengths.contains(identifier)) {
            const auto outcome = try_append_assignment(identifier, *target);
            if (outcome != AppendOutcome::not_applicable) {
                return outcome == AppendOutcome::done;
            }
        }
    }
    auto value = parse_expression();
    if (!value.has_value()) {
        return false;
    }
    if (variable.scope->variant_variables.contains(identifier)) {
        // A Variant-declared variable freely accepts any value type,
        // retyping itself on each assignment (the agreed scalar-Variant
        // scope: no fixed-type enforcement for these variables).
        if (execute_) {
            // Assigning an object with a default property to a Variant
            // without Set takes the property's value.
            if (!resolve_default_value(*value, identifier_offset)) {
                return false;
            }
            if (!terminate_if_last_reference(*variable.value)) {
                return false;
            }
            *variable.value = copy_if_udt(std::move(*value));
        }
        return true;
    }
    // Byte() <-> String assignment copies the string's UCS-2 bytes (VB6
    // semantics).
    if (auto* target_array = std::get_if<ArrayValue>(variable.value);
        target_array != nullptr &&
        target_array->element_type_index == Value{Byte{}}.index() &&
        std::holds_alternative<std::string>(*value)) {
        if (execute_) {
            const auto& text = std::get<std::string>(*value);
            target_array->elements.clear();
            for (const char byte : string_to_bytes(text)) {
                target_array->elements.emplace_back(static_cast<Byte>(byte));
            }
            target_array->lower_bound = 0;
            target_array->dimensions.clear();
            target_array->is_allocated = true;
        }
        return true;
    }
    if (std::holds_alternative<std::string>(*variable.value)) {
        if (const auto* source_array = std::get_if<ArrayValue>(&*value);
            source_array != nullptr &&
            source_array->element_type_index == Value{Byte{}}.index()) {
            if (execute_) {
                std::u16string units;
                const auto byte_at = [&](const std::size_t i) -> unsigned {
                    const auto* byte =
                        std::get_if<Byte>(&source_array->elements[i]);
                    return byte != nullptr ? *byte : static_cast<unsigned>('?');
                };
                for (std::size_t i = 0; i < source_array->elements.size();
                     i += 2U) {
                    if (i + 1U < source_array->elements.size()) {
                        units.push_back(static_cast<char16_t>(
                            byte_at(i) | (byte_at(i + 1U) << 8U)));
                    } else {
                        units.push_back(
                            static_cast<char16_t>(0xF700U | byte_at(i)));
                    }
                }
                *variable.value = from_utf16_units(units);
            }
            return true;
        }
    }
    if (!coerce_numeric_value(*value, variable.value->index(),
                              identifier_offset)) {
        return false;
    }
    if (variable.value->index() != value->index()) {
        set_error("WFC0016", "assignment type mismatch", identifier_offset);
        return false;
    }
    if (execute_) {
        if (const auto fixed =
                variable.scope->fixed_string_lengths.find(identifier);
            fixed != variable.scope->fixed_string_lengths.end()) {
            if (auto* text = std::get_if<std::string>(&*value)) {
                fit_to_units(*text, fixed->second);
            }
        }
        if (std::holds_alternative<ArrayValue>(*value)) {
            deep_copy_udt_values(*value);
        }
        // Assigning an array to a dynamic array leaves the target dynamic
        // (resizable).
        const bool target_was_dynamic = [&] {
            const auto* previous = std::get_if<ArrayValue>(variable.value);
            return previous != nullptr && previous->is_dynamic;
        }();
        *variable.value = std::move(*value);
        if (target_was_dynamic) {
            std::get<ArrayValue>(*variable.value).is_dynamic = true;
        }
    }
    return true;
}

bool Interpreter::class_satisfies(
    const std::string& actual_class_name,
    const std::string& declared_class_name) const {
    if (actual_class_name == declared_class_name) {
        return true;
    }
    // A built-in class registered under its public name (`As Dictionary`)
    // accepts the instance CreateObject builds under its internal `Wfc` name.
    if ("wfc" + declared_class_name == actual_class_name ||
        "wfc" + actual_class_name == declared_class_name) {
        return true;
    }
    const auto class_iterator = class_definitions_.find(actual_class_name);
    if (class_iterator == class_definitions_.end()) {
        return false;
    }
    for (const auto& implemented : class_iterator->second.implements) {
        if (implemented == declared_class_name) {
            return true;
        }
    }
    return false;
}

bool Interpreter::assign_object_reference(
    Value& target, const std::string& declared_class_name, Value source,
    const std::size_t offset) {
    if (execute_ && !std::holds_alternative<Nothing>(source) &&
        !std::holds_alternative<ObjectInstance>(source)) {
        set_error("WFC0106", "Set requires an object reference", offset);
        return false;
    }
    if (!declared_class_name.empty() &&
        std::holds_alternative<ObjectInstance>(source) &&
        !class_satisfies(std::get<ObjectInstance>(source).data->class_name,
                         declared_class_name)) {
        set_error("WFC0137",
                  "Set source does not match the target's declared class",
                  offset);
        return false;
    }
    if (execute_) {
        if (!terminate_if_last_reference(target)) {
            return false;
        }
        target = std::move(source);
    }
    return true;
}

bool Interpreter::invoke_property_let_or_set(
    InstanceData& instance, const ClassDef& class_def,
    const ProcedureDef& definition, const std::string& property_name,
    const std::size_t property_offset) {
    std::vector<CallArgument> arguments;
    skip_horizontal_whitespace();
    if (!at_end() && current() == '(') {
        auto parsed = parse_call_argument_list();
        if (!parsed.has_value()) {
            return false;
        }
        arguments = std::move(*parsed);
    }
    skip_horizontal_whitespace();
    if (!consume('=')) {
        set_error("WFC0014", "expected assignment operator", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    auto value = parse_expression();
    if (!value.has_value()) {
        return false;
    }
    // A Let (unlike a Set) passes an object's default value.
    const auto let_found = class_def.property_let.find(property_name);
    if (let_found != class_def.property_let.end() &&
        &let_found->second == &definition &&
        !resolve_default_value(*value, property_offset)) {
        return false;
    }
    arguments.push_back(CallArgument{std::move(*value), nullptr});
    auto result =
        invoke_definition(definition, property_name, std::move(arguments),
                          property_offset, class_def.source, &instance);
    return result.has_value();
}

bool Interpreter::parse_member_set_assignment(const Value base,
                                              const std::size_t base_offset) {
    skip_horizontal_whitespace();
    const auto member_offset = offset_;
    char type_character{};
    auto member_name = parse_identifier(&type_character);
    if (!member_name.has_value() || type_character != '\0') {
        set_error("WFC0011", "expected member name after '.'", member_offset);
        return false;
    }
    if (std::holds_alternative<Nothing>(base)) {
        if (!execute_) {
            // Dry-run parsing of a not-taken branch (REQ-0229): as
            // parse_member_access_after_dot's identical case explains,
            // there is no class to resolve `.member` against at all
            // when `base` is genuinely `Nothing`. Still parses through
            // the `= expression` that must follow, so the source text
            // and the RHS's own shape are validated, but assigns
            // nothing (there is nowhere to assign it to).
            skip_horizontal_whitespace();
            if (!consume('=')) {
                set_error("WFC0014", "expected assignment operator", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            return parse_expression().has_value();
        }
        set_error("WFC0106", "Invalid use of Nothing", base_offset);
        return false;
    }
    InstanceData& instance = *std::get<ObjectInstance>(base).data;
    if (instance.com != nullptr) {
        return com_set_member(base, *member_name, member_offset);
    }
    const auto class_iterator = class_definitions_.find(instance.class_name);
    const ClassDef& class_def = class_iterator->second;
    const auto setter_iterator = class_def.property_set.find(*member_name);
    if (setter_iterator != class_def.property_set.end()) {
        if (!member_accessible(class_def, setter_iterator->second.is_private)) {
            set_error("WFC0142", "member is not accessible outside its class",
                      member_offset);
            return false;
        }
        return invoke_property_let_or_set(instance, class_def,
                                          setter_iterator->second, *member_name,
                                          member_offset);
    }
    const auto field_iterator = instance.fields.variables.find(*member_name);
    if (field_iterator == instance.fields.variables.end() ||
        (!instance.fields.object_variables.contains(*member_name) &&
         !instance.fields.variant_variables.contains(*member_name))) {
        set_error("WFC0135", "unknown member (no Property Set accessor)",
                  member_offset);
        return false;
    }
    const auto field_def_iterator = class_def.fields.find(*member_name);
    if (field_def_iterator != class_def.fields.end() &&
        !member_accessible(class_def, field_def_iterator->second.is_private)) {
        set_error("WFC0142", "member is not accessible outside its class",
                  member_offset);
        return false;
    }
    skip_horizontal_whitespace();
    if (!at_end() && current() == '.') {
        // Chained `Set a.b.c = x`: `a.b` holds the object `c` is set on.
        const Value& next_base = field_iterator->second;
        if (!std::holds_alternative<ObjectInstance>(next_base) &&
            !std::holds_alternative<Nothing>(next_base)) {
            set_error("WFC0136", "member access requires an object reference",
                      member_offset);
            return false;
        }
        advance();
        return parse_member_set_assignment(next_base, member_offset);
    }
    if (!consume('=')) {
        set_error("WFC0014", "expected assignment operator", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    auto value = parse_expression();
    if (!value.has_value()) {
        return false;
    }
    const auto declared_class =
        instance.fields.object_class_names.find(*member_name);
    const std::string declared_class_name =
        declared_class != instance.fields.object_class_names.end()
            ? declared_class->second
            : std::string{};
    return assign_field_reference(instance, *member_name,
                                  field_iterator->second, declared_class_name,
                                  std::move(*value), member_offset);
}

bool Interpreter::parse_set_statement() {
    skip_horizontal_whitespace();
    const auto identifier_offset = offset_;
    char type_character{};
    auto identifier = parse_identifier(&type_character);
    if (!identifier.has_value()) {
        set_error("WFC0011", "expected variable name after Set",
                  identifier_offset);
        return false;
    }
    // `Set Me.Prop = expression`.
    if (type_character == '\0' && *identifier == "me") {
        skip_horizontal_whitespace();
        if (!at_end() && current() == '.') {
            auto base = me_value(identifier_offset);
            if (!base.has_value()) {
                return false;
            }
            advance();
            return parse_member_set_assignment(*base, identifier_offset);
        }
        set_error("WFC0011", "expected member name after '.'", offset_);
        return false;
    }
    auto variable = find_variable_raw(*identifier);
    {
        // `Set a.b = x` / `Set a(i) = x` use `a`'s object, so an auto-new
        // variable is created first; `Set a = x` replaces it untouched.
        std::size_t look = offset_;
        while (look < source_.size() &&
               (source_[look] == ' ' || source_[look] == '\t')) {
            ++look;
        }
        if (look < source_.size() &&
            (source_[look] == '.' || source_[look] == '(')) {
            variable = find_variable(*identifier);
        }
    }
    if (variable.value == nullptr) {
        // An unqualified `Set Prop = expr` for a sibling Property Set
        // of the class currently executing -- the Set counterpart of
        // parse_assignment's unqualified Property Let write (REQ-0205).
        if (type_character == '\0') {
            if (auto* const instance = current_instance()) {
                if (const auto* const class_def = current_class_def()) {
                    const auto setter_iterator =
                        class_def->property_set.find(*identifier);
                    if (setter_iterator != class_def->property_set.end()) {
                        return invoke_property_let_or_set(
                            *instance, *class_def, setter_iterator->second,
                            *identifier, identifier_offset);
                    }
                }
            }
        }
        if (type_character == '\0') {
            const auto setter = procedures_.find("wfcset_" + *identifier);
            if (setter != procedures_.end()) {
                std::vector<CallArgument> arguments;
                skip_horizontal_whitespace();
                if (!at_end() && current() == '(') {
                    auto parsed = parse_call_argument_list();
                    if (!parsed.has_value()) {
                        return false;
                    }
                    arguments = std::move(*parsed);
                }
                skip_horizontal_whitespace();
                if (!consume('=')) {
                    set_error("WFC0014", "expected assignment operator",
                              offset_);
                    return false;
                }
                skip_horizontal_whitespace();
                auto value = parse_expression();
                if (!value.has_value()) {
                    return false;
                }
                arguments.push_back(CallArgument{std::move(*value), nullptr});
                return invoke_definition(setter->second, *identifier,
                                         std::move(arguments),
                                         identifier_offset, source_, nullptr)
                    .has_value();
            }
        }
        set_error("WFC0015", "undeclared variable", identifier_offset);
        return false;
    }

    const auto after_identifier_offset = offset_;
    skip_horizontal_whitespace();
    if (type_character == '\0' && !at_end() && current() == '(' &&
        std::holds_alternative<ObjectInstance>(*variable.value)) {
        auto& instance_data = *std::get<ObjectInstance>(*variable.value).data;
        const auto class_iterator =
            class_definitions_.find(instance_data.class_name);
        if (class_iterator != class_definitions_.end() &&
            !class_iterator->second.default_member.empty()) {
            const auto& class_def = class_iterator->second;
            const auto setter =
                class_def.property_set.find(class_def.default_member);
            if (setter != class_def.property_set.end()) {
                return invoke_property_let_or_set(
                    instance_data, class_def, setter->second,
                    class_def.default_member, identifier_offset);
            }
        }
    }
    if (type_character == '\0' && !at_end() && current() == '.' &&
        (std::holds_alternative<Nothing>(*variable.value) ||
         std::holds_alternative<ObjectInstance>(*variable.value))) {
        const auto base = *variable.value;
        advance();
        return parse_member_set_assignment(base, identifier_offset);
    }
    // `Set arrayName(index [, index...]) = expression`, the Object-
    // element-array (REQ-0212) counterpart of a plain Object variable's
    // `Set name = expression`.
    if (type_character == '\0' && !at_end() && current() == '(' &&
        std::holds_alternative<ArrayValue>(*variable.value)) {
        auto& array = std::get<ArrayValue>(*variable.value);
        if (!array.is_object_element && !array.is_variant_element) {
            set_error("WFC0109", "Set requires an Object or Variant target",
                      identifier_offset);
            return false;
        }
        advance();  // consume '('
        const auto dimension_count = array_expected_dimension_count(array);
        auto indices = parse_index_list(dimension_count);
        if (!indices.has_value()) {
            return false;
        }
        skip_horizontal_whitespace();
        if (!consume('=')) {
            set_error("WFC0014", "expected assignment operator", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        if (!execute_) {
            return true;
        }
        const auto flat_offset = array_flat_offset(array, *indices);
        if (!flat_offset.has_value()) {
            return false;
        }
        return assign_object_reference(array.elements[*flat_offset],
                                       array.element_class_name,
                                       std::move(*value), identifier_offset);
    }
    offset_ = after_identifier_offset;

    if (!type_character_matches(*variable.value, type_character,
                                identifier_offset)) {
        return false;
    }
    if (variable.scope->constants.contains(*identifier)) {
        set_error("WFC0062", "cannot assign to constant", identifier_offset);
        return false;
    }
    if (!variable.scope->object_variables.contains(*identifier) &&
        !variable.scope->variant_variables.contains(*identifier)) {
        set_error("WFC0109", "Set requires an Object or Variant target",
                  identifier_offset);
        return false;
    }

    skip_horizontal_whitespace();
    if (!consume('=')) {
        set_error("WFC0014", "expected assignment operator", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    auto value = parse_expression();
    if (!value.has_value()) {
        return false;
    }
    const auto declared_class =
        variable.scope->object_class_names.find(*identifier);
    const std::string declared_class_name =
        declared_class != variable.scope->object_class_names.end()
            ? declared_class->second
            : std::string{};
    InstanceData* const owner = current_instance();
    if (owner != nullptr && &owner->fields == variable.scope) {
        return assign_field_reference(*owner, *identifier, *variable.value,
                                      declared_class_name, std::move(*value),
                                      identifier_offset);
    }
    return assign_object_reference(*variable.value, declared_class_name,
                                   std::move(*value), identifier_offset);
}

bool Interpreter::assign_field_reference(InstanceData& owner,
                                         const std::string& field_name,
                                         Value& slot,
                                         const std::string& declared_class_name,
                                         Value source,
                                         const std::size_t offset) {
    bool with_events = false;
    if (const auto class_iterator = class_definitions_.find(owner.class_name);
        class_iterator != class_definitions_.end()) {
        const auto field_def = class_iterator->second.fields.find(field_name);
        with_events = field_def != class_iterator->second.fields.end() &&
                      field_def->second.with_events;
    }
    if (!with_events || !execute_) {
        return assign_object_reference(slot, declared_class_name,
                                       std::move(source), offset);
    }
    const auto unsubscribe = [&]() {
        if (const auto* old_instance = std::get_if<ObjectInstance>(&slot)) {
            auto& sinks = old_instance->data->event_sinks;
            std::erase_if(sinks, [&](const auto& entry) {
                return entry.second == field_name &&
                       entry.first.lock().get() == &owner;
            });
        }
    };
    unsubscribe();
    const std::shared_ptr<InstanceData> new_data =
        std::holds_alternative<ObjectInstance>(source)
            ? std::get<ObjectInstance>(source).data
            : nullptr;
    if (!assign_object_reference(slot, declared_class_name, std::move(source),
                                 offset)) {
        return false;
    }
    if (new_data != nullptr) {
        new_data->event_sinks.emplace_back(owner.weak_from_this(), field_name);
    }
    return true;
}

bool Interpreter::parse_member_assignment(
    const Value base, const std::size_t base_offset,
    const std::string& via_interface_class) {
    skip_horizontal_whitespace();
    const auto member_offset = offset_;
    char type_character{};
    auto member_name = parse_identifier(&type_character);
    if (!member_name.has_value() || type_character != '\0') {
        set_error("WFC0011", "expected member name after '.'", member_offset);
        return false;
    }
    if (std::holds_alternative<Nothing>(base)) {
        if (!execute_) {
            // Dry-run parsing of a not-taken branch (REQ-0229): as
            // parse_member_access_after_dot's identical case explains,
            // there is no class to resolve `.member` against at all
            // when `base` is genuinely `Nothing`. Still parses through
            // the `= expression` that must follow, so the source text
            // and the RHS's own shape are validated, but assigns
            // nothing (there is nowhere to assign it to).
            skip_horizontal_whitespace();
            // Skip a longer chain (`o.Child.Prop(1).Name = x`) down to its last
            // member.
            while (!at_end() && (current() == '.' || current() == '(')) {
                if (consume('.')) {
                    skip_horizontal_whitespace();
                    char chain_type_character{};
                    if (!parse_identifier(&chain_type_character).has_value()) {
                        set_error("WFC0011", "expected member name after '.'",
                                  offset_);
                        return false;
                    }
                } else {
                    advance();
                    if (!parse_index_list(kAnyDimensionCount).has_value()) {
                        return false;
                    }
                }
                skip_horizontal_whitespace();
            }
            if (!consume('=')) {
                if (at_statement_end() || current() != '=') {
                    // A call statement `obj.Method args`: parse (and ignore)
                    // the arguments.
                    skip_horizontal_whitespace();
                    while (!at_statement_end()) {
                        if (!parse_expression().has_value()) {
                            return false;
                        }
                        skip_horizontal_whitespace();
                        if (!consume(',')) {
                            break;
                        }
                        skip_horizontal_whitespace();
                    }
                    return true;
                }
                set_error("WFC0014", "expected assignment operator", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            return parse_expression().has_value();
        }
        set_error("WFC0106", "Invalid use of Nothing", base_offset);
        return false;
    }
    InstanceData& instance = *std::get<ObjectInstance>(base).data;
    if (instance.com != nullptr) {
        return com_member_statement(base, *member_name, member_offset);
    }
    const auto class_iterator = class_definitions_.find(instance.class_name);
    const ClassDef& class_def = class_iterator->second;

    // `obj.Prop(args).member = x` / `obj.Prop.member = x`: read the property or
    // method first, then write the member of the object it returns.
    if (!instance.fields.variables.contains(*member_name) &&
        !class_def.property_let.contains(*member_name) &&
        (class_def.property_get.contains(*member_name) ||
         class_def.methods.contains(*member_name))) {
        const auto after_member = offset_;
        skip_horizontal_whitespace();
        if (!at_end() &&
            (current() == '.' ||
             (current() == '(' && paren_followed_by_dot(offset_)))) {
            offset_ = member_offset;
            auto chained = parse_member_access_after_dot(
                base, base_offset, /*require_function=*/true,
                via_interface_class);
            if (!chained.has_value()) {
                return false;
            }
            skip_horizontal_whitespace();
            if (!at_end() && current() == '.') {
                const std::string temp_name =
                    "wfcchain" + std::to_string(chain_counter_++);
                current_scope().variables.insert_or_assign(
                    temp_name, std::holds_alternative<ObjectInstance>(*chained)
                                   ? *chained
                                   : Value{Nothing{}});
                const bool chained_ok =
                    parse_assignment_or_array_element(temp_name, '\0');
                current_scope().variables.erase(temp_name);
                return chained_ok;
            }
            set_error("WFC0014", "expected assignment operator", offset_);
            return false;
        }
        offset_ = after_member;
    }

    // REQ-0233: through an interface-typed reference the implementing
    // class's `Interface_Member` accessor is the target.
    if (!via_interface_class.empty()) {
        const std::string prefixed = via_interface_class + "_" + *member_name;
        const bool implements_interface =
            std::find(class_def.implements.begin(), class_def.implements.end(),
                      via_interface_class) != class_def.implements.end();
        if (implements_interface && class_def.property_let.contains(prefixed)) {
            return invoke_property_let_or_set(
                instance, class_def, class_def.property_let.at(prefixed),
                prefixed, member_offset);
        }
    }
    const auto letter_iterator = class_def.property_let.find(*member_name);
    if (letter_iterator != class_def.property_let.end()) {
        if (!member_accessible(class_def, letter_iterator->second.is_private)) {
            set_error("WFC0142", "member is not accessible outside its class",
                      member_offset);
            return false;
        }
        return invoke_property_let_or_set(instance, class_def,
                                          letter_iterator->second, *member_name,
                                          member_offset);
    }

    const auto field_iterator = instance.fields.variables.find(*member_name);
    if (field_iterator == instance.fields.variables.end()) {
        set_error("WFC0135", "unknown member", member_offset);
        return false;
    }
    const auto field_def_iterator = class_def.fields.find(*member_name);
    if (field_def_iterator != class_def.fields.end() &&
        !member_accessible(class_def, field_def_iterator->second.is_private)) {
        set_error("WFC0142", "member is not accessible outside its class",
                  member_offset);
        return false;
    }
    if (std::holds_alternative<ArrayValue>(field_iterator->second)) {
        skip_horizontal_whitespace();
        if (!at_end() && current() == '(') {
            return parse_array_element_assignment_on(field_iterator->second,
                                                     member_offset);
        }
    }
    {
        // Chained write (`o.i.tag = 9`): the field holds the object the
        // next `.member` is written on, so recurse with it as the base.
        const auto chain_offset = offset_;
        skip_horizontal_whitespace();
        if (!at_end() && current() == '.') {
            const Value& next_base = field_iterator->second;
            if (!std::holds_alternative<ObjectInstance>(next_base) &&
                !std::holds_alternative<Nothing>(next_base)) {
                set_error("WFC0136",
                          "member access requires an object reference",
                          member_offset);
                return false;
            }
            ++offset_;
            return parse_member_assignment(next_base, member_offset);
        }
        offset_ = chain_offset;
    }
    if (instance.fields.object_variables.contains(*member_name)) {
        const auto declared =
            instance.fields.object_class_names.find(*member_name);
        if (declared != instance.fields.object_class_names.end() &&
            is_udt_class(declared->second)) {
            skip_horizontal_whitespace();
            if (!consume('=')) {
                set_error("WFC0014", "expected assignment operator", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            auto source = parse_expression();
            if (!source.has_value()) {
                return false;
            }
            return assign_udt(field_iterator->second, *source, member_offset);
        }
        set_error("WFC0108", "object assignment requires Set", member_offset);
        return false;
    }
    skip_horizontal_whitespace();
    if (!consume('=')) {
        set_error("WFC0014", "expected assignment operator", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    auto value = parse_expression();
    if (!value.has_value()) {
        return false;
    }
    if (instance.fields.variant_variables.contains(*member_name)) {
        if (execute_) {
            if (!terminate_if_last_reference(field_iterator->second)) {
                return false;
            }
            field_iterator->second = copy_if_udt(std::move(*value));
        }
        return true;
    }
    if (!coerce_numeric_value(*value, field_iterator->second.index(),
                              member_offset)) {
        return false;
    }
    if (field_iterator->second.index() != value->index()) {
        set_error("WFC0016", "assignment type mismatch", member_offset);
        return false;
    }
    if (execute_) {
        if (const auto fixed =
                instance.fields.fixed_string_lengths.find(*member_name);
            fixed != instance.fields.fixed_string_lengths.end()) {
            if (auto* text = std::get_if<std::string>(&*value)) {
                fit_to_units(*text, fixed->second);
            }
        }
        field_iterator->second = std::move(*value);
    }
    return true;
}

bool Interpreter::parse_assignment_or_array_element(std::string identifier,
                                                    const char type_character) {
    if (type_character == '\0' && identifier == "me") {
        const auto identifier_offset = offset_ - identifier.size();
        const auto saved_offset = offset_;
        skip_horizontal_whitespace();
        if (!at_end() && current() == '.') {
            auto base = me_value(identifier_offset);
            if (!base.has_value()) {
                return false;
            }
            advance();
            if (const auto* const me_instance =
                    std::get_if<ObjectInstance>(&*base)) {
                // `Me.Method args` with no `Call`.
                const auto& me_class =
                    class_definitions_.at(me_instance->data->class_name);
                const auto peek_offset = offset_;
                char peek_type_character{};
                auto peek_member_name = parse_identifier(&peek_type_character);
                skip_horizontal_whitespace();
                const bool bare_statement_end =
                    at_end() || current() == '\r' || current() == '\n' ||
                    current() == ':' || current() == '\'';
                const bool arguments_follow =
                    !bare_statement_end && current() != '=' &&
                    current() != '(' && current() != '.';
                const bool is_method =
                    peek_member_name.has_value() &&
                    peek_type_character == '\0' &&
                    me_class.methods.contains(*peek_member_name);
                offset_ = peek_offset;
                if ((bare_statement_end || arguments_follow) && is_method) {
                    bare_call_arguments_ = arguments_follow;
                    const auto result = parse_member_access_after_dot(
                        *base, identifier_offset, /*require_function=*/false);
                    return result.has_value();
                }
            }
            return parse_member_assignment(*base, identifier_offset);
        }
        offset_ = saved_offset;
    }
    if (type_character == '\0') {
        const auto variable = find_variable(identifier);
        if (variable.value != nullptr &&
            std::holds_alternative<ArrayValue>(*variable.value)) {
            const auto saved_offset = offset_;
            skip_horizontal_whitespace();
            if (!at_end() && current() == '(') {
                const auto identifier_offset = saved_offset - identifier.size();
                return parse_array_element_assignment(identifier,
                                                      identifier_offset);
            }
            offset_ = saved_offset;
        }
        if (variable.value != nullptr &&
            (std::holds_alternative<ObjectInstance>(*variable.value) ||
             (!execute_ && std::holds_alternative<Nothing>(*variable.value)))) {
            // `obj(args).member ...` (for example `items(1).Name = x` on a
            // Collection): run the default-member call, then treat its object
            // as the statement's base.
            {
                const auto call_offset = offset_;
                skip_horizontal_whitespace();
                if (!at_end() && current() == '(' &&
                    paren_followed_by_dot(offset_)) {
                    offset_ = call_offset - identifier.size();
                    auto chained = parse_primary_base();
                    if (!chained.has_value()) {
                        return false;
                    }
                    skip_horizontal_whitespace();
                    if (at_end() || current() != '.') {
                        set_error("WFC0004", "unexpected trailing input",
                                  offset_);
                        return false;
                    }
                    const std::string temp_name =
                        "wfcchain" + std::to_string(chain_counter_++);
                    current_scope().variables.insert_or_assign(
                        temp_name,
                        std::holds_alternative<ObjectInstance>(*chained)
                            ? *chained
                            : Value{Nothing{}});
                    const bool chained_ok =
                        parse_assignment_or_array_element(temp_name, '\0');
                    current_scope().variables.erase(temp_name);
                    return chained_ok;
                }
                offset_ = call_offset;
            }
        }
        if (variable.value != nullptr &&
            std::holds_alternative<ObjectInstance>(*variable.value)) {
            // `obj(args) = value` through the class's default member
            // (a Property Let).
            const auto saved_offset = offset_;
            skip_horizontal_whitespace();
            if (!at_end() && current() == '(') {
                auto& instance_data =
                    *std::get<ObjectInstance>(*variable.value).data;
                if (instance_data.com != nullptr) {
                    return com_member_statement(*variable.value, std::string{},
                                                saved_offset);
                }
                const auto class_iterator =
                    class_definitions_.find(instance_data.class_name);
                if (class_iterator != class_definitions_.end() &&
                    !class_iterator->second.default_member.empty()) {
                    const auto& class_def = class_iterator->second;
                    const auto letter =
                        class_def.property_let.find(class_def.default_member);
                    if (letter != class_def.property_let.end()) {
                        return invoke_property_let_or_set(
                            instance_data, class_def, letter->second,
                            class_def.default_member, saved_offset);
                    }
                }
            }
            offset_ = saved_offset;
        }
        if (variable.value != nullptr &&
            (std::holds_alternative<Nothing>(*variable.value) ||
             std::holds_alternative<ObjectInstance>(*variable.value))) {
            const auto saved_offset = offset_;
            skip_horizontal_whitespace();
            if (!at_end() && current() == '.') {
                const auto identifier_offset = saved_offset - identifier.size();
                const auto base = *variable.value;
                advance();
                // `obj.Method` with no `Call` and no parentheses (zero
                // arguments; REQ-0217): checked only when nothing else
                // follows the bare member name on this statement (no
                // `=`, no `(`), so it never competes with `obj.Prop =
                // expr` or `obj.Method(args) = ...` (an indexed
                // Property Let/Set), both still handled by
                // parse_member_assignment below.
                if (const auto* const instance_base =
                        std::get_if<ObjectInstance>(&base)) {
                    // REQ-0233: `obj`'s own declared class, so a
                    // bare `obj.Method` reaching an interface member
                    // is recognized here the same way `obj.Method`
                    // through `parse_primary`'s expression path
                    // already is -- checked against the
                    // interface-prefixed name below, not just the
                    // bare one.
                    std::string declared_interface_class;
                    const auto declared_class =
                        variable.scope->object_class_names.find(identifier);
                    if (declared_class !=
                        variable.scope->object_class_names.end()) {
                        declared_interface_class = declared_class->second;
                    }
                    const auto& instance_class_def =
                        class_definitions_.at(instance_base->data->class_name);
                    bool implements_interface = false;
                    for (const auto& implemented :
                         instance_class_def.implements) {
                        if (implemented == declared_interface_class) {
                            implements_interface = true;
                            break;
                        }
                    }
                    const auto peek_offset = offset_;
                    char peek_type_character{};
                    auto peek_member_name =
                        parse_identifier(&peek_type_character);
                    skip_horizontal_whitespace();
                    const bool bare_statement_end =
                        at_end() || current() == '\r' || current() == '\n' ||
                        current() == ':' || current() == '\'';
                    const bool arguments_follow =
                        !bare_statement_end && current() != '=' &&
                        current() != '(' && current() != '.';
                    const bool is_method =
                        peek_member_name.has_value() &&
                        peek_type_character == '\0' &&
                        (instance_class_def.methods.contains(
                             *peek_member_name) ||
                         (implements_interface &&
                          instance_class_def.methods.contains(
                              declared_interface_class + "_" +
                              *peek_member_name)));
                    ParenGroup paren_group;
                    if (is_method && !at_end() && current() == '(') {
                        paren_group = scan_statement_paren_group(offset_);
                    }
                    offset_ = peek_offset;
                    if (paren_group.ends_statement && is_method) {
                        bare_call_arguments_ = !paren_group.is_list;
                        const auto result = parse_member_access_after_dot(
                            base, identifier_offset, /*require_function=*/false,
                            declared_interface_class);
                        bare_call_arguments_ = false;
                        return result.has_value();
                    }
                    if ((bare_statement_end || arguments_follow) && is_method) {
                        bare_call_arguments_ = arguments_follow;
                        const auto result = parse_member_access_after_dot(
                            base, identifier_offset, /*require_function=*/false,
                            declared_interface_class);
                        return result.has_value();
                    }
                }
                const auto declared_for_assignment =
                    variable.scope->object_class_names.find(identifier);
                return parse_member_assignment(
                    base, identifier_offset,
                    declared_for_assignment !=
                            variable.scope->object_class_names.end()
                        ? declared_for_assignment->second
                        : std::string{});
            }
            offset_ = saved_offset;
        }
    }
    return parse_assignment(std::move(identifier), type_character);
}

std::size_t Interpreter::array_expected_dimension_count(
    const ArrayValue& array) noexcept {
    if (!array.dimensions.empty()) {
        return array.dimensions.size();
    }
    if (!array.is_allocated && array.dynamic_dimension_count > 0U) {
        return array.dynamic_dimension_count;
    }
    return 1U;
}

std::optional<std::vector<std::pair<Integer, std::size_t>>>
Interpreter::parse_index_list(const std::size_t dimension_count) {
    std::vector<std::pair<Integer, std::size_t>> indices;
    while (true) {
        skip_horizontal_whitespace();
        const auto index_offset = offset_;
        auto index_value = parse_expression();
        if (!index_value.has_value()) {
            return std::nullopt;
        }
        const auto index = coerce_long(*index_value, index_offset);
        if (!index.has_value()) {
            return std::nullopt;
        }
        indices.emplace_back(*index, index_offset);
        skip_horizontal_whitespace();
        if (!at_end() && current() == ',') {
            advance();
            continue;
        }
        break;
    }
    if (!consume(')')) {
        set_error("WFC0005", "expected closing parenthesis", offset_);
        return std::nullopt;
    }
    if (dimension_count != kAnyDimensionCount &&
        indices.size() != dimension_count) {
        set_error("WFC0115", "index count does not match array dimensions",
                  indices.front().second);
        return std::nullopt;
    }
    return indices;
}

std::optional<std::size_t> Interpreter::array_flat_offset(
    const ArrayValue& array,
    const std::vector<std::pair<Integer, std::size_t>>& indices) {
    if (array.dimensions.empty()) {
        const Integer upper_bound =
            array.lower_bound + static_cast<Integer>(array.elements.size()) - 1;
        if (indices[0].first < array.lower_bound ||
            indices[0].first > upper_bound) {
            set_error("WFC0111", "array subscript out of range",
                      indices[0].second);
            return std::nullopt;
        }
        return static_cast<std::size_t>(indices[0].first - array.lower_bound);
    }
    std::size_t flat_offset = 0;
    for (std::size_t dim = 0; dim < array.dimensions.size(); ++dim) {
        const auto& dimension_bound = array.dimensions[dim];
        const auto& index_entry = indices[dim];
        if (index_entry.first < dimension_bound.first ||
            index_entry.first > dimension_bound.second) {
            set_error("WFC0111", "array subscript out of range",
                      index_entry.second);
            return std::nullopt;
        }
        const auto dimension_size =
            static_cast<std::size_t>(dimension_bound.second -
                                     dimension_bound.first) +
            1U;
        flat_offset =
            flat_offset * dimension_size +
            static_cast<std::size_t>(index_entry.first - dimension_bound.first);
    }
    return flat_offset;
}

bool Interpreter::parse_array_element_assignment(
    const std::string& identifier, const std::size_t identifier_offset) {
    const auto variable = find_variable(identifier);
    return parse_array_element_assignment_on(*variable.value,
                                             identifier_offset);
}

bool Interpreter::parse_array_element_assignment_on(
    Value& slot, const std::size_t identifier_offset) {
    advance();  // consume '('
    auto& array = std::get<ArrayValue>(slot);
    const auto dimension_count = array_expected_dimension_count(array);
    auto indices = parse_index_list(dimension_count);
    if (!indices.has_value()) {
        return false;
    }
    // `v(0).Name = x`: write a member of the object a Variant element holds.
    if (array.is_variant_element && !at_end() && current() == '.') {
        Value base{Nothing{}};
        if (execute_) {
            const auto flat_offset = array_flat_offset(array, *indices);
            if (!flat_offset.has_value()) {
                return false;
            }
            base = array.elements[*flat_offset];
            if (!std::holds_alternative<ObjectInstance>(base)) {
                set_error("WFC0136",
                          "member access requires an object reference",
                          identifier_offset);
                return false;
            }
        }
        advance();
        return parse_member_assignment(base, identifier_offset);
    }
    // `jag(1)(2) = x`: assign into an array held by a Variant element.
    if (array.is_variant_element && !at_end() && current() == '(') {
        if (!execute_) {
            while (!at_end() && current() == '(') {
                advance();
                if (!parse_index_list(kAnyDimensionCount).has_value()) {
                    return false;
                }
            }
            skip_horizontal_whitespace();
            if (!consume('=')) {
                set_error("WFC0014", "expected assignment operator", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            return parse_expression().has_value();
        }
        const auto flat_offset = array_flat_offset(array, *indices);
        if (!flat_offset.has_value()) {
            return false;
        }
        Value& inner = array.elements[*flat_offset];
        if (!std::holds_alternative<ArrayValue>(inner)) {
            set_error("WFC0136", "indexing requires an array",
                      identifier_offset);
            return false;
        }
        return parse_array_element_assignment_on(inner, identifier_offset);
    }
    // An Object-element array (REQ-0212) matches a scalar Object
    // variable's own rule: a plain `=` is rejected outright, requiring
    // `Set arr(i) = ...` instead (see parse_member_set_assignment's
    // array-element branch).
    if (array.is_object_element) {
        skip_horizontal_whitespace();
        const bool member_write = !at_end() && current() == '.';
        const bool udt_write =
            !member_write && is_udt_class(array.element_class_name);
        if (!member_write && !udt_write) {
            set_error("WFC0108", "object assignment requires Set",
                      identifier_offset);
            return false;
        }
        Value* element = nullptr;
        if (execute_) {
            const auto flat_offset = array_flat_offset(array, *indices);
            if (!flat_offset.has_value()) {
                return false;
            }
            element = &array.elements[*flat_offset];
        }
        Value placeholder{Nothing{}};
        if (member_write) {
            advance();
            if (element != nullptr) {
                if (const auto* const instance_base =
                        std::get_if<ObjectInstance>(element)) {
                    // `arr(i).Method args` with no `Call`: reaches
                    // the method directly, or through the array's
                    // declared interface (REQ-0233).
                    const auto& instance_class_def =
                        class_definitions_.at(instance_base->data->class_name);
                    const std::string& interface_name =
                        array.element_class_name;
                    bool implements_interface = false;
                    for (const auto& implemented :
                         instance_class_def.implements) {
                        if (implemented == interface_name) {
                            implements_interface = true;
                        }
                    }
                    const auto peek_offset = offset_;
                    char peek_type_character{};
                    auto peek_member_name =
                        parse_identifier(&peek_type_character);
                    skip_horizontal_whitespace();
                    const bool bare_statement_end =
                        at_end() || current() == '\r' || current() == '\n' ||
                        current() == ':' || current() == '\'';
                    const bool arguments_follow =
                        !bare_statement_end && current() != '=' &&
                        current() != '(' && current() != '.';
                    const bool is_method =
                        peek_member_name.has_value() &&
                        peek_type_character == '\0' &&
                        (instance_class_def.methods.contains(
                             *peek_member_name) ||
                         (implements_interface &&
                          instance_class_def.methods.contains(
                              interface_name + "_" + *peek_member_name)));
                    offset_ = peek_offset;
                    if ((bare_statement_end || arguments_follow) && is_method) {
                        bare_call_arguments_ = arguments_follow;
                        const auto base = *element;
                        const auto result = parse_member_access_after_dot(
                            base, identifier_offset, /*require_function=*/false,
                            interface_name);
                        return result.has_value();
                    }
                }
            }
            return parse_member_assignment(
                element != nullptr ? *element : placeholder, identifier_offset);
        }
        if (!consume('=')) {
            set_error("WFC0014", "expected assignment operator", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        auto source = parse_expression();
        if (!source.has_value()) {
            return false;
        }
        return assign_udt(element != nullptr ? *element : placeholder, *source,
                          identifier_offset);
    }

    skip_horizontal_whitespace();
    if (!consume('=')) {
        set_error("WFC0014", "expected assignment operator", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    auto value = parse_expression();
    if (!value.has_value()) {
        return false;
    }

    // A Variant-element array (REQ-0212) retypes freely per element,
    // the same as a scalar Variant variable -- no fixed-type
    // enforcement.
    if (!array.is_variant_element) {
        const auto element_type_index = array.element_type_index;
        if (!coerce_numeric_value(*value, element_type_index,
                                  identifier_offset)) {
            return false;
        }
        if (element_type_index != value->index()) {
            set_error("WFC0016", "assignment type mismatch", identifier_offset);
            return false;
        }
    }
    if (!execute_) {
        return true;
    }
    const auto flat_offset = array_flat_offset(array, *indices);
    if (!flat_offset.has_value()) {
        return false;
    }
    if (array.element_fixed_length != 0U) {
        if (auto* text = std::get_if<std::string>(&*value)) {
            fit_to_units(*text, array.element_fixed_length);
        }
    }
    array.elements[*flat_offset] = copy_if_udt(std::move(*value));
    return true;
}

}  // namespace wfc::detail
