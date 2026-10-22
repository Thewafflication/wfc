// The value-level face of the interpreter for native programs (ADR-0009).

#include "native_engine.hpp"

#include <cmath>
#include <limits>

#include "interpreter.hpp"

namespace wfc::detail {

// A friend of `Interpreter`: the operators and conversions are its private
// members.
struct NativeSupport {
    static void take_error(Interpreter& in, NativeError& error) {
        error.number = static_cast<int>(in.runtime_error_number());
        if (error.number == 0) {
            error.number = 5;  // an error with no Visual Basic number
        }
        error.description =
            in.err_number_ == error.number && !in.err_description_.empty()
                ? in.err_description_
                : Interpreter::vb_error_description(
                      static_cast<Integer>(error.number));
        in.error_ = wfc::Evaluation{};
    }

    static bool is_null(const Value& value) {
        return std::holds_alternative<Null>(value);
    }

    static std::optional<Value> binary(Interpreter& in, std::string_view op,
                                       const Value& left, const Value& right,
                                       NativeError& error) {
        std::optional<Value> result;
        // An operand is a Variant: a result that overflows promotes instead.
        in.variant_operand_seen_ = true;
        if (op == "+" || op == "-" || op == "*" || op == "/") {
            result = in.numeric_binary(left, right, op.front(), 0U);
        } else if (op == "\\") {
            result = in.integer_binary(left, right, '\\', 0U);
        } else if (op == "mod") {
            result = in.integer_binary(left, right, '%', 0U);
        } else if (op == "=" || op == "<>" || op == "<" || op == ">" ||
                   op == "<=" || op == ">=") {
            result = in.compare(left, right, op, 0U);
        } else if (op == "and") {
            result = in.logical_binary(left, right, 'A', 0U);
        } else if (op == "or") {
            result = in.logical_binary(left, right, 'O', 0U);
        } else if (op == "xor") {
            result = in.logical_binary(left, right, 'X', 0U);
        } else if (op == "eqv") {
            result = in.logical_binary(left, right, 'E', 0U);
        } else if (op == "imp") {
            result = in.logical_binary(left, right, 'I', 0U);
        } else if (op == "&") {
            result = concatenate(in, left, right);
        } else if (op == "^") {
            result = power(in, left, right);
        } else {
            in.set_error("WFC0010", "unknown operator", 0U);
        }
        in.variant_operand_seen_ = false;
        if (!result.has_value()) {
            take_error(in, error);
        }
        return result;
    }

    // `&`: two Nulls are an error, one Null is an empty String.
    static std::optional<Value> concatenate(Interpreter& in, const Value& left,
                                            const Value& right) {
        const bool left_null = is_null(left);
        const bool right_null = is_null(right);
        if (left_null && right_null) {
            in.set_error("WFC0104", "Invalid use of Null", 0U);
            return std::nullopt;
        }
        Value a = left;
        Value b = right;
        if (!in.resolve_default_value(a, 0U) ||
            !in.resolve_default_value(b, 0U)) {
            return std::nullopt;
        }
        if (is_object_reference(a) || is_object_reference(b) ||
            std::holds_alternative<ArrayValue>(a) ||
            std::holds_alternative<ArrayValue>(b)) {
            in.set_error("WFC0020",
                         "concatenation requires String or Long operands", 0U);
            return std::nullopt;
        }
        return Value{(left_null ? std::string{} : NativeSupport::render(a)) +
                     (right_null ? std::string{} : NativeSupport::render(b))};
    }

    static std::string render(const Value& value) {
        return Interpreter::render(value);
    }

    static std::optional<double> number_of(const Value& value) {
        if (std::holds_alternative<Empty>(value)) {
            return 0.0;
        }
        if (is_number(value)) {
            return as_double(value);
        }
        if (const auto* date = std::get_if<DateValue>(&value)) {
            return date->serial;
        }
        if (const auto* flag = std::get_if<bool>(&value)) {
            return *flag ? -1.0 : 0.0;
        }
        if (const auto* text = std::get_if<std::string>(&value)) {
            const auto parsed = parse_numeric_string(*text);
            if (parsed.status == NumericStringStatus::valid) {
                return parsed.value;
            }
        }
        return std::nullopt;
    }

    static std::optional<Value> power(Interpreter& in, const Value& left,
                                      const Value& right) {
        if (is_null(left) || is_null(right)) {
            return Value{Null{}};
        }
        const auto base = number_of(left);
        const auto exponent = number_of(right);
        if (!base.has_value() || !exponent.has_value()) {
            in.set_error("WFC0007", "operator requires numeric operands", 0U);
            return std::nullopt;
        }
        if (*base == 0.0 && *exponent < 0.0) {
            in.set_error("WFC0008", "division by zero", 0U);
            return std::nullopt;
        }
        const double result = std::pow(*base, *exponent);
        if (!std::isfinite(result)) {
            in.set_error("WFC0009", "numeric overflow", 0U);
            return std::nullopt;
        }
        return Value{result};
    }

