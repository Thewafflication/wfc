// Interpreter: Expression parsing, literals, conversions, comparison, and
// arithmetic. Internal to the WFC evaluator; not part of the public API. Split
// out of src/evaluator.cpp; see src/interpreter/README.md.

#include "interpreter.hpp"

namespace wfc::detail {

bool Interpreter::implicit_scalar_conversion(Value& value,
                                             const std::size_t target_index,
                                             const std::size_t offset) {
    const std::size_t object_index = Value{ObjectInstance{}}.index();
    if (value.index() == object_index && target_index != object_index &&
        !resolve_default_value(value, offset)) {
        return false;
    }
    const std::size_t long_index = Value{Integer{}}.index();
    const std::size_t string_index = Value{std::string{}}.index();
    const std::size_t bool_index = Value{false}.index();
    const bool target_numeric = target_index == long_index ||
                                target_index == Value{Int16{}}.index() ||
                                target_index == Value{Byte{}}.index() ||
                                target_index == Value{0.0}.index() ||
                                target_index == Value{0.0f}.index() ||
                                target_index == Value{Currency{}}.index() ||
                                target_index == Value{Decimal{}}.index();
    const bool target_scalar = target_numeric || target_index == string_index ||
                               target_index == bool_index ||
                               target_index == Value{DateValue{}}.index();
    if (!target_scalar) {
        return true;
    }
    const bool source_scalar = is_number(value) ||
                               std::holds_alternative<bool>(value) ||
                               std::holds_alternative<std::string>(value) ||
                               std::holds_alternative<Empty>(value) ||
                               std::holds_alternative<DateValue>(value) ||
                               std::holds_alternative<Null>(value);
    if (!source_scalar) {
        return true;
    }
    if (std::holds_alternative<Null>(value)) {
        if (execute_) {
            set_error("WFC0104", "Invalid use of Null", offset);
            return false;
        }
        value = zero_value_for_index(target_index);
        return true;
    }
    if (std::holds_alternative<Empty>(value)) {
        value = zero_value_for_index(target_index);
        return true;
    }
    if (!execute_) {
        // Dry run: the operand is a placeholder; only the type matters.
        if (target_index == bool_index || target_index == string_index ||
            target_numeric) {
            value = zero_value_for_index(target_index);
        }
        return true;
    }
    const auto mismatch = [&]() {
        set_error("WFC0016", "assignment type mismatch", offset);
        return false;
    };
    // To String.
    if (target_index == string_index) {
        if (std::holds_alternative<std::string>(value)) {
            return true;
        }
        value = render(value);
        return true;
    }
    // From String.
    if (const auto* text = std::get_if<std::string>(&value)) {
        if (target_index == string_index) {
            return true;
        }
        if (target_index == bool_index) {
            std::string lowered;
            for (const char c : *text) {
                lowered.push_back(ascii_lower(c));
            }
            if (lowered == "true") {
                value = true;
                return true;
            }
            if (lowered == "false") {
                value = false;
                return true;
            }
        }
        if (target_index == Value{DateValue{}}.index()) {
            return true;  // handled by the Date branch below
        }
        const auto parsed = parse_numeric_string(*text);
        if (parsed.status == NumericStringStatus::out_of_range) {
            set_error("WFC0009", "numeric overflow", offset);
            return false;
        }
        if (parsed.status != NumericStringStatus::valid) {
            return mismatch();
        }
        value = parsed.value;
        if (target_index == bool_index) {
            value = parsed.value != 0.0;
            return true;
        }
        return coerce_numeric_value(value, target_index, offset);
    }
    // From Boolean.
    if (const auto* flag = std::get_if<bool>(&value)) {
        if (target_index == bool_index) {
            return true;
        }
        value = Integer{*flag ? -1 : 0};
        return coerce_numeric_value(value, target_index, offset);
    }
    // To Boolean from a number.
    if (target_index == bool_index) {
        if (std::holds_alternative<DateValue>(value)) {
            value = std::get<DateValue>(value).serial != 0.0;
        } else {
            value = as_double(value) != 0.0;
        }
        return true;
    }
    // To Long from the other numerics (round half to even).
    if (target_index == long_index && !std::holds_alternative<Integer>(value)) {
        double number{};
        if (const auto* date = std::get_if<DateValue>(&value)) {
            number = date->serial;
        } else if (std::holds_alternative<Decimal>(value)) {
            number = decimal_to_double(std::get<Decimal>(value));
        } else {
            number = as_double(value);
        }
        const double rounded = std::nearbyint(number);
        if (!(rounded >= -2147483648.0 && rounded <= 2147483647.0)) {
            set_error("WFC0009", "integer overflow", offset);
            return false;
        }
        value = static_cast<Integer>(rounded);
        return true;
    }
    return true;
}

bool Interpreter::coerce_numeric_value(Value& value,
                                       const std::size_t target_index,
                                       const std::size_t offset) {
    if (value.index() == target_index) {
        return true;
    }
    if (!implicit_scalar_conversion(value, target_index, offset)) {
        return false;
    }
    if (value.index() == target_index) {
        return true;
    }
    // REQ-0254: Decimal target from any exact or floating numeric.
    if (target_index == Value{Decimal{}}.index()) {
        if (!is_number(value)) {
            return true;  // caller reports the mismatch
        }
        Decimal result;
        if (const auto* integer = std::get_if<Integer>(&value)) {
            result.negative = *integer < 0;
            result.mantissa = big_from_u32(static_cast<std::uint32_t>(
                *integer < 0 ? -static_cast<std::int64_t>(*integer)
                             : *integer));
        } else if (const auto* short_integer = std::get_if<Int16>(&value)) {
            result.negative = *short_integer < 0;
            result.mantissa = big_from_u32(static_cast<std::uint32_t>(
                *short_integer < 0 ? -static_cast<std::int32_t>(*short_integer)
                                   : *short_integer));
        } else if (const auto* byte = std::get_if<Byte>(&value)) {
            result.mantissa = big_from_u32(static_cast<std::uint32_t>(*byte));
        } else if (const auto* currency = std::get_if<Currency>(&value)) {
            result.negative = currency->scaled < 0;
            const auto magnitude =
                currency->scaled < 0
                    ? (~static_cast<std::uint64_t>(currency->scaled) + 1ULL)
                    : static_cast<std::uint64_t>(currency->scaled);
            result.mantissa.limb[0] = static_cast<std::uint32_t>(magnitude);
            result.mantissa.limb[1] =
                static_cast<std::uint32_t>(magnitude >> 32U);
            result.scale = 4U;
        } else {
            const auto converted = decimal_from_double(as_double(value));
            if (!converted.has_value()) {
                set_error("WFC0009", "numeric overflow", offset);
                return false;
            }
            result = *converted;
        }
        value = result;
        return true;
    }
    // REQ-0247: Byte target (range-checked) and Byte source (widens).
    if (target_index == Value{Byte{}}.index()) {
        double source_value{};
        if (std::holds_alternative<Decimal>(value) || !is_number(value)) {
            return true;  // caller reports the type mismatch
        }
        source_value = std::nearbyint(as_double(value));
        if (!(source_value >= 0.0 && source_value <= 255.0)) {
            set_error("WFC0009", "integer overflow", offset);
            return false;
        }
        value = static_cast<Byte>(source_value);
        return true;
    }
    if (const auto* byte_source = std::get_if<Byte>(&value)) {
        value = static_cast<Integer>(*byte_source);
        if (target_index == Value{Integer{}}.index()) {
            return true;
        }
        return coerce_numeric_value(value, target_index, offset);
    }
    // REQ-0242: Date <-> numeric/String implicit conversions.
    if (target_index == Value{DateValue{}}.index()) {
        if (is_number(value) && !std::holds_alternative<Decimal>(value)) {
            value = DateValue{as_double(value)};
        } else if (const auto* text = std::get_if<std::string>(&value)) {
            if (const auto parsed = parse_date_text(*text)) {
                value = DateValue{*parsed};
            }
        }
        return true;
    }
    if (const auto* date = std::get_if<DateValue>(&value)) {
        if (target_index == Value{std::string{}}.index()) {
            value = render_date(date->serial);
            return true;
        }
        if (target_index != Value{Empty{}}.index() &&
            target_index != Value{Null{}}.index() &&
            target_index != Value{Nothing{}}.index() &&
            target_index != Value{false}.index()) {
            value = date->serial;
            return coerce_numeric_value(value, target_index, offset);
        }
        return true;
    }
    if (target_index == Value{0.0}.index()) {
        if (const auto* integer = std::get_if<Integer>(&value)) {
            value = static_cast<double>(*integer);
        } else if (const auto* short_integer = std::get_if<Int16>(&value)) {
            value = static_cast<double>(*short_integer);
        } else if (const auto* single = std::get_if<float>(&value)) {
            value = static_cast<double>(*single);
        } else if (std::holds_alternative<Currency>(value)) {
            value = as_double(value);
        }
        return true;
    }
    if (target_index == Value{0.0f}.index()) {
        if (const auto* integer = std::get_if<Integer>(&value)) {
            value = static_cast<float>(*integer);
            return true;
        }
        if (const auto* short_integer = std::get_if<Int16>(&value)) {
            value = static_cast<float>(*short_integer);
            return true;
        }
        if (const auto* number = std::get_if<double>(&value)) {
            const auto narrowed = static_cast<float>(*number);
            if (!std::isfinite(narrowed)) {
                set_error("WFC0009", "numeric overflow", offset);
                return false;
            }
            value = narrowed;
            return true;
        }
        if (std::holds_alternative<Currency>(value)) {
            const auto narrowed = static_cast<float>(as_double(value));
            if (!std::isfinite(narrowed)) {
                set_error("WFC0009", "numeric overflow", offset);
                return false;
            }
            value = narrowed;
            return true;
        }
        return true;
    }
    if (target_index == Value{Currency{}}.index()) {
        if (const auto* integer = std::get_if<Integer>(&value)) {
            value = Currency{static_cast<std::int64_t>(*integer) * 10000};
            return true;
        }
        if (const auto* short_integer = std::get_if<Int16>(&value)) {
            value = Currency{static_cast<std::int64_t>(*short_integer) * 10000};
            return true;
        }
        if (const auto* number = std::get_if<double>(&value)) {
            const auto scaled = currency_from_double(*number);
            if (!scaled.has_value()) {
                set_error("WFC0009", "numeric overflow", offset);
                return false;
            }
            value = Currency{*scaled};
            return true;
        }
        if (const auto* single = std::get_if<float>(&value)) {
            const auto scaled =
                currency_from_double(static_cast<double>(*single));
            if (!scaled.has_value()) {
                set_error("WFC0009", "numeric overflow", offset);
                return false;
            }
            value = Currency{*scaled};
            return true;
        }
        return true;
    }
    if (target_index == Value{Int16{}}.index()) {
        // Widening from Long, or narrowing from Double/Single/Currency
        // with a checked range (VB6 Integer is -32768 through 32767); an
        // already-integral Long source needs a plain range check, not
        // rounding.
        if (const auto* integer = std::get_if<Integer>(&value)) {
            if (*integer < std::numeric_limits<Int16>::min() ||
                *integer > std::numeric_limits<Int16>::max()) {
                set_error("WFC0009", "integer overflow", offset);
                return false;
            }
            value = static_cast<Int16>(*integer);
            return true;
        }
        if (const auto* number = std::get_if<double>(&value)) {
            const double rounded = std::nearbyint(*number);
            if (!(rounded >=
                      static_cast<double>(std::numeric_limits<Int16>::min()) &&
                  rounded <=
                      static_cast<double>(std::numeric_limits<Int16>::max()))) {
                set_error("WFC0009", "integer overflow", offset);
                return false;
            }
            value = static_cast<Int16>(rounded);
            return true;
        }
        if (const auto* single = std::get_if<float>(&value)) {
            const double rounded = std::nearbyint(static_cast<double>(*single));
            if (!(rounded >=
                      static_cast<double>(std::numeric_limits<Int16>::min()) &&
                  rounded <=
                      static_cast<double>(std::numeric_limits<Int16>::max()))) {
                set_error("WFC0009", "integer overflow", offset);
                return false;
            }
            value = static_cast<Int16>(rounded);
            return true;
        }
        if (std::holds_alternative<Currency>(value)) {
            const double rounded = std::nearbyint(as_double(value));
            if (!(rounded >=
                      static_cast<double>(std::numeric_limits<Int16>::min()) &&
                  rounded <=
                      static_cast<double>(std::numeric_limits<Int16>::max()))) {
                set_error("WFC0009", "integer overflow", offset);
                return false;
            }
            value = static_cast<Int16>(rounded);
            return true;
        }
        return true;
    }
    return true;
}

std::optional<Value> Interpreter::parse_expression() {
    // Deeply nested parentheses or calls recurse through here; stop before
    // the native stack runs out.
    if (stack_nearly_exhausted()) {
        set_error("WFC0123", "expression nesting is too deep", offset_);
        return std::nullopt;
    }
    return parse_implication();
}

std::optional<Value> Interpreter::parse_implication() {
    auto left = parse_equivalence();
    if (!left.has_value()) {
        return std::nullopt;
    }
    while (true) {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        if (!consume_keyword("imp")) {
            return left;
        }
        skip_horizontal_whitespace();
        auto right = parse_equivalence();
        if (!right.has_value()) {
            return std::nullopt;
        }
        left = logical_binary(*left, *right, 'I', operator_offset);
        if (!left.has_value()) {
            return std::nullopt;
        }
    }
}

std::optional<Value> Interpreter::parse_equivalence() {
    auto left = parse_exclusive_or();
    if (!left.has_value()) {
        return std::nullopt;
    }
    while (true) {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        if (!consume_keyword("eqv")) {
            return left;
        }
        skip_horizontal_whitespace();
        auto right = parse_exclusive_or();
        if (!right.has_value()) {
            return std::nullopt;
        }
        left = logical_binary(*left, *right, 'E', operator_offset);
        if (!left.has_value()) {
            return std::nullopt;
        }
    }
}

std::optional<Value> Interpreter::parse_exclusive_or() {
    auto left = parse_or();
    if (!left.has_value()) {
        return std::nullopt;
    }
    while (true) {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        if (!consume_keyword("xor")) {
            return left;
        }
        skip_horizontal_whitespace();
        auto right = parse_or();
        if (!right.has_value()) {
            return std::nullopt;
        }
        left = logical_binary(*left, *right, 'X', operator_offset);
        if (!left.has_value()) {
            return std::nullopt;
        }
    }
}

std::optional<Value> Interpreter::parse_or() {
    auto left = parse_and();
    if (!left.has_value()) {
        return std::nullopt;
    }
    while (true) {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        if (!consume_keyword("or")) {
            return left;
        }
        skip_horizontal_whitespace();
        auto right = parse_and();
        if (!right.has_value()) {
            return std::nullopt;
        }
        left = logical_binary(*left, *right, 'O', operator_offset);
        if (!left.has_value()) {
            return std::nullopt;
        }
    }
}

std::optional<Value> Interpreter::parse_and() {
    auto left = parse_not();
    if (!left.has_value()) {
        return std::nullopt;
    }
    while (true) {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        if (!consume_keyword("and")) {
            return left;
        }
        skip_horizontal_whitespace();
        auto right = parse_not();
        if (!right.has_value()) {
            return std::nullopt;
        }
        left = logical_binary(*left, *right, 'A', operator_offset);
        if (!left.has_value()) {
            return std::nullopt;
        }
    }
}

std::optional<Value> Interpreter::parse_not() {
    skip_horizontal_whitespace();
    const auto operator_offset = offset_;
    if (!consume_keyword("not")) {
        return parse_comparison();
    }
    skip_horizontal_whitespace();
    auto value = parse_not();
    if (!value.has_value()) {
        return std::nullopt;
    }
    if (const auto* integer = std::get_if<Integer>(&*value)) {
        return Value{static_cast<Integer>(~*integer)};
    }
    if (const auto* short_integer = std::get_if<Int16>(&*value)) {
        return Value{static_cast<Int16>(~*short_integer)};
    }
    if (const auto* byte = std::get_if<Byte>(&*value)) {
        return Value{static_cast<Byte>(~*byte)};
    }
    if (execute_ && !std::holds_alternative<bool>(*value)) {
        Value widened = *value;
        if (!widen_bitwise_operand(widened, operator_offset)) {
            return std::nullopt;
        }
        if (const auto* integer = std::get_if<Integer>(&widened)) {
            return Value{static_cast<Integer>(~*integer)};
        }
    }
    const auto operand = coerce_ternary_operand(*value, operator_offset);
    if (!operand.has_value()) {
        return std::nullopt;
    }
    if (operand->is_null) {
        return Value{Null{}};
    }
    return Value{!operand->value};
}

bool Interpreter::like_match(const std::string& text,
                             const std::string& pattern, const bool fold) {
    if (is_ascii_text(text) && is_ascii_text(pattern)) {
        return like_match_units(text, pattern, fold);
    }
    return like_match_units(to_utf16_units(text), to_utf16_units(pattern),
                            fold);
}

std::optional<Value> Interpreter::parse_comparison() {
    auto left = parse_concatenation();
    if (!left.has_value()) {
        return std::nullopt;
    }

    while (true) {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        if (consume_keyword("is")) {
            skip_horizontal_whitespace();
            auto right = parse_concatenation();
            if (!right.has_value()) {
                return std::nullopt;
            }
            // `Is` compares two object references for identity: Nothing Is
            // Nothing is always True; two live instances are the same
            // object exactly when they share the same underlying
            // InstanceData (Value's variant-generated operator== already
            // does the right thing for both cases, since ObjectInstance's
            // own operator== compares the shared_ptr, not field contents).
            // Both operands must still be object references, matching real
            // VB6's requirement that Is only accepts object operands.
            if (!is_object_reference(*left) || !is_object_reference(*right)) {
                if (!execute_) {
                    left = Value{
                        false};  // placeholder operand of a not-taken branch
                    continue;
                }
                set_error("WFC0107", "Is requires object operands",
                          operator_offset);
                return std::nullopt;
            }
            left = Value{*left == *right};
            continue;
        }
        if (consume_keyword("like")) {
            skip_horizontal_whitespace();
            auto pattern = parse_concatenation();
            if (!pattern.has_value()) {
                return std::nullopt;
            }
            const auto* text = std::get_if<std::string>(&*left);
            const auto* mask = std::get_if<std::string>(&*pattern);
            if ((text == nullptr || mask == nullptr) && !execute_) {
                left = Value{false};
                continue;
            }
            if (text == nullptr || mask == nullptr) {
                set_error("WFC0018", "Like requires String operands",
                          operator_offset);
                return std::nullopt;
            }
            left =
                Value{execute_ ? like_match(*text, *mask, option_compare_text_)
                               : false};
            continue;
        }
        std::string_view operation;
        if (consume('=')) {
            operation = "=";
        } else if (consume('<')) {
            if (consume('=')) {
                operation = "<=";
            } else if (consume('>')) {
                operation = "<>";
            } else {
                operation = "<";
            }
        } else if (consume('>')) {
            operation = consume('=') ? ">=" : ">";
        } else {
            return left;
        }

        skip_horizontal_whitespace();
        auto right = parse_concatenation();
        if (!right.has_value()) {
            return std::nullopt;
        }
        left = compare(*left, *right, operation, operator_offset);
        if (!left.has_value()) {
            return std::nullopt;
        }
    }
}

std::optional<Value> Interpreter::parse_concatenation() {
    auto left = parse_additive();
    if (!left.has_value()) {
        return std::nullopt;
    }

    while (true) {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        if (!consume('&')) {
            return left;
        }
        skip_horizontal_whitespace();
        auto right = parse_additive();
        if (!right.has_value()) {
            return std::nullopt;
        }
        // Concatenating two Nulls raises "Invalid use of Null" (verified
        // against the local VB6 6.00.8176 reference); a single Null
        // operand instead concatenates as an empty string (also
        // verified: Null & "x" = "x", with no error).
        const bool left_null = std::holds_alternative<Null>(*left);
        const bool right_null = std::holds_alternative<Null>(*right);
        if (left_null && right_null) {
            set_error("WFC0104", "Invalid use of Null", operator_offset);
            return std::nullopt;
        }
        if (left_null || right_null) {
            left = Value{(left_null ? std::string{} : render(*left)) +
                         (right_null ? std::string{} : render(*right))};
            continue;
        }
        if (!resolve_default_value(*left, operator_offset) ||
            !resolve_default_value(*right, operator_offset)) {
            return std::nullopt;
        }
        if (is_object_reference(*left) || is_object_reference(*right) ||
            std::holds_alternative<ArrayValue>(*left) ||
            std::holds_alternative<ArrayValue>(*right)) {
            set_error("WFC0020",
                      "concatenation requires String or Long operands",
                      operator_offset);
            return std::nullopt;
        }
        if (auto* text = std::get_if<std::string>(&*left)) {
            if (const auto* tail = std::get_if<std::string>(&*right)) {
                const auto left_size = text->size();
                text->append(
                    *tail);  // extend in place instead of copying `left` again
                merge_byte_halves(*text, left_size);
                continue;
            }
        }
        left = Value{render(*left) + render(*right)};
    }
}

std::optional<Value> Interpreter::parse_additive() {
    auto left = parse_multiplicative();
    if (!left.has_value()) {
        return std::nullopt;
    }

    while (true) {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        char operation = '\0';
        if (consume('+')) {
            operation = '+';
        } else if (consume('-')) {
            operation = '-';
        } else {
            return left;
        }

        skip_horizontal_whitespace();
        auto right = parse_multiplicative();
        if (!right.has_value()) {
            return std::nullopt;
        }
        left = numeric_binary(*left, *right, operation, operator_offset);
        if (!left.has_value()) {
            return std::nullopt;
        }
    }
}

std::optional<Value> Interpreter::parse_star_slash() {
    auto left = parse_unary();
    if (!left.has_value()) {
        return std::nullopt;
    }

    while (true) {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        char operation = '\0';
        bool integer_only = false;
        if (consume('*')) {
            operation = '*';
        } else if (consume('/')) {
            operation = '/';
        } else {
            return left;
        }

        skip_horizontal_whitespace();
        auto right = parse_unary();
        if (!right.has_value()) {
            return std::nullopt;
        }
        left = integer_only
                   ? integer_binary(*left, *right, operation, operator_offset)
                   : numeric_binary(*left, *right, operation, operator_offset);
        if (!left.has_value()) {
            return std::nullopt;
        }
    }
}

std::optional<Value> Interpreter::parse_integer_division() {
    auto left = parse_star_slash();
    if (!left.has_value()) {
        return std::nullopt;
    }
    while (true) {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        if (!consume('\\')) {
            return left;
        }
        skip_horizontal_whitespace();
        auto right = parse_star_slash();
        if (!right.has_value()) {
            return std::nullopt;
        }
        left = integer_binary(*left, *right, '\\', operator_offset);
        if (!left.has_value()) {
            return std::nullopt;
        }
    }
}

std::optional<Value> Interpreter::parse_multiplicative() {
    auto left = parse_integer_division();
    if (!left.has_value()) {
        return std::nullopt;
    }
    while (true) {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        if (!consume_keyword("mod")) {
            return left;
        }
        skip_horizontal_whitespace();
        auto right = parse_integer_division();
        if (!right.has_value()) {
            return std::nullopt;
        }
        left = integer_binary(*left, *right, '%', operator_offset);
        if (!left.has_value()) {
            return std::nullopt;
        }
    }
}

std::optional<Value> Interpreter::parse_unary() {
    skip_horizontal_whitespace();
    const auto operator_offset = offset_;
    if (consume('+')) {
        auto value = parse_unary();
        if (!value.has_value()) {
            return std::nullopt;
        }
        if (const auto* flag = std::get_if<bool>(&*value)) {
            value = Value{static_cast<Int16>(*flag ? -1 : 0)};
        }
        // Unary +/- follow the same Null-propagates,
        // Empty-coerces-to-zero rule already verified for the binary
        // arithmetic operators.
        if (std::holds_alternative<Null>(*value)) {
            return value;
        }
        if (std::holds_alternative<Empty>(*value)) {
            return Value{Integer{0}};
        }
        if (!std::holds_alternative<double>(*value) &&
            !std::holds_alternative<float>(*value) &&
            !std::holds_alternative<Currency>(*value) &&
            !std::holds_alternative<Decimal>(*value) &&
            !std::holds_alternative<Int16>(*value) &&
            require_integer(*value, operator_offset) == nullptr) {
            return std::nullopt;
        }
        return value;
    }
    if (consume('-')) {
        skip_horizontal_whitespace();
        if (!at_end() &&
            (std::isdigit(static_cast<unsigned char>(current())) != 0 ||
             (current() == '.' &&
              std::isdigit(static_cast<unsigned char>(peek(1))) != 0))) {
            // `-2 ^ 2` is -(2 ^ 2): only a bare literal takes the
            // negative-literal path.
            const auto literal_start = offset_;
            const bool parsed_literal = parse_number().has_value();
            skip_horizontal_whitespace();
            const bool followed_by_power =
                parsed_literal && !at_end() && current() == '^';
            offset_ = literal_start;
            error_ = wfc::Evaluation{};
            if (!followed_by_power) {
                return parse_negative_number();
            }
        }

        auto value = parse_unary();
        if (!value.has_value()) {
            return std::nullopt;
        }
        if (const auto* flag = std::get_if<bool>(&*value)) {
            value = Value{static_cast<Int16>(*flag ? -1 : 0)};
        }
        if (const auto* byte = std::get_if<Byte>(&*value)) {
            value = Value{
                static_cast<Int16>(*byte)};  // negated below as an Integer
        }
        if (std::holds_alternative<Null>(*value)) {
            return value;
        }
        if (std::holds_alternative<Empty>(*value)) {
            return execute_ ? Value{Integer{0}} : Value{Integer{}};
        }
        if (const auto* number = std::get_if<double>(&*value)) {
            return execute_ ? Value{-*number} : Value{0.0};
        }
        if (const auto* single = std::get_if<float>(&*value)) {
            return execute_ ? Value{-*single} : Value{0.0f};
        }
        if (const auto* decimal = std::get_if<Decimal>(&*value)) {
            if (!execute_) {
                return Value{Decimal{}};
            }
            Decimal negated = *decimal;
            if (!is_zero_big(negated.mantissa)) {
                negated.negative = !negated.negative;
            }
            return Value{negated};
        }
        if (const auto* currency = std::get_if<Currency>(&*value)) {
            if (!execute_) {
                return Value{Currency{}};
            }
            if (currency->scaled == std::numeric_limits<std::int64_t>::min()) {
                set_error("WFC0009", "numeric overflow", operator_offset);
                return std::nullopt;
            }
            return Value{Currency{-currency->scaled}};
        }
        if (const auto* short_integer = std::get_if<Int16>(&*value)) {
            if (!execute_) {
                return Value{Int16{}};
            }
            if (*short_integer == std::numeric_limits<Int16>::min()) {
                set_error("WFC0009", "integer overflow", operator_offset);
                return std::nullopt;
            }
            return Value{static_cast<Int16>(-*short_integer)};
        }
        if (const auto* byte = std::get_if<Byte>(&*value)) {
            return Value{static_cast<Integer>(-static_cast<Integer>(*byte))};
        }
        const auto* integer = require_integer(*value, operator_offset);
        if (integer == nullptr) {
            return std::nullopt;
        }
        if (!execute_) {
            return Value{Integer{}};
        }
        if (*integer == std::numeric_limits<Integer>::min()) {
            set_error("WFC0009", "integer overflow", operator_offset);
            return std::nullopt;
        }
        return Value{static_cast<Integer>(-*integer)};
    }
    return parse_power();
}

std::optional<Value> Interpreter::parse_power() {
    auto left = parse_primary();
    if (!left.has_value()) {
        return std::nullopt;
    }
    while (true) {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        if (!consume('^')) {
            return left;
        }
        skip_horizontal_whitespace();
        bool negate = false;
        if (consume('-')) {
            negate = true;
            skip_horizontal_whitespace();
        } else {
            static_cast<void>(consume('+'));
            skip_horizontal_whitespace();
        }
        auto right = parse_primary();
        if (!right.has_value()) {
            return std::nullopt;
        }
        if (std::holds_alternative<Null>(*left) ||
            std::holds_alternative<Null>(*right)) {
            left = Value{Null{}};
            continue;
        }
        const auto operand = [](const Value& v) -> std::optional<double> {
            if (std::holds_alternative<Empty>(v)) {
                return 0.0;
            }
            if (is_number(v)) {
                return as_double(v);
            }
            if (const auto* d = std::get_if<DateValue>(&v)) {
                return d->serial;
            }
            if (const auto* flag = std::get_if<bool>(&v)) {
                return *flag ? -1.0 : 0.0;
            }
            if (const auto* text = std::get_if<std::string>(&v)) {
                const auto parsed = parse_numeric_string(*text);
                if (parsed.status == NumericStringStatus::valid) {
                    return parsed.value;
                }
            }
            return std::nullopt;
        };
        const auto base = operand(*left);
        auto exponent = operand(*right);
        if (!base.has_value() || !exponent.has_value()) {
            set_error("WFC0007", "operator requires numeric operands",
                      operator_offset);
            return std::nullopt;
        }
        if (negate) {
            exponent = -*exponent;
        }
        if (!execute_) {
            left = Value{0.0};
            continue;
        }
        if (*base == 0.0 && *exponent < 0.0) {
            set_error("WFC0008", "division by zero", operator_offset);
            return std::nullopt;
        }
        const double result = std::pow(*base, *exponent);
        if (!std::isfinite(result)) {
            set_error("WFC0009", "numeric overflow", operator_offset);
            return std::nullopt;
        }
        left = Value{result};
    }
}

std::optional<Value> Interpreter::parse_primary() {
    const auto base_offset = offset_;
    // REQ-0233: if the very next primary is a bare identifier naming a
    // variable with a specific declared class (REQ-0203/REQ-0228's
    // `object_class_names`), remember that class name so the *first*
    // `.member` access immediately following it can dispatch through
    // that class's own `Implements` list (see
    // `parse_member_access_after_dot`'s own `via_interface_class`)
    // when it names an interface the live instance's actual class
    // implements. A speculative lookahead-then-rewind:
    // `parse_primary_base` below still does the real parse of
    // whatever this turns out to be, unaffected by this peek.
    std::string declared_interface_class;
    {
        const auto lookahead_offset = offset_;
        skip_horizontal_whitespace();
        char lookahead_type_character{};
        auto lookahead_identifier = parse_identifier(&lookahead_type_character);
        if (lookahead_identifier.has_value() &&
            lookahead_type_character == '\0') {
            const auto variable = find_variable(*lookahead_identifier);
            if (variable.value != nullptr) {
                const auto declared_class =
                    variable.scope->object_class_names.find(
                        *lookahead_identifier);
                if (declared_class !=
                    variable.scope->object_class_names.end()) {
                    declared_interface_class = declared_class->second;
                } else if (const auto* array =
                               std::get_if<ArrayValue>(variable.value)) {
                    // An array of interface-typed references dispatches
                    // through the element class the same way.
                    if (array->is_object_element) {
                        declared_interface_class = array->element_class_name;
                    }
                }
            }
        }
        offset_ = lookahead_offset;
    }
    auto value = parse_primary_base();
    if (!value.has_value()) {
        return std::nullopt;
    }
    bool is_first_member_access = true;
    while (true) {
        // `jag(2)(1)`, `Split(s)(0)`: index the array a previous primary
        // produced.
        if (!at_end() && current() == '(' &&
            std::holds_alternative<ArrayValue>(*value)) {
            value = parse_array_index(*value);
            if (!value.has_value()) {
                return std::nullopt;
            }
            continue;
        }
        // `c(1)("b")`: parentheses after an object apply its default member.
        if (execute_ && !at_end() && current() == '(' &&
            std::holds_alternative<ObjectInstance>(*value)) {
            const auto held = std::get<ObjectInstance>(*value);
            value = call_default_member(held, base_offset);
            if (!value.has_value()) {
                return std::nullopt;
            }
            continue;
        }
        if (!execute_ && !at_end() && current() == '(' &&
            !is_object_reference(*value)) {
            // A not-taken branch: the placeholder stands for an array element's
            // array.
            advance();
            if (!parse_index_list(kAnyDimensionCount).has_value()) {
                return std::nullopt;
            }
            value = Value{Empty{}};
            continue;
        }
        skip_horizontal_whitespace();
        if (at_end() || current() != '.') {
            return value;
        }
        advance();
        value = parse_member_access_after_dot(
            std::move(*value), base_offset, /*require_function=*/true,
            is_first_member_access ? declared_interface_class : std::string{});
        is_first_member_access = false;
        if (!value.has_value()) {
            return std::nullopt;
        }
    }
}

std::optional<Value> Interpreter::parse_primary_base() {
    skip_horizontal_whitespace();
    if (at_end() || current() == '\r' || current() == '\n' ||
        current() == ':' || current() == '\'') {
        set_error("WFC0002", "expected expression", offset_);
        return std::nullopt;
    }

    if (current() == '"') {
        return parse_string();
    }
    if (current() == '&' &&
        (ascii_lower(peek(1)) == 'h' || ascii_lower(peek(1)) == 'o')) {
        // REQ-0244: `&HFF` / `&O17` (optionally `&`-suffixed for Long).
        const bool hex = ascii_lower(peek(1)) == 'h';
        const auto literal_offset = offset_;
        offset_ += 2;
        std::uint64_t magnitude = 0;
        std::size_t digits = 0;
        while (!at_end()) {
            const char c = ascii_lower(current());
            int digit = -1;
            if (c >= '0' && c <= '9') {
                digit = c - '0';
            } else if (hex && c >= 'a' && c <= 'f') {
                digit = c - 'a' + 10;
            }
            if (digit < 0 || (!hex && digit > 7)) {
                break;
            }
            magnitude = magnitude * (hex ? 16U : 8U) +
                        static_cast<std::uint64_t>(digit);
            if (magnitude > 0xFFFFFFFFULL) {
                set_error("WFC0006", "integer literal out of range",
                          literal_offset);
                return std::nullopt;
            }
            ++digits;
            advance();
        }
        if (digits == 0) {
            set_error("WFC0006", "invalid numeric literal", literal_offset);
            return std::nullopt;
        }
        const bool long_suffix = !at_end() && current() == '&';
        if (long_suffix) {
            advance();
        }
        if (!long_suffix && magnitude <= 0xFFFFULL) {
            return Value{
                static_cast<Int16>(static_cast<std::uint16_t>(magnitude))};
        }
        return Value{
            static_cast<Integer>(static_cast<std::uint32_t>(magnitude))};
    }
    if (current() == '#') {
        // REQ-0242: a Date literal `#m/d/yyyy h:mm:ss AM#`.
        const auto literal_offset = offset_;
        {
            // `#3` in an argument list (`Input(5, #1)`) is a file number.
            std::size_t look = offset_ + 1;
            while (look < source_.size() &&
                   std::isdigit(static_cast<unsigned char>(source_[look])) !=
                       0) {
                ++look;
            }
            std::size_t after = look;
            while (after < source_.size() &&
                   (source_[after] == ' ' || source_[after] == '\t')) {
                ++after;
            }
            // `#f` before `)` or `,` is a file number held in a variable.
            std::size_t name_end = offset_ + 1;
            while (name_end < source_.size() &&
                   is_identifier_part(source_[name_end])) {
                ++name_end;
            }
            std::size_t name_after = name_end;
            while (
                name_after < source_.size() &&
                (source_[name_after] == ' ' || source_[name_after] == '\t')) {
                ++name_after;
            }
            if (name_end > offset_ + 1 &&
                is_identifier_start(source_[offset_ + 1]) &&
                (name_after >= source_.size() || source_[name_after] == ')' ||
                 source_[name_after] == ',')) {
                advance();  // the '#'
                return parse_primary_base();
            }
            if (look > offset_ + 1 &&
                (after >= source_.size() || source_[after] == ')' ||
                 source_[after] == ',')) {
                Integer file_number = 0;
                for (std::size_t k = offset_ + 1; k < look; ++k) {
                    file_number = file_number * 10 + (source_[k] - '0');
                }
                offset_ = look;
                return Value{file_number};
            }
        }
        advance();
        const auto text_start = offset_;
        while (!at_end() && current() != '#' && current() != '\r' &&
               current() != '\n') {
            advance();
        }
        if (at_end() || current() != '#') {
            set_error("WFC0006", "unterminated Date literal", literal_offset);
            return std::nullopt;
        }
        const auto parsed =
            parse_date_text(source_.substr(text_start, offset_ - text_start));
        advance();
        if (!parsed.has_value()) {
            set_error("WFC0006", "invalid Date literal", literal_offset);
            return std::nullopt;
        }
        return Value{DateValue{*parsed}};
    }
    if (std::isdigit(static_cast<unsigned char>(current())) != 0 ||
        (current() == '.' &&
         std::isdigit(static_cast<unsigned char>(peek(1))) != 0)) {
        return parse_number();
    }
    if (consume_keyword("true")) {
        return Value{true};
    }
    if (consume_keyword("false")) {
        return Value{false};
    }
    if (consume_keyword("null")) {
        return Value{Null{}};
    }
    if (consume_keyword("empty")) {
        return Value{Empty{}};
    }
    if (consume_keyword("nothing")) {
        return Value{Nothing{}};
    }
    {
        const auto me_offset = offset_;
        if (consume_keyword("me")) {
            return me_value(me_offset);
        }
    }
    {
        // REQ-0250: `TypeOf obj Is ClassName` and `Erl`.
        const auto typeof_offset = offset_;
        if (consume_keyword("typeof")) {
            skip_horizontal_whitespace();
            auto operand = parse_concatenation();
            if (!operand.has_value()) {
                return std::nullopt;
            }
            skip_horizontal_whitespace();
            if (!consume_keyword("is")) {
                set_error("WFC0010", "expected Is after TypeOf operand",
                          offset_);
                return std::nullopt;
            }
            skip_horizontal_whitespace();
            const auto class_offset = offset_;
            auto class_name = parse_identifier();
            if (!class_name.has_value()) {
                set_error("WFC0011", "expected class name after Is",
                          class_offset);
                return std::nullopt;
            }
            if (*class_name != "object" &&
                !class_definitions_.contains(*class_name)) {
                set_error("WFC0134", "unknown class name", class_offset);
                return std::nullopt;
            }
            if (!is_object_reference(*operand)) {
                set_error("WFC0107", "TypeOf requires an object operand",
                          typeof_offset);
                return std::nullopt;
            }
            const auto* instance = std::get_if<ObjectInstance>(&*operand);
            if (instance == nullptr || !execute_) {
                return Value{false};
            }
            return Value{
                *class_name == "object" ||
                class_satisfies(instance->data->class_name, *class_name)};
        }
        if (consume_keyword("erl")) {
            // Inside a handler Erl stays at the line the error occurred on.
            return Value{err_number_ != 0 ? err_erl_ : erl_};
        }
    }
    {
        const auto err_start = offset_;
        // Inside `With Err`, a leading `.Member` is `Err.Member`.
        const bool with_err_member =
            at_with_member() && with_names_.back() == "err";
        if (with_err_member || consume_keyword("err")) {
            if (consume('.')) {
                const auto member_offset = offset_;
                const auto member = parse_identifier();
                if (member == "number") {
                    return Value{err_number_};
                }
                if (member == "description") {
                    return Value{err_description_};
                }
                if (member == "source") {
                    return Value{err_source_};
                }
                if (member == "helpfile") {
                    return Value{err_help_file_};
                }
                if (member == "helpcontext") {
                    return Value{err_help_context_};
                }
                if (member == "lastdllerror") {
                    return Value{Integer{0}};
                }
                set_error("WFC0135", "unknown member", member_offset);
                return std::nullopt;
            }
            offset_ = err_start;
        }
    }
    if (consume_keyword("new")) {
        skip_horizontal_whitespace();
        const auto class_name_offset = offset_;
        char class_type_character{};
        auto class_name = parse_identifier(&class_type_character);
        if (!class_name.has_value() || class_type_character != '\0') {
            set_error("WFC0011", "expected class name after New",
                      class_name_offset);
            return std::nullopt;
        }
        return instantiate_class(*class_name, class_name_offset);
    }
    if (consume('(')) {
        auto value = parse_expression();
        if (!value.has_value()) {
            return std::nullopt;
        }
        skip_horizontal_whitespace();
        if (!consume(')')) {
            set_error("WFC0005", "expected closing parenthesis", offset_);
            return std::nullopt;
        }
        return value;
    }
    if (is_identifier_start(current()) || at_with_member()) {
        const auto identifier_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        skip_horizontal_whitespace();
        if (!at_end() && current() == '(') {
            const auto array_variable = find_variable(*identifier);
            if (array_variable.value != nullptr &&
                std::holds_alternative<ArrayValue>(*array_variable.value)) {
                if (type_character != '\0') {
                    set_error("WFC0016",
                              "identifier type-declaration character mismatch",
                              identifier_offset);
                    return std::nullopt;
                }
                return parse_array_index(*array_variable.value);
            }
            if (array_variable.value != nullptr && type_character == '\0') {
                // REQ-0253/0257: a class's default member, `obj(1)`.
                if (const auto* holder =
                        std::get_if<ObjectInstance>(array_variable.value)) {
                    const auto class_iterator =
                        class_definitions_.find(holder->data->class_name);
                    if (class_iterator != class_definitions_.end() &&
                        !class_iterator->second.default_member.empty()) {
                        const auto& class_def = class_iterator->second;
                        const auto& member = class_def.default_member;
                        if (class_def.methods.contains(member)) {
                            return call_class_method(*holder->data, class_def,
                                                     member, identifier_offset,
                                                     /*require_function=*/true);
                        }
                        const auto getter = class_def.property_get.find(member);
                        if (getter != class_def.property_get.end()) {
                            auto arguments = parse_call_argument_list();
                            if (!arguments.has_value()) {
                                return std::nullopt;
                            }
                            return invoke_definition(
                                getter->second, member, std::move(*arguments),
                                identifier_offset, class_def.source,
                                holder->data.get());
                        }
                    }
                }
            }
            if (array_variable.value != nullptr && type_character == '\0' &&
                std::holds_alternative<Nothing>(*array_variable.value) &&
                !procedures_.contains(*identifier) &&
                !(current_class_def() != nullptr &&
                  (current_class_def()->methods.contains(*identifier) ||
                   current_class_def()->property_get.contains(*identifier)))) {
                // `obj(args)` through an unset Object variable.
                auto ignored = parse_call_argument_list();
                if (!ignored.has_value()) {
                    return std::nullopt;
                }
                if (execute_) {
                    static_cast<void>(raise_runtime(
                        91, "Object variable or With block variable not set",
                        identifier_offset));
                    return std::nullopt;
                }
                return Value{Empty{}};
            }
            if (procedures_.contains(*identifier) &&
                (type_character == '\0' ||
                 procedures_.at(*identifier).is_function)) {
                return parse_procedure_call(*identifier, identifier_offset);
            }
            // An unqualified call to a sibling method of the class
            // currently executing (including a method calling itself,
            // for recursion) -- the implicit-Me equivalent of
            // `Me.Method(args)`, which this evaluator does not require
            // spelling out since a class method never sees a same-named
            // module-level procedure anyway (its scope is isolated from
            // the module; see find_variable).
            if (type_character == '\0') {
                if (auto* const instance = current_instance()) {
                    if (const auto* const class_def = current_class_def()) {
                        if (class_def->methods.contains(*identifier)) {
                            return call_class_method(*instance, *class_def,
                                                     *identifier,
                                                     identifier_offset,
                                                     /*require_function=*/true);
                        }
                        // An unqualified indexed Property Get read
                        // (REQ-0205): `Item(0)` reaches the same
                        // parenthesized shape a sibling method call
                        // would.
                        const auto getter_iterator =
                            class_def->property_get.find(*identifier);
                        if (getter_iterator != class_def->property_get.end()) {
                            auto arguments = parse_call_argument_list();
                            if (!arguments.has_value()) {
                                return std::nullopt;
                            }
                            return invoke_definition(
                                getter_iterator->second, *identifier,
                                std::move(*arguments), identifier_offset,
                                class_def->source, instance);
                        }
                    }
                }
            }
            if (type_character != '\0') {
                identifier->push_back(type_character);
            }
            return parse_function_call(*identifier, identifier_offset);
        }
        // A declared variable shadows the built-in constants.
        const bool names_variable =
            allow_identifiers_ &&
            find_variable_raw(*identifier).value != nullptr;
        if (!names_variable) {
            if (const auto global = global_class_constants_.find(*identifier);
                global != global_class_constants_.end() &&
                type_character == '\0') {
                return global->second;
            }
            if (const auto constant = vba_constant_value(*identifier)) {
                Value value{*constant};
                if (!type_character_matches(value, type_character,
                                            identifier_offset)) {
                    return std::nullopt;
                }
                return value;
            }
            if (auto text = vba_string_constant(*identifier)) {
                Value value{std::move(*text)};
                if (!type_character_matches(value, type_character,
                                            identifier_offset)) {
                    return std::nullopt;
                }
                return value;
            }
        }
        if (!allow_identifiers_) {
            set_error("WFC0002", "expected expression", identifier_offset);
            return std::nullopt;
        }
        const auto variable = find_variable(*identifier);
        if (variable.value == nullptr) {
            // An unqualified read of the current class's own Property
            // Get (no parentheses, like a field read), the same
            // implicit-Me convenience as the method-call branch above.
            // Checked only once find_variable has already found nothing
            // -- a local parameter/variable of the same name (a Property
            // Let's own value parameter commonly shares its property's
            // name, e.g. `Property Let V(v As Long)`) must shadow it,
            // matching ordinary lexical scoping.
            if (type_character == '\0') {
                if (auto* const instance = current_instance()) {
                    if (const auto* const class_def = current_class_def()) {
                        const auto getter_iterator =
                            class_def->property_get.find(*identifier);
                        if (getter_iterator != class_def->property_get.end()) {
                            return invoke_definition(
                                getter_iterator->second, *identifier, {},
                                identifier_offset, class_def->source, instance);
                        }
                    }
                }
            }
            if (type_character == '\0' && *identifier == "app" &&
                class_definitions_.contains("wfcapp")) {
                if (!app_instance_.has_value()) {
                    app_instance_ =
                        instantiate_class("wfcapp", identifier_offset);
                    if (!app_instance_.has_value()) {
                        return std::nullopt;
                    }
                }
                return *app_instance_;
            }
            // A parenthesis-free, zero-argument call to a known
            // module-level Function or class-sibling Function/method
            // (REQ-0213) -- e.g. bare `Print NextId`. Checked only
            // once every variable/Property-Get possibility above has
            // found nothing, so a local name always shadows a
            // same-named procedure, matching ordinary lexical scoping.
            if (procedures_.contains(*identifier) &&
                (type_character == '\0' ||
                 procedures_.at(*identifier).is_function)) {
                return parse_procedure_call(*identifier, identifier_offset);
            }
            if (type_character == '\0') {
                if (auto* const instance = current_instance()) {
                    if (const auto* const class_def = current_class_def()) {
                        if (class_def->methods.contains(*identifier)) {
                            return call_class_method(*instance, *class_def,
                                                     *identifier,
                                                     identifier_offset,
                                                     /*require_function=*/true);
                        }
                    }
                }
            }
            // A parenthesis-free, zero-argument intrinsic function call
            // (REQ-0213) -- e.g. bare `Print Rnd`. parse_function_call
            // itself reports WFC0071 "unsupported function" for any
            // name it does not recognize, before consuming any input;
            // that specific, no-input-consumed failure is
            // indistinguishable from "this was never a function-call
            // attempt at all", so it falls through to the ordinary
            // undeclared-variable diagnostic below instead of
            // surfacing the less helpful "unsupported function"
            // message for what is more likely a typo'd variable name.
            {
                std::string call_identifier = *identifier;
                if (type_character != '\0') {
                    call_identifier.push_back(type_character);
                }
                const auto saved_offset = offset_;
                auto result =
                    parse_function_call(call_identifier, identifier_offset);
                if (result.has_value() || offset_ != saved_offset ||
                    error_.diagnostic.rfind("WFC0071 ", 0) != 0) {
                    return result;
                }
                error_ = wfc::Evaluation{};
            }
            if (!strict_declarations_ && type_character == '\0' &&
                !constant_expression_ && !in_with_identifier(*identifier)) {
                return Value{
                    Empty{}};  // REQ-0265: an undeclared name reads as Empty
            }
            set_error("WFC0015", "undeclared variable", identifier_offset);
            return std::nullopt;
        }
        if (!type_character_matches(*variable.value, type_character,
                                    identifier_offset)) {
            return std::nullopt;
        }
        if (constant_expression_ &&
            !variable.scope->constants.contains(*identifier)) {
            set_error("WFC0064",
                      "constant initializer cannot reference a variable",
                      identifier_offset);
            return std::nullopt;
        }
        if (variable.scope->variant_variables.contains(*identifier)) {
            note_variant_value(*variable.value);
        }
        return *variable.value;
    }

    set_error("WFC0002", "expected expression", offset_);
    return std::nullopt;
}

Value Interpreter::zero_value_for_index(const std::size_t type_index) {
    if (type_index == Value{Integer{}}.index()) {
        return Value{Integer{}};
    }
    if (type_index == Value{Int16{}}.index()) {
        return Value{Int16{}};
    }
    if (type_index == Value{0.0}.index()) {
        return Value{0.0};
    }
    if (type_index == Value{0.0f}.index()) {
        return Value{0.0f};
    }
    if (type_index == Value{Currency{}}.index()) {
        return Value{Currency{}};
    }
    if (type_index == Value{std::string{}}.index()) {
        return Value{std::string{}};
    }
    if (type_index == Value{DateValue{}}.index()) {
        return Value{DateValue{}};
    }
    if (type_index == Value{Byte{}}.index()) {
        return Value{Byte{}};
    }
    if (type_index == Value{Decimal{}}.index()) {
        return Value{Decimal{}};
    }
    // REQ-0228: an omitted Optional object-reference parameter with no
    // explicit `= Nothing` default (`parameter.has_default` false)
    // reaches here via `parameter.type_index` -- `Nothing` is that
    // type's own zero value, not the `Boolean` fallback below.
    if (type_index == Value{Nothing{}}.index()) {
        return Value{Nothing{}};
    }
    return Value{false};
}

std::optional<Value> Interpreter::parse_array_index(
    const Value& array_variable) {
    const auto& array = std::get<ArrayValue>(array_variable);
    advance();  // consume '('
    const auto dimension_count = array_expected_dimension_count(array);
    auto indices = parse_index_list(dimension_count);
    if (!indices.has_value()) {
        return std::nullopt;
    }
    if (!execute_) {
        // array.elements can legitimately be empty here -- a
        // ParamArray (REQ-0206) called with zero extra arguments, or an
        // unallocated dynamic array before its first ReDim (REQ-0207)
        // -- so a placeholder of the array's own declared element type
        // stands in via array_element_default (ArrayValue.
        // element_type_index) rather than reading a nonexistent first
        // element; this is only ever reached while type-checking an
        // unreached branch, where the actual value is discarded.
        return array_element_default(array.element_type_index);
    }
    const auto flat_offset = array_flat_offset(array, *indices);
    if (!flat_offset.has_value()) {
        return std::nullopt;
    }
    const auto& element = array.elements[*flat_offset];
    if (array.is_variant_element) {
        // A Variant element compares and combines like a Variant variable.
        note_variant_value(element);
    }
    return element;
}

void Interpreter::note_variant_value(const Value& value) noexcept {
    variant_operand_seen_ = true;
    if (std::holds_alternative<std::string>(value)) {
        variant_string_seen_ = true;
    } else if (is_number(value)) {
        variant_number_seen_ = true;
    }
}

std::optional<Value> Interpreter::parse_string() {
    advance();
    std::string value;
    while (!at_end() && current() != '\r' && current() != '\n') {
        if (current() != '"') {
            value.push_back(current());
            advance();
            continue;
        }

        advance();
        if (!at_end() && current() == '"') {
            value.push_back('"');
            advance();
            continue;
        }
        return Value{std::move(value)};
    }

    set_error("WFC0003", "unterminated string literal", offset_);
    return std::nullopt;
}

bool Interpreter::lex_number_span() noexcept {
    const auto is_digit = [](const char character) {
        return std::isdigit(static_cast<unsigned char>(character)) != 0;
    };
    const auto number_start = offset_;
    while (!at_end() && is_digit(current())) {
        advance();
    }
    bool is_float = false;
    if (!at_end() && current() == '.' && is_digit(peek(1))) {
        is_float = true;
        advance();
        while (!at_end() && is_digit(current())) {
            advance();
        }
    } else if (!at_end() && current() == '.' && offset_ > number_start) {
        // A trailing dot (`5.`, `5.E2`, `5.#`) is a Double literal, unless an
        // identifier follows (`5.Foo`).
        const char next = peek(1);
        const bool exponent =
            (next == 'e' || next == 'E' || next == 'd' || next == 'D') &&
            (is_digit(peek(2)) ||
             ((peek(2) == '+' || peek(2) == '-') && is_digit(peek(3))));
        if (exponent || std::isalpha(static_cast<unsigned char>(next)) == 0) {
            is_float = true;
            advance();
        }
    }
    if (!at_end() && (current() == 'e' || current() == 'E' ||
                      current() == 'd' || current() == 'D')) {
        const char sign = peek(1);
        const std::size_t digit_ahead = (sign == '+' || sign == '-') ? 2U : 1U;
        if (is_digit(peek(digit_ahead))) {
            is_float = true;
            advance();
            if (!at_end() && (current() == '+' || current() == '-')) {
                advance();
            }
            while (!at_end() && is_digit(current())) {
                advance();
            }
        }
    }
    return is_float;
}

std::optional<Value> Interpreter::parse_double(const std::size_t start,
                                               const std::size_t end) {
    double value{};
    std::string text(source_.substr(start, end - start));
    std::replace(text.begin(), text.end(), 'd', 'e');
    std::replace(text.begin(), text.end(), 'D', 'e');
    const auto conversion =
        std::from_chars(text.data(), text.data() + text.size(), value);
    if (conversion.ec != std::errc{} ||
        conversion.ptr != text.data() + text.size()) {
        set_error("WFC0006", "numeric literal is malformed", start);
        return std::nullopt;
    }
    return Value{value};
}

std::optional<Value> Interpreter::parse_single(const std::size_t start,
                                               const std::size_t end) {
    float value{};
    std::string text(source_.substr(start, end - start));
    std::replace(text.begin(), text.end(), 'd', 'e');
    std::replace(text.begin(), text.end(), 'D', 'e');
    const auto conversion =
        std::from_chars(text.data(), text.data() + text.size(), value);
    if (conversion.ec != std::errc{} ||
        conversion.ptr != text.data() + text.size()) {
        set_error("WFC0006", "numeric literal is malformed", start);
        return std::nullopt;
    }
    return Value{value};
}

std::optional<Value> Interpreter::parse_short_integer(const std::size_t start,
                                                      const std::size_t end) {
    Int16 value{};
    const auto conversion =
        std::from_chars(source_.data() + start, source_.data() + end, value);
    if (conversion.ec != std::errc{} ||
        conversion.ptr != source_.data() + end) {
        set_error("WFC0006", "Integer literal is out of range", start);
        return std::nullopt;
    }
    return Value{value};
}

std::optional<Value> Interpreter::parse_currency(const std::size_t start,
                                                 const std::size_t end) {
    const std::string_view text = source_.substr(start, end - start);
    if (text.find_first_of("eE") != std::string_view::npos) {
        set_error("WFC0006",
                  "Currency literal does not support exponent notation", start);
        return std::nullopt;
    }
    const auto dot = text.find('.');
    const std::string_view integer_part =
        dot == std::string_view::npos ? text : text.substr(0, dot);
    const std::string_view fraction_part = dot == std::string_view::npos
                                               ? std::string_view{}
                                               : text.substr(dot + 1U);
    if (fraction_part.size() > 4U) {
        set_error("WFC0006",
                  "Currency literal supports at most four decimal digits",
                  start);
        return std::nullopt;
    }
    std::uint64_t integer_magnitude{};
    if (!integer_part.empty()) {
        const auto conversion = std::from_chars(
            integer_part.data(), integer_part.data() + integer_part.size(),
            integer_magnitude);
        if (conversion.ec != std::errc{}) {
            set_error("WFC0006", "Currency literal is out of range", start);
            return std::nullopt;
        }
    }
    std::uint64_t fraction_magnitude{};
    if (!fraction_part.empty()) {
        const auto conversion = std::from_chars(
            fraction_part.data(), fraction_part.data() + fraction_part.size(),
            fraction_magnitude);
        if (conversion.ec != std::errc{}) {
            set_error("WFC0006", "numeric literal is malformed", start);
            return std::nullopt;
        }
        for (std::size_t pad = fraction_part.size(); pad < 4U; ++pad) {
            fraction_magnitude *= 10ULL;
        }
    }
    constexpr auto maximum =
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    if (integer_magnitude > maximum / 10000ULL) {
        set_error("WFC0006", "Currency literal is out of range", start);
        return std::nullopt;
    }
    const std::uint64_t scaled_integer = integer_magnitude * 10000ULL;
    if (scaled_integer > maximum - fraction_magnitude) {
        set_error("WFC0006", "Currency literal is out of range", start);
        return std::nullopt;
    }
    return Value{Currency{
        static_cast<std::int64_t>(scaled_integer + fraction_magnitude)}};
}

std::optional<Value> Interpreter::parse_number() {
    const auto start = offset_;
    const bool floating_form = lex_number_span();
    const auto end = offset_;
    char suffix = '\0';
    if (!at_end() &&
        (current() == '#' || current() == '&' || current() == '!' ||
         current() == '@' || current() == '%')) {
        suffix = current();
        advance();
    }
    if (suffix == '!') {
        return parse_single(start, end);
    }
    if (suffix == '@') {
        return parse_currency(start, end);
    }
    if (floating_form || suffix == '#') {
        if (suffix == '&') {
            set_error("WFC0006", "Long literal suffix requires an integer",
                      start);
            return std::nullopt;
        }
        if (suffix == '%') {
            set_error("WFC0006", "Integer literal suffix requires an integer",
                      start);
            return std::nullopt;
        }
        return parse_double(start, end);
    }
    if (suffix == '%') {
        return parse_short_integer(start, end);
    }
    Integer value{};
    const auto conversion =
        std::from_chars(source_.data() + start, source_.data() + end, value);
    if (conversion.ec == std::errc::result_out_of_range) {
        if (suffix != '&') {
            return parse_double(start, end);  // beyond Long: a Double literal
        }
        set_error("WFC0006", "integer literal out of range", start);
        return std::nullopt;
    }
    if (suffix == '\0' && integer_literals_are_integer_ && value >= -32768 &&
        value <= 32767) {
        return Value{
            static_cast<Int16>(value)};  // a small literal is an Integer
    }
    return Value{value};
}

std::optional<Value> Interpreter::parse_negative_number() {
    const auto start = offset_;
    const bool floating_form = lex_number_span();
    const auto end = offset_;
    char suffix = '\0';
    if (!at_end() &&
        (current() == '#' || current() == '&' || current() == '!' ||
         current() == '@' || current() == '%')) {
        suffix = current();
        advance();
    }
    if (suffix == '!') {
        auto value = parse_single(start, end);
        if (!value.has_value()) {
            return std::nullopt;
        }
        return Value{-std::get<float>(*value)};
    }
    if (suffix == '@') {
        auto value = parse_currency(start, end);
        if (!value.has_value()) {
            return std::nullopt;
        }
        return Value{Currency{-std::get<Currency>(*value).scaled}};
    }
    if (floating_form || suffix == '#') {
        if (suffix == '&') {
            set_error("WFC0006", "Long literal suffix requires an integer",
                      start);
            return std::nullopt;
        }
        if (suffix == '%') {
            set_error("WFC0006", "Integer literal suffix requires an integer",
                      start);
            return std::nullopt;
        }
        auto value = parse_double(start, end);
        if (!value.has_value()) {
            return std::nullopt;
        }
        return Value{-std::get<double>(*value)};
    }

    if (suffix == '%') {
        std::uint64_t magnitude{};
        const auto conversion = std::from_chars(
            source_.data() + start, source_.data() + end, magnitude);
        constexpr auto maximum_magnitude =
            static_cast<std::uint64_t>(std::numeric_limits<Int16>::max()) + 1U;
        if (conversion.ec == std::errc::result_out_of_range ||
            magnitude > maximum_magnitude) {
            set_error("WFC0006", "Integer literal is out of range", start);
            return std::nullopt;
        }
        if (magnitude == maximum_magnitude) {
            return Value{std::numeric_limits<Int16>::min()};
        }
        return Value{static_cast<Int16>(-static_cast<Int16>(magnitude))};
    }

    std::uint64_t magnitude{};
    const auto conversion = std::from_chars(source_.data() + start,
                                            source_.data() + end, magnitude);
    constexpr auto maximum_magnitude =
        static_cast<std::uint64_t>(std::numeric_limits<Integer>::max()) + 1U;
    if (conversion.ec == std::errc::result_out_of_range ||
        magnitude > maximum_magnitude) {
        if (suffix != '&') {
            auto wide = parse_double(start, end);
            if (!wide.has_value()) {
                return std::nullopt;
            }
            return Value{-std::get<double>(*wide)};
        }
        set_error("WFC0006", "integer literal out of range", start);
        return std::nullopt;
    }
    if (magnitude == maximum_magnitude) {
        return Value{std::numeric_limits<Integer>::min()};
    }
    if (suffix == '\0' && integer_literals_are_integer_ &&
        magnitude <= 32768U) {
        return Value{static_cast<Int16>(-static_cast<std::int32_t>(magnitude))};
    }
    return Value{static_cast<Integer>(-static_cast<Integer>(magnitude))};
}

std::optional<Value> Interpreter::round_double_to_long(
    const double number, const Integer minimum, const Integer maximum,
    const std::size_t offset) {
    const double rounded = std::nearbyint(number);
    if (!(rounded >= static_cast<double>(minimum) &&
          rounded <= static_cast<double>(maximum))) {
        set_error("WFC0009", "integer overflow", offset);
        return std::nullopt;
    }
    return Value{static_cast<Integer>(rounded)};
}

std::optional<Value> Interpreter::round_double_to_short_integer(
    const double number, const std::size_t offset) {
    const double rounded = std::nearbyint(number);
    if (!(rounded >= static_cast<double>(std::numeric_limits<Int16>::min()) &&
          rounded <= static_cast<double>(std::numeric_limits<Int16>::max()))) {
        set_error("WFC0009", "integer overflow", offset);
        return std::nullopt;
    }
    return Value{static_cast<Int16>(rounded)};
}

const Integer* Interpreter::require_integer(const Value& value,
                                            const std::size_t operator_offset) {
    const auto* integer = std::get_if<Integer>(&value);
    if (integer == nullptr) {
        set_error("WFC0007", "operator requires integer operands",
                  operator_offset);
    }
    return integer;
}

const bool* Interpreter::require_boolean(const Value& value,
                                         const std::size_t operator_offset) {
    const auto* boolean = std::get_if<bool>(&value);
    if (boolean == nullptr) {
        set_error("WFC0019", "logical operator requires Boolean operands",
                  operator_offset);
    }
    return boolean;
}

auto Interpreter::coerce_ternary_operand(const Value& value,
                                         const std::size_t operator_offset)
    -> std::optional<TernaryOperand> {
    if (std::holds_alternative<Null>(value)) {
        return TernaryOperand{true, false};
    }
    if (std::holds_alternative<Empty>(value)) {
        return TernaryOperand{false, false};
    }
    if (!execute_ && !std::holds_alternative<bool>(value)) {
        return TernaryOperand{
            false, false};  // placeholder operand of a not-taken branch
    }
    const auto* boolean = require_boolean(value, operator_offset);
    if (boolean == nullptr) {
        return std::nullopt;
    }
    return TernaryOperand{false, *boolean};
}

std::optional<bool> Interpreter::coerce_condition_boolean(
    const Value& value, const std::size_t offset,
    const std::string_view error_code, const std::string_view error_message) {
    if (std::holds_alternative<Null>(value) ||
        std::holds_alternative<Empty>(value)) {
        return false;
    }
    const auto* boolean = std::get_if<bool>(&value);
    if (boolean == nullptr) {
        // Any non-zero number is True; "True"/"False" and numeric
        // strings convert as CBool would.
        if (is_number(value)) {
            return as_double(value) != 0.0;
        }
        if (const auto* text = std::get_if<std::string>(&value)) {
            std::string lowered;
            for (const char c : *text) {
                lowered.push_back(ascii_lower(c));
            }
            if (lowered == "true") {
                return true;
            }
            if (lowered == "false") {
                return false;
            }
            const auto parsed = parse_numeric_string(*text);
            if (parsed.status == NumericStringStatus::valid) {
                return parsed.value != 0.0;
            }
        }
        set_error(error_code, error_message, offset);
        return std::nullopt;
    }
    return *boolean;
}

bool Interpreter::widen_bitwise_operand(Value& value,
                                        const std::size_t offset) {
    if (const auto* text = std::get_if<std::string>(&value)) {
        const auto parsed = parse_numeric_string(*text);
        if (parsed.status != NumericStringStatus::valid) {
            return true;
        }
        value = parsed.value;
    } else if (!(std::holds_alternative<float>(value) ||
                 std::holds_alternative<double>(value) ||
                 std::holds_alternative<Currency>(value) ||
                 std::holds_alternative<Decimal>(value))) {
        return true;
    }
    return coerce_numeric_value(value, Value{Integer{}}.index(), offset);
}

std::optional<Value> Interpreter::logical_binary(
    const Value& left, const Value& right, const char operation,
    const std::size_t operator_offset) {
    if (execute_ && (std::holds_alternative<ObjectInstance>(left) ||
                     std::holds_alternative<ObjectInstance>(right))) {
        Value resolved_left = left;
        Value resolved_right = right;
        if (!resolve_default_value(resolved_left, operator_offset) ||
            !resolve_default_value(resolved_right, operator_offset)) {
            return std::nullopt;
        }
        if (!std::holds_alternative<ObjectInstance>(resolved_left) &&
            !std::holds_alternative<ObjectInstance>(resolved_right)) {
            return logical_binary(resolved_left, resolved_right, operation,
                                  operator_offset);
        }
    }
    if (execute_) {
        const auto widens = [](const Value& v) {
            return std::holds_alternative<float>(v) ||
                   std::holds_alternative<double>(v) ||
                   std::holds_alternative<Currency>(v) ||
                   std::holds_alternative<Decimal>(v) ||
                   std::holds_alternative<std::string>(v);
        };
        const auto integral = [](const Value& v) {
            return std::holds_alternative<Integer>(v) ||
                   std::holds_alternative<Int16>(v) ||
                   std::holds_alternative<Byte>(v) ||
                   std::holds_alternative<bool>(v);
        };
        Value widened_left = left;
        Value widened_right = right;
        if ((widens(left) || widens(right)) &&
            (widens(left) || integral(left)) &&
            (widens(right) || integral(right)) &&
            (!std::holds_alternative<bool>(left) ||
             !std::holds_alternative<bool>(right))) {
            if (!widen_bitwise_operand(widened_left, operator_offset) ||
                !widen_bitwise_operand(widened_right, operator_offset)) {
                return std::nullopt;
            }
            if (integral(widened_left) && integral(widened_right)) {
                return logical_binary(widened_left, widened_right, operation,
                                      operator_offset);
            }
        }
    }
    // REQ-0244: And/Or/Xor/Eqv/Imp on integer operands are bitwise.
    {
        const auto integer_of =
            [](const Value& v) -> std::optional<std::int32_t> {
            if (const auto* i = std::get_if<Integer>(&v)) {
                return *i;
            }
            if (const auto* i = std::get_if<Int16>(&v)) {
                return static_cast<std::int32_t>(*i);
            }
            if (const auto* i = std::get_if<Byte>(&v)) {
                return static_cast<std::int32_t>(*i);
            }
            return std::nullopt;
        };
        const auto li = integer_of(left);
        const auto ri = integer_of(right);
        const bool lb = std::holds_alternative<bool>(left);
        const bool rb = std::holds_alternative<bool>(right);
        if ((li || ri) && (li || lb) && (ri || rb)) {
            const std::int32_t a = li ? *li : (std::get<bool>(left) ? -1 : 0);
            const std::int32_t b = ri ? *ri : (std::get<bool>(right) ? -1 : 0);
            std::int32_t r{};
            switch (operation) {
                case 'A':
                    r = a & b;
                    break;
                case 'O':
                    r = a | b;
                    break;
                case 'X':
                    r = a ^ b;
                    break;
                case 'E':
                    r = ~(a ^ b);
                    break;
                default:
                    r = ~a | b;
                    break;
            }
            if (std::holds_alternative<Byte>(left) &&
                std::holds_alternative<Byte>(right)) {
                return Value{static_cast<Byte>(r)};
            }
            if (!std::holds_alternative<Integer>(left) &&
                !std::holds_alternative<Integer>(right)) {
                return Value{static_cast<Int16>(
                    r)};  // Byte/Integer/Boolean mixes are Integer
            }
            return Value{static_cast<Integer>(r)};
        }
    }
    const auto left_operand = coerce_ternary_operand(left, operator_offset);
    if (!left_operand.has_value()) {
        return std::nullopt;
    }
    const auto right_operand = coerce_ternary_operand(right, operator_offset);
    if (!right_operand.has_value()) {
        return std::nullopt;
    }
    const std::optional<bool> left_ternary =
        left_operand->is_null ? std::optional<bool>{}
                              : std::optional<bool>{left_operand->value};
    const std::optional<bool> right_ternary =
        right_operand->is_null ? std::optional<bool>{}
                               : std::optional<bool>{right_operand->value};

    std::optional<bool> result;
    switch (operation) {
        case 'A':
            result = ternary_and(left_ternary, right_ternary);
            break;
        case 'O':
            result = ternary_or(left_ternary, right_ternary);
            break;
        case 'X':
            result = ternary_xor(left_ternary, right_ternary);
            break;
        case 'E':
            result = ternary_eqv(left_ternary, right_ternary);
            break;
        case 'I':
            result = ternary_imp(left_ternary, right_ternary);
            break;
        default:
            set_error("WFC0004", "unsupported operator", operator_offset);
            return std::nullopt;
    }
    if (!result.has_value()) {
        return Value{Null{}};
    }
    return Value{*result};
}

int Interpreter::compare_strings(const std::string_view left_in,
                                 const std::string_view right_in) const {
    std::string folded_left;
    std::string folded_right;
    std::string_view left = left_in;
    std::string_view right = right_in;
    if (option_compare_text_ &&
        (!is_ascii_text(left) || !is_ascii_text(right))) {
        folded_left = fold_case(std::string(left));
        folded_right = fold_case(std::string(right));
        left = folded_left;
        right = folded_right;
    }
    const auto common_size = std::min(left.size(), right.size());
    for (std::size_t index = 0; index < common_size; ++index) {
        const auto left_character = static_cast<unsigned char>(
            option_compare_text_ ? ascii_lower(left[index]) : left[index]);
        const auto right_character = static_cast<unsigned char>(
            option_compare_text_ ? ascii_lower(right[index]) : right[index]);
        if (left_character < right_character) {
            return -1;
        }
        if (left_character > right_character) {
            return 1;
        }
    }
    if (left.size() < right.size()) {
        return -1;
    }
    if (left.size() > right.size()) {
        return 1;
    }
    return 0;
}

bool Interpreter::values_equal(const Value& left, const Value& right) const {
    const auto* left_string = std::get_if<std::string>(&left);
    const auto* right_string = std::get_if<std::string>(&right);
    if (left_string != nullptr || right_string != nullptr) {
        // Empty beside a String acts as the empty String; any other mix is
        // simply unequal.
        static const std::string empty_text;
        const std::string* a = left_string != nullptr ? left_string
                               : std::holds_alternative<Empty>(left)
                                   ? &empty_text
                                   : nullptr;
        const std::string* b = right_string != nullptr ? right_string
                               : std::holds_alternative<Empty>(right)
                                   ? &empty_text
                                   : nullptr;
        return a != nullptr && b != nullptr && compare_strings(*a, *b) == 0;
    }
    return left == right;
}

std::optional<Value> Interpreter::compare(const Value& left, const Value& right,
                                          const std::string_view operation,
                                          const std::size_t operator_offset) {
    // A comparison against Null yields Null itself (three-valued
    // logic), not True or False: VB6 famously cannot answer "x = Null"
    // definitively, which is why IsNull exists. Verified against the
    // local VB6 6.00.8176 reference.
    if (std::holds_alternative<Null>(left) ||
        std::holds_alternative<Null>(right)) {
        return Value{Null{}};
    }
    // An object beside a value compares through its default member.
    if (execute_ && (std::holds_alternative<ObjectInstance>(left) ||
                     std::holds_alternative<ObjectInstance>(right))) {
        Value resolved_left = left;
        Value resolved_right = right;
        if (!resolve_default_value(resolved_left, operator_offset) ||
            !resolve_default_value(resolved_right, operator_offset)) {
            return std::nullopt;
        }
        if (!is_object_reference(resolved_left) &&
            !is_object_reference(resolved_right)) {
            return compare(resolved_left, resolved_right, operation,
                           operator_offset);
        }
    }
    // Object references compare only through Is, matching real VB6
    // (plain =/<> on an object reference requires a default member,
    // which nothing in this evaluator's minimal object model has).
    if (is_object_reference(left) || is_object_reference(right)) {
        set_error("WFC0107", "object comparison requires Is", operator_offset);
        return std::nullopt;
    }
    // Empty coerces to whichever side of the comparison the other
    // operand expects: a number if compared against a number, an empty
    // string if compared against a String. Verified (Empty = 0 and
    // Empty = "" are both True).
    if (std::holds_alternative<Empty>(left) ||
        std::holds_alternative<Empty>(right)) {
        Value coerced_left = left;
        Value coerced_right = right;
        if (std::holds_alternative<Empty>(left)) {
            coerced_left = std::holds_alternative<std::string>(right)
                               ? Value{std::string{}}
                               : Value{Integer{0}};
        }
        if (std::holds_alternative<Empty>(right)) {
            coerced_right = std::holds_alternative<std::string>(left)
                                ? Value{std::string{}}
                                : Value{Integer{0}};
        }
        return compare(coerced_left, coerced_right, operation, operator_offset);
    }
    // REQ-0242: a Date compares by serial against a Date or a number.
    if (std::holds_alternative<DateValue>(left) ||
        std::holds_alternative<DateValue>(right)) {
        const auto serial_of = [](const Value& v) -> std::optional<double> {
            if (const auto* d = std::get_if<DateValue>(&v)) {
                return d->serial;
            }
            if (is_number(v)) {
                return as_double(v);
            }
            return std::nullopt;
        };
        const auto l = serial_of(left);
        const auto r = serial_of(right);
        if (!l.has_value() || !r.has_value()) {
            set_error("WFC0018",
                      "comparison requires operands of the same type",
                      operator_offset);
            return std::nullopt;
        }
        return compare(Value{*l}, Value{*r}, operation, operator_offset);
    }
    // Boolean beside a number compares as the number (True = -1).
    if (std::holds_alternative<bool>(left) !=
        std::holds_alternative<bool>(right)) {
        const auto as_small = [](const Value& v) -> Value {
            if (const auto* flag = std::get_if<bool>(&v)) {
                return Value{static_cast<Int16>(*flag ? -1 : 0)};
            }
            return v;
        };
        const Value other = std::holds_alternative<bool>(left) ? right : left;
        if (is_number(other) && !std::holds_alternative<DateValue>(other)) {
            return compare(as_small(left), as_small(right), operation,
                           operator_offset);
        }
    }
    // A String beside a number: Variant operands follow VB's Variant comparison
    // rules.
    if ((std::holds_alternative<std::string>(left) && is_number(right)) ||
        (std::holds_alternative<std::string>(right) && is_number(left))) {
        if ((variant_string_seen_ || variant_number_seen_) && execute_) {
            const bool string_left = std::holds_alternative<std::string>(left);
            const std::string& text =
                std::get<std::string>(string_left ? left : right);
            const Value& number = string_left ? right : left;
            int ordering{};
            if (variant_string_seen_ && variant_number_seen_) {
                ordering = string_left ? 1 : -1;  // a numeric Variant sorts
                                                  // before a string Variant
            } else if (variant_string_seen_) {
                const auto parsed = parse_numeric_string(text);
                if (parsed.status != NumericStringStatus::valid) {
                    set_error("WFC0018",
                              "comparison requires operands of the same type",
                              operator_offset);
                    return std::nullopt;
                }
                const double l = string_left ? parsed.value : as_double(number);
                const double r = string_left ? as_double(number) : parsed.value;
                ordering = l < r ? -1 : l > r ? 1 : 0;
            } else {
                const std::string rendered = render(number);
                ordering = string_left ? compare_strings(text, rendered)
                                       : compare_strings(rendered, text);
            }
            if (operation == "=") {
                return Value{ordering == 0};
            }
            if (operation == "<>") {
                return Value{ordering != 0};
            }
            if (operation == "<") {
                return Value{ordering < 0};
            }
            if (operation == "<=") {
                return Value{ordering <= 0};
            }
            if (operation == ">") {
                return Value{ordering > 0};
            }
            return Value{ordering >= 0};
        }
    }
    // Long and Double operands compare numerically, in either combination;
    // int32 widens to double exactly, so the ordering is precise.
    if (is_number(left) && is_number(right)) {
        const double left_value = as_double(left);
        const double right_value = as_double(right);
        if (operation == "=") {
            return Value{left_value == right_value};
        }
        if (operation == "<>") {
            return Value{left_value != right_value};
        }
        if (operation == "<") {
            return Value{left_value < right_value};
        }
        if (operation == "<=") {
            return Value{left_value <= right_value};
        }
        if (operation == ">") {
            return Value{left_value > right_value};
        }
        if (operation == ">=") {
            return Value{left_value >= right_value};
        }
        set_error("WFC0004", "unsupported operator", operator_offset);
        return std::nullopt;
    }

    if (left.index() != right.index()) {
        if (!execute_) {
            return Value{false};  // placeholder operand of a not-taken branch
        }
        set_error("WFC0018", "comparison requires operands of the same type",
                  operator_offset);
        return std::nullopt;
    }

    if (operation == "=") {
        return Value{values_equal(left, right)};
    }
    if (operation == "<>") {
        return Value{!values_equal(left, right)};
    }
    if (std::holds_alternative<bool>(left)) {
        set_error("WFC0018", "Boolean ordering is not supported",
                  operator_offset);
        return std::nullopt;
    }
    if (!std::holds_alternative<std::string>(left)) {
        set_error("WFC0018", "ordering is not supported for this type",
                  operator_offset);
        return std::nullopt;
    }

    bool less{};
    bool greater{};
    {
        const auto& left_string = std::get<std::string>(left);
        const auto& right_string = std::get<std::string>(right);
        const auto ordering = compare_strings(left_string, right_string);
        less = ordering < 0;
        greater = ordering > 0;
    }

    if (operation == "<") {
        return Value{less};
    }
    if (operation == "<=") {
        return Value{!greater};
    }
    if (operation == ">") {
        return Value{greater};
    }
    if (operation == ">=") {
        return Value{!less};
    }
    set_error("WFC0004", "unsupported operator", operator_offset);
    return std::nullopt;
}

Value Interpreter::widen_byte(const Value& value) {
    if (const auto* byte = std::get_if<Byte>(&value)) {
        return Value{static_cast<Integer>(*byte)};
    }
    if (const auto* flag = std::get_if<bool>(&value)) {
        return Value{static_cast<Int16>(*flag ? -1 : 0)};  // True is -1
    }
    return value;
}

std::optional<Value> Interpreter::numeric_binary(
    const Value& left_in, const Value& right_in, const char operation,
    const std::size_t operator_offset) {
    if (execute_ && (std::holds_alternative<ObjectInstance>(left_in) ||
                     std::holds_alternative<ObjectInstance>(right_in))) {
        Value resolved_left = left_in;
        Value resolved_right = right_in;
        if (!resolve_default_value(resolved_left, operator_offset) ||
            !resolve_default_value(resolved_right, operator_offset)) {
            return std::nullopt;
        }
        if (!std::holds_alternative<ObjectInstance>(resolved_left) &&
            !std::holds_alternative<ObjectInstance>(resolved_right)) {
            return numeric_binary(resolved_left, resolved_right, operation,
                                  operator_offset);
        }
    }
    // Byte/Integer operands keep their own width: Byte op Byte is a Byte,
    // anything else small is an Integer (a Variant operand promotes instead of
    // overflowing).
    if ((operation == '+' || operation == '-' || operation == '*' ||
         operation == '\\' || operation == '%') &&
        (std::holds_alternative<Byte>(left_in) ||
         std::holds_alternative<Int16>(left_in)) &&
        (std::holds_alternative<Byte>(right_in) ||
         std::holds_alternative<Int16>(right_in))) {
        const bool both_bytes = std::holds_alternative<Byte>(left_in) &&
                                std::holds_alternative<Byte>(right_in);
        if (!execute_) {
            return both_bytes ? Value{Byte{}} : Value{Int16{}};
        }
        const auto small_value = [](const Value& v) -> std::int64_t {
            if (const auto* b = std::get_if<Byte>(&v)) {
                return *b;
            }
            return std::get<Int16>(v);
        };
        const std::int64_t a = small_value(left_in);
        const std::int64_t b = small_value(right_in);
        if ((operation == '\\' || operation == '%') && b == 0) {
            set_error("WFC0008", "division by zero", operator_offset);
            return std::nullopt;
        }
        std::int64_t result{};
        switch (operation) {
            case '+':
                result = a + b;
                break;
            case '-':
                result = a - b;
                break;
            case '*':
                result = a * b;
                break;
            case '\\':
                result = a / b;
                break;
            default:
                result = a % b;
                break;
        }
        if (both_bytes && result >= 0 && result <= 255) {
            return Value{static_cast<Byte>(result)};
        }
        if (!both_bytes && result >= -32768 && result <= 32767) {
            return Value{static_cast<Int16>(result)};
        }
        if (both_bytes && result >= -32768 && result <= 32767 &&
            variant_operand_seen_) {
            return Value{static_cast<Int16>(result)};
        }
        if (variant_operand_seen_) {
            return Value{static_cast<Integer>(result)};
        }
        set_error("WFC0009", "integer overflow", operator_offset);
        return std::nullopt;
    }
    const Value left = widen_byte(left_in);
    const Value right = widen_byte(right_in);
    if (operation == '+' && std::holds_alternative<std::string>(left) &&
        std::holds_alternative<std::string>(right)) {
        return Value{std::get<std::string>(left) +
                     std::get<std::string>(right)};
    }
    if (std::holds_alternative<Null>(left) ||
        std::holds_alternative<Null>(right)) {
        return Value{Null{}};
    }
    if (std::holds_alternative<Empty>(left) ||
        std::holds_alternative<Empty>(right)) {
        const Value coerced_left =
            std::holds_alternative<Empty>(left) ? Value{Integer{0}} : left;
        const Value coerced_right =
            std::holds_alternative<Empty>(right) ? Value{Integer{0}} : right;
        return numeric_binary(coerced_left, coerced_right, operation,
                              operator_offset);
    }
    {
        // REQ-0269: a numeric String beside a number (or another numeric
        // String under - * /) converts to Double, as VB does for Variants.
        const bool left_text = std::holds_alternative<std::string>(left);
        const bool right_text = std::holds_alternative<std::string>(right);
        if ((left_text || right_text) &&
            !(left_text && right_text && operation == '+')) {
            const auto convert = [&](const Value& v) -> std::optional<Value> {
                if (const auto* text = std::get_if<std::string>(&v)) {
                    const auto parsed = parse_numeric_string(*text);
                    if (parsed.status != NumericStringStatus::valid) {
                        return std::nullopt;
                    }
                    return Value{parsed.value};
                }
                if (is_number(v) && !std::holds_alternative<DateValue>(v)) {
                    return v;
                }
                return std::nullopt;
            };
            const auto converted_left = convert(left);
            const auto converted_right = convert(right);
            if (converted_left && converted_right) {
                return numeric_binary(*converted_left, *converted_right,
                                      operation, operator_offset);
            }
            if (execute_) {
                err_number_ = 13;
                err_erl_ = erl_;
                err_description_ = "Type mismatch";
                set_error("WFC0300", "Type mismatch", operator_offset);
                return std::nullopt;
            }
            return Value{0.0};
        }
    }
    if (std::holds_alternative<DateValue>(left) ||
        std::holds_alternative<DateValue>(right)) {
        // REQ-0242: Date +/- number stays a Date; Date - Date is a
        // Double day count; everything else computes as Double.
        const auto* left_date = std::get_if<DateValue>(&left);
        const auto* right_date = std::get_if<DateValue>(&right);
        const auto to_double = [](const Value& v) -> std::optional<double> {
            if (const auto* d = std::get_if<DateValue>(&v)) {
                return d->serial;
            }
            if (is_number(v)) {
                return as_double(v);
            }
            return std::nullopt;
        };
        const auto l = to_double(left);
        const auto r = to_double(right);
        if (!l.has_value() || !r.has_value()) {
            static_cast<void>(
                require_integer(l.has_value() ? right : left, operator_offset));
            return std::nullopt;
        }
        double result{};
        switch (operation) {
            case '+':
                result = *l + *r;
                break;
            case '-':
                result = *l - *r;
                break;
            case '*':
                result = *l * *r;
                break;
            default:
                if (*r == 0.0) {
                    set_error("WFC0008", "division by zero", operator_offset);
                    return std::nullopt;
                }
                result = *l / *r;
                break;
        }
        if ((operation == '+' &&
             (left_date != nullptr) != (right_date != nullptr)) ||
            (operation == '-' && left_date != nullptr &&
             right_date == nullptr)) {
            return Value{DateValue{result}};
        }
        if (operation == '+' && left_date != nullptr && right_date != nullptr) {
            return Value{DateValue{result}};
        }
        return Value{result};
    }
    if (operation != '/' && std::holds_alternative<Integer>(left) &&
        std::holds_alternative<Integer>(right)) {
        return integer_binary(left, right, operation, operator_offset);
    }
    if (operation != '/' && std::holds_alternative<Int16>(left) &&
        std::holds_alternative<Int16>(right)) {
        return short_integer_binary(left, right, operation, operator_offset);
    }
    if (!is_number(left) || !is_number(right)) {
        if (require_integer(left, operator_offset) == nullptr) {
            return std::nullopt;
        }
        static_cast<void>(require_integer(right, operator_offset));
        return std::nullopt;
    }

    auto category = std::max(numeric_category(left), numeric_category(right));
    if (operation != '/' && (category == NumericCategory::integer ||
                             category == NumericCategory::int16)) {
        // Both-Long and both-Int16 were already short-circuited above, so
        // reaching here with an integral-only category means one operand
        // is Int16 and the other Long: widen the Int16 side exactly (it
        // always fits) and compute exact checked Long arithmetic, the
        // same as Long op Long.
        const auto widen_to_long = [](const Value& value) -> Value {
            if (const auto* short_integer = std::get_if<Int16>(&value)) {
                return Value{static_cast<Integer>(*short_integer)};
            }
            return value;
        };
        return integer_binary(widen_to_long(left), widen_to_long(right),
                              operation, operator_offset);
    }
    if (category == NumericCategory::integer ||
        category == NumericCategory::int16) {
        // Only reachable for `/` between any combination of Long/Int16,
        // which always promotes to Double under the pre-existing rule.
        category = NumericCategory::double_precision;
    }

    if (!execute_) {
        switch (category) {
            case NumericCategory::currency:
                return Value{Currency{}};
            case NumericCategory::single:
                return Value{0.0f};
            case NumericCategory::decimal_precision:
                return Value{Decimal{}};
            default:
                return Value{0.0};
        }
    }

    if (category == NumericCategory::decimal_precision) {
        // Int16, Long, Currency, Single, or Double can all be the *other*
        // operand here (each sorts below decimal_precision), so each
        // needs an exact or best-effort widening to Decimal. Currency
        // widens exactly (its scaled int64 becomes a scale-4 Decimal
        // mantissa). Int16, Long, Single, and Double have no dedicated
        // branch below and fall through to the generic Double-to-Decimal
        // conversion via `as_double`, which is exact for Int16/Long and
        // best-effort (shortest round-tripping decimal text) for
        // Single/Double.
        const auto to_decimal = [](const Value& value) -> Decimal {
            if (const auto* decimal = std::get_if<Decimal>(&value)) {
                return *decimal;
            }
            if (const auto* integer = std::get_if<Integer>(&value)) {
                Decimal result;
                result.negative = *integer < 0;
                result.mantissa = big_from_u32(static_cast<std::uint32_t>(
                    *integer < 0 ? -static_cast<std::int64_t>(*integer)
                                 : *integer));
                return result;
            }
            if (const auto* currency = std::get_if<Currency>(&value)) {
                Decimal result;
                result.negative = currency->scaled < 0;
                const auto magnitude =
                    currency->scaled < 0
                        ? (~static_cast<std::uint64_t>(currency->scaled) + 1ULL)
                        : static_cast<std::uint64_t>(currency->scaled);
                result.mantissa.limb[0] = static_cast<std::uint32_t>(magnitude);
                result.mantissa.limb[1] =
                    static_cast<std::uint32_t>(magnitude >> 32U);
                result.scale = 4U;
                return result;
            }
            return decimal_from_double(as_double(value)).value_or(Decimal{});
        };
        const Decimal left_decimal = to_decimal(left);
        const Decimal right_decimal = to_decimal(right);
        std::optional<Decimal> result;
        switch (operation) {
            case '+':
                result = decimal_add(left_decimal, right_decimal);
                break;
            case '-':
                result = decimal_subtract(left_decimal, right_decimal);
                break;
            case '*':
                result = decimal_multiply(left_decimal, right_decimal);
                break;
            case '/':
                if (is_zero_big(right_decimal.mantissa)) {
                    set_error("WFC0008", "division by zero", operator_offset);
                    return std::nullopt;
                }
                result = decimal_divide(left_decimal, right_decimal);
                break;
            default:
                set_error("WFC0004", "unsupported operator", operator_offset);
                return std::nullopt;
        }
        if (!result.has_value()) {
            set_error("WFC0009", "numeric overflow", operator_offset);
            return std::nullopt;
        }
        return Value{*result};
    }

    if (category == NumericCategory::currency) {
        const std::int64_t left_scaled = as_currency_scaled(left);
        const std::int64_t right_scaled = as_currency_scaled(right);
        std::optional<std::int64_t> result;
        switch (operation) {
            case '+': {
                std::int64_t sum{};
                const bool overflowed =
                    (right_scaled > 0 &&
                     left_scaled > std::numeric_limits<std::int64_t>::max() -
                                       right_scaled) ||
                    (right_scaled < 0 &&
                     left_scaled < std::numeric_limits<std::int64_t>::min() -
                                       right_scaled);
                if (!overflowed) {
                    sum = left_scaled + right_scaled;
                    result = sum;
                }
                break;
            }
            case '-': {
                std::int64_t difference{};
                const bool overflowed =
                    (right_scaled < 0 &&
                     left_scaled > std::numeric_limits<std::int64_t>::max() +
                                       right_scaled) ||
                    (right_scaled > 0 &&
                     left_scaled < std::numeric_limits<std::int64_t>::min() +
                                       right_scaled);
                if (!overflowed) {
                    difference = left_scaled - right_scaled;
                    result = difference;
                }
                break;
            }
            case '*':
                result = currency_multiply(left_scaled, right_scaled);
                break;
            case '/':
                if (right_scaled == 0) {
                    set_error("WFC0008", "division by zero", operator_offset);
                    return std::nullopt;
                }
                result = currency_divide(left_scaled, right_scaled);
                break;
            default:
                set_error("WFC0004", "unsupported operator", operator_offset);
                return std::nullopt;
        }
        if (!result.has_value()) {
            set_error("WFC0009", "numeric overflow", operator_offset);
            return std::nullopt;
        }
        return Value{Currency{*result}};
    }

    if (category == NumericCategory::single) {
        const float left_value = as_single(left);
        const float right_value = as_single(right);
        float result{};
        switch (operation) {
            case '+':
                result = left_value + right_value;
                break;
            case '-':
                result = left_value - right_value;
                break;
            case '*':
                result = left_value * right_value;
                break;
            case '/':
                if (right_value == 0.0f) {
                    set_error("WFC0008", "division by zero", operator_offset);
                    return std::nullopt;
                }
                result = left_value / right_value;
                break;
            default:
                set_error("WFC0004", "unsupported operator", operator_offset);
                return std::nullopt;
        }
        if (!std::isfinite(result)) {
            set_error("WFC0009", "numeric overflow", operator_offset);
            return std::nullopt;
        }
        return Value{result};
    }

    const double left_value = as_double(left);
    const double right_value = as_double(right);
    double result{};
    switch (operation) {
        case '+':
            result = left_value + right_value;
            break;
        case '-':
            result = left_value - right_value;
            break;
        case '*':
            result = left_value * right_value;
            break;
        case '/':
            if (right_value == 0.0) {
                set_error("WFC0008", "division by zero", operator_offset);
                return std::nullopt;
            }
            result = left_value / right_value;
            break;
        default:
            set_error("WFC0004", "unsupported operator", operator_offset);
            return std::nullopt;
    }
    if (!std::isfinite(result)) {
        set_error("WFC0009", "numeric overflow", operator_offset);
        return std::nullopt;
    }
    return Value{result};
}

std::optional<Integer> Interpreter::coerce_long(
    const Value& value, const std::size_t operator_offset) {
    if (const auto* integer = std::get_if<Integer>(&value)) {
        return *integer;
    }
    if (const auto* short_integer = std::get_if<Int16>(&value)) {
        return static_cast<Integer>(*short_integer);
    }
    if (const auto* number = std::get_if<double>(&value)) {
        const double rounded = std::nearbyint(*number);
        if (!(rounded >=
                  static_cast<double>(std::numeric_limits<Integer>::min()) &&
              rounded <=
                  static_cast<double>(std::numeric_limits<Integer>::max()))) {
            set_error("WFC0009", "integer overflow", operator_offset);
            return std::nullopt;
        }
        return static_cast<Integer>(rounded);
    }
    if (const auto* single = std::get_if<float>(&value)) {
        const double rounded = std::nearbyint(static_cast<double>(*single));
        if (!(rounded >=
                  static_cast<double>(std::numeric_limits<Integer>::min()) &&
              rounded <=
                  static_cast<double>(std::numeric_limits<Integer>::max()))) {
            set_error("WFC0009", "integer overflow", operator_offset);
            return std::nullopt;
        }
        return static_cast<Integer>(rounded);
    }
    if (std::holds_alternative<Currency>(value) ||
        std::holds_alternative<Decimal>(value)) {
        const double rounded = std::nearbyint(as_double(value));
        if (!(rounded >=
                  static_cast<double>(std::numeric_limits<Integer>::min()) &&
              rounded <=
                  static_cast<double>(std::numeric_limits<Integer>::max()))) {
            set_error("WFC0009", "integer overflow", operator_offset);
            return std::nullopt;
        }
        return static_cast<Integer>(rounded);
    }
    if (const auto* byte = std::get_if<Byte>(&value)) {
        return static_cast<Integer>(*byte);
    }
    if (const auto* flag = std::get_if<bool>(&value)) {
        return *flag ? Integer{-1} : Integer{0};
    }
    if (std::holds_alternative<Empty>(value)) {
        return Integer{0};
    }
    if (const auto* text = std::get_if<std::string>(&value);
        text != nullptr && execute_) {
        const auto parsed = parse_numeric_string(*text);
        if (parsed.status == NumericStringStatus::valid &&
            std::fabs(parsed.value) < 2147483647.5) {
            return static_cast<Integer>(std::nearbyint(parsed.value));
        }
    }
    if (const auto* date = std::get_if<DateValue>(&value)) {
        return static_cast<Integer>(std::nearbyint(date->serial));
    }
    if (!execute_) {
        return Integer{0};  // a placeholder operand of a not-taken branch
    }
    set_error("WFC0007", "operator requires integer operands", operator_offset);
    return std::nullopt;
}

std::optional<Value> Interpreter::integer_binary(
    const Value& left_in, const Value& right_in, const char operation,
    const std::size_t operator_offset) {
    if (execute_ && (std::holds_alternative<ObjectInstance>(left_in) ||
                     std::holds_alternative<ObjectInstance>(right_in))) {
        Value resolved_left = left_in;
        Value resolved_right = right_in;
        if (!resolve_default_value(resolved_left, operator_offset) ||
            !resolve_default_value(resolved_right, operator_offset)) {
            return std::nullopt;
        }
        if (!std::holds_alternative<ObjectInstance>(resolved_left) &&
            !std::holds_alternative<ObjectInstance>(resolved_right)) {
            return integer_binary(resolved_left, resolved_right, operation,
                                  operator_offset);
        }
    }
    if ((std::holds_alternative<Byte>(left_in) ||
         std::holds_alternative<Int16>(left_in)) &&
        (std::holds_alternative<Byte>(right_in) ||
         std::holds_alternative<Int16>(right_in))) {
        return numeric_binary(left_in, right_in, operation, operator_offset);
    }
    const Value left = widen_byte(left_in);
    const Value right = widen_byte(right_in);
    const auto left_coerced = coerce_long(left, operator_offset);
    if (!left_coerced.has_value()) {
        return std::nullopt;
    }
    const auto right_coerced = coerce_long(right, operator_offset);
    if (!right_coerced.has_value()) {
        return std::nullopt;
    }
    const Integer left_integer = *left_coerced;
    const Integer right_integer = *right_coerced;
    if (!execute_) {
        return Value{Integer{}};
    }

    if ((operation == '\\' || operation == '%') && right_integer == 0) {
        set_error("WFC0008", "division by zero", operator_offset);
        return std::nullopt;
    }
    if ((operation == '\\' || operation == '%') &&
        left_integer == std::numeric_limits<Integer>::min() &&
        right_integer == -1) {
        set_error("WFC0009", "integer overflow", operator_offset);
        return std::nullopt;
    }

    std::int64_t result{};
    switch (operation) {
        case '+':
            result = static_cast<std::int64_t>(left_integer) + right_integer;
            break;
        case '-':
            result = static_cast<std::int64_t>(left_integer) - right_integer;
            break;
        case '*':
            result = static_cast<std::int64_t>(left_integer) * right_integer;
            break;
        case '\\':
            result = left_integer / right_integer;
            break;
        case '%':
            result = left_integer % right_integer;
            break;
        default:
            set_error("WFC0004", "unsupported operator", operator_offset);
            return std::nullopt;
    }

    if (result < std::numeric_limits<Integer>::min() ||
        result > std::numeric_limits<Integer>::max()) {
        if (variant_operand_seen_ &&
            (operation == '+' || operation == '-' || operation == '*')) {
            return Value{static_cast<double>(result)};
        }
        set_error("WFC0009", "integer overflow", operator_offset);
        return std::nullopt;
    }
    return Value{static_cast<Integer>(result)};
}

std::optional<Value> Interpreter::short_integer_binary(
    const Value& left, const Value& right, const char operation,
    const std::size_t operator_offset) {
    const Int16 left_integer = std::get<Int16>(left);
    const Int16 right_integer = std::get<Int16>(right);
    if (!execute_) {
        return Value{Int16{}};
    }

    std::int32_t result{};
    switch (operation) {
        case '+':
            result = static_cast<std::int32_t>(left_integer) + right_integer;
            break;
        case '-':
            result = static_cast<std::int32_t>(left_integer) - right_integer;
            break;
        case '*':
            result = static_cast<std::int32_t>(left_integer) * right_integer;
            break;
        default:
            set_error("WFC0004", "unsupported operator", operator_offset);
            return std::nullopt;
    }

    if (result < std::numeric_limits<Int16>::min() ||
        result > std::numeric_limits<Int16>::max()) {
        if (variant_operand_seen_) {
            return Value{static_cast<Integer>(result)};
        }
        set_error("WFC0009", "integer overflow", operator_offset);
        return std::nullopt;
    }
    return Value{static_cast<Int16>(result)};
}

}  // namespace wfc::detail