    static std::optional<Value> negate(Interpreter& in, Value value,
                                       NativeError& error) {
        std::optional<Value> result;
        if (const auto* flag = std::get_if<bool>(&value)) {
            value = Value{static_cast<Int16>(*flag ? -1 : 0)};
        }
        if (const auto* byte = std::get_if<Byte>(&value)) {
            value = Value{static_cast<Int16>(*byte)};
        }
        if (is_null(value)) {
            return value;
        }
        if (std::holds_alternative<Empty>(value)) {
            return Value{Integer{0}};
        }
        if (const auto* number = std::get_if<double>(&value)) {
            return Value{-*number};
        }
        if (const auto* single = std::get_if<float>(&value)) {
            return Value{-*single};
        }
        if (const auto* decimal = std::get_if<Decimal>(&value)) {
            Decimal negated = *decimal;
            if (!is_zero_big(negated.mantissa)) {
                negated.negative = !negated.negative;
            }
            return Value{negated};
        }
        if (const auto* currency = std::get_if<Currency>(&value)) {
            if (currency->scaled == std::numeric_limits<std::int64_t>::min()) {
                in.set_error("WFC0009", "numeric overflow", 0U);
                take_error(in, error);
                return std::nullopt;
            }
            return Value{Currency{-currency->scaled}};
        }
        if (const auto* short_integer = std::get_if<Int16>(&value)) {
            if (*short_integer == std::numeric_limits<Int16>::min()) {
                in.set_error("WFC0009", "integer overflow", 0U);
                take_error(in, error);
                return std::nullopt;
            }
            return Value{static_cast<Int16>(-*short_integer)};
        }
        const auto* integer = in.require_integer(value, 0U);
        if (integer == nullptr) {
            take_error(in, error);
            return std::nullopt;
        }
        if (*integer == std::numeric_limits<Integer>::min()) {
            in.set_error("WFC0009", "integer overflow", 0U);
            take_error(in, error);
            return std::nullopt;
        }
        return Value{static_cast<Integer>(-*integer)};
    }

    static std::optional<Value> logical_not(Interpreter& in, const Value& value,
                                            NativeError& error) {
        if (const auto* integer = std::get_if<Integer>(&value)) {
            return Value{static_cast<Integer>(~*integer)};
        }
        if (const auto* short_integer = std::get_if<Int16>(&value)) {
            return Value{static_cast<Int16>(~*short_integer)};
        }
        if (const auto* byte = std::get_if<Byte>(&value)) {
            return Value{static_cast<Byte>(~*byte)};
        }
        if (!std::holds_alternative<bool>(value)) {
            Value widened = value;
            if (!in.widen_bitwise_operand(widened, 0U)) {
                take_error(in, error);
                return std::nullopt;
            }
            if (const auto* integer = std::get_if<Integer>(&widened)) {
                return Value{static_cast<Integer>(~*integer)};
            }
        }
        const auto operand = in.coerce_ternary_operand(value, 0U);
        if (!operand.has_value()) {
            take_error(in, error);
            return std::nullopt;
        }
        if (operand->is_null) {
            return Value{Null{}};
        }
        return Value{!operand->value};
    }

    static std::optional<std::int32_t> to_long(Interpreter& in,
                                               const Value& value,
                                               NativeError& error) {
        if (is_null(value)) {
            in.set_error("WFC0104", "Invalid use of Null", 0U);
            take_error(in, error);
            return std::nullopt;
        }
        const auto converted = in.coerce_long(value, 0U);
        if (!converted.has_value()) {
            if (in.runtime_error_number() == 0) {
                in.set_error("WFC0007", "Type mismatch", 0U);
            }
            take_error(in, error);
            return std::nullopt;
        }
        return static_cast<std::int32_t>(*converted);
    }

    static std::optional<double> to_double(Interpreter& in, const Value& value,
                                           NativeError& error) {
        if (is_null(value)) {
            in.set_error("WFC0104", "Invalid use of Null", 0U);
            take_error(in, error);
            return std::nullopt;
        }
        const auto number = number_of(value);
        if (!number.has_value()) {
            in.set_error("WFC0007", "Type mismatch", 0U);
            take_error(in, error);
        }
        return number;
    }

    static std::optional<std::string> to_text(Interpreter& in,
                                              const Value& value,
                                              NativeError& error) {
        if (is_null(value)) {
            in.set_error("WFC0104", "Invalid use of Null", 0U);
            take_error(in, error);
            return std::nullopt;
        }
        Value resolved = value;
        if (!in.resolve_default_value(resolved, 0U)) {
            take_error(in, error);
            return std::nullopt;
        }
        if (is_object_reference(resolved) ||
            std::holds_alternative<ArrayValue>(resolved)) {
            in.set_error("WFC0007", "Type mismatch", 0U);
            take_error(in, error);
            return std::nullopt;
        }
        return NativeSupport::render(resolved);
    }

    static std::optional<bool> to_boolean(Interpreter& in, const Value& value,
                                          NativeError& error) {
        if (is_null(value)) {
            return false;
        }
        const auto flag =
            in.coerce_condition_boolean(value, 0U, "WFC0007", "Type mismatch");
        if (!flag.has_value()) {
            take_error(in, error);
        }
        return flag;
    }

    static std::string type_name(Interpreter& in, const Value& value) {
        if (std::holds_alternative<Integer>(value)) {
            return "Long";
        }
        if (std::holds_alternative<Int16>(value)) {
            return "Integer";
        }
        if (std::holds_alternative<double>(value)) {
            return "Double";
        }
        if (std::holds_alternative<float>(value)) {
            return "Single";
        }
        if (std::holds_alternative<Currency>(value)) {
            return "Currency";
        }
        if (std::holds_alternative<Decimal>(value)) {
            return "Decimal";
        }
        if (std::holds_alternative<Byte>(value)) {
            return "Byte";
        }
        if (std::holds_alternative<bool>(value)) {
            return "Boolean";
        }
        if (std::holds_alternative<DateValue>(value)) {
            return "Date";
        }
        if (std::holds_alternative<std::string>(value)) {
            return "String";
        }
        if (std::holds_alternative<Empty>(value)) {
            return "Empty";
        }
        if (std::holds_alternative<Null>(value)) {
            return "Null";
        }
        if (std::holds_alternative<ErrorValue>(value)) {
            return "Error";
        }
        if (std::holds_alternative<Nothing>(value)) {
            return "Nothing";
        }
        if (const auto* instance = std::get_if<ObjectInstance>(&value)) {
            if (instance->data->native != nullptr &&
                instance->data->class_name == "wfcnative") {
                return instance->data->native->type_name();
            }
            const auto found =
                in.class_definitions_.find(instance->data->class_name);
            if (found != in.class_definitions_.end()) {
                return found->second.display_name;
            }
            return instance->data->class_name;
        }
        return "Variant()";
    }
};

NativeEngine::NativeEngine()
    : interpreter_(std::make_unique<Interpreter>(std::string_view{})) {}

NativeEngine::~NativeEngine() = default;

std::optional<Value> NativeEngine::binary(const std::string_view op,
                                          const Value& left, const Value& right,
                                          NativeError& error) {
    return NativeSupport::binary(*interpreter_, op, left, right, error);
}

std::optional<Value> NativeEngine::negate(const Value& value,
                                          NativeError& error) {
    return NativeSupport::negate(*interpreter_, value, error);
}

std::optional<Value> NativeEngine::logical_not(const Value& value,
                                               NativeError& error) {
    return NativeSupport::logical_not(*interpreter_, value, error);
}

std::optional<std::int32_t> NativeEngine::to_long(const Value& value,
                                                  NativeError& error) {
    return NativeSupport::to_long(*interpreter_, value, error);
}

std::optional<double> NativeEngine::to_double(const Value& value,
                                              NativeError& error) {
    return NativeSupport::to_double(*interpreter_, value, error);
}

std::optional<std::string> NativeEngine::to_text(const Value& value,
                                                 NativeError& error) {
    return NativeSupport::to_text(*interpreter_, value, error);
}

std::optional<bool> NativeEngine::to_boolean(const Value& value,
                                             NativeError& error) {
    return NativeSupport::to_boolean(*interpreter_, value, error);
}

std::string NativeEngine::render(const Value& value) {
    return NativeSupport::render(value);
}

std::string NativeEngine::print_text(const Value& value) {
    if (std::holds_alternative<Null>(value)) {
        return "Null";
    }
    if (is_number(value) && !std::holds_alternative<DateValue>(value)) {
        // A sign position before a number and a space after it.
        const std::string digits = NativeSupport::render(value);
        return (digits.empty() || digits.front() != '-' ? " " : "") + digits +
               " ";
    }
    return NativeSupport::render(value);
}

std::string NativeEngine::type_name(const Value& value) {
    return NativeSupport::type_name(*interpreter_, value);
}

int NativeEngine::var_type(const Value& value) {
    if (std::holds_alternative<Empty>(value)) {
        return 0;
    }
    if (std::holds_alternative<Null>(value)) {
        return 1;
    }
    if (std::holds_alternative<Int16>(value)) {
        return 2;
    }
    if (std::holds_alternative<Integer>(value)) {
        return 3;
    }
    if (std::holds_alternative<float>(value)) {
        return 4;
    }
    if (std::holds_alternative<double>(value)) {
        return 5;
    }
    if (std::holds_alternative<Currency>(value)) {
        return 6;
    }
    if (std::holds_alternative<DateValue>(value)) {
        return 7;
    }
    if (std::holds_alternative<std::string>(value)) {
        return 8;
    }
    if (std::holds_alternative<ErrorValue>(value)) {
        return 10;
    }
    if (std::holds_alternative<bool>(value)) {
        return 11;
    }
    if (std::holds_alternative<Decimal>(value)) {
        return 14;
    }
    if (std::holds_alternative<Byte>(value)) {
        return 17;
    }
    return 9;  // an object reference, Nothing included
}

}  // namespace wfc::detail
