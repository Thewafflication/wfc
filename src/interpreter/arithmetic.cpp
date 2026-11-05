// How the arithmetic and logical operators combine two operands, as Visual
// Basic 6 does. The rules were measured with the probe in
// evidence/reference/arithmetic-probe (about 17,000 operations run under
// Visual Basic 6; REQ-0315), not read from the documentation:
//
// - `+ - *` give the wider of the two types (Byte < Integer < Long, Single,
//   Double; a Currency stays Currency under `+` and `-`; a Date stays a Date
//   under `+` and `-`). Two typed operands that overflow are error 6; when an
//   operand is a Variant the result is promoted (Byte to Integer to Long to
//   Double) instead. A String beside a number is read as a Double (error 13
//   when it is not a number); two Strings under `+` are joined.
// - `/` gives a Double (a Single when both operands are whole numbers or
//   Singles and one is a Single). Dividing by zero is error 11 when both
//   operands are typed and the dividend is not zero, and error 6 otherwise.
// - `\` and `Mod` round the operands to Long (error 6 when they do not fit),
//   divide by zero is error 11, and the result is Integer when both operands
//   are Byte, Boolean or Integer (Byte when both are Byte), Long otherwise.
// - `^` gives a Double; zero to a negative power and a negative number to a
//   fraction are error 5.
// - `And Or Xor Eqv Imp` work on the bits of the operands rounded to Long and
//   give the narrower type that holds them (Boolean for two Booleans). A Null
//   operand gives Null, except where the other operand decides the result:
//   `0 And Null` is 0, `x Or Null` is x when x is not 0, and so on.
// - A Null operand makes any other result Null, but a String that is not a
//   number is error 13 even then (`+` joins Strings and so gives Null).
//
// Internal to the WFC evaluator; not part of the public API.

#include <cmath>
#include <cstdint>
#include <limits>

#include "interpreter.hpp"
#include "operand.hpp"

namespace wfc::detail {

namespace {

using namespace operands;

// The type an operand has as a number in the arithmetic.
enum class Num {
    byte,
    integer,
    long_integer,
    single,
    floating,
    currency,
    date
};

[[nodiscard]] bool is_null(const Operand& operand) {
    return operand.kind == Kind::null;
}

[[nodiscard]] bool is_text(const Operand& operand) {
    return operand.kind == Kind::string;
}

[[nodiscard]] bool is_empty(const Operand& operand) {
    return operand.kind == Kind::empty;
}

// Whether a String reads as a number (the way CDbl reads it).
[[nodiscard]] bool numeric_text(const Operand& operand) {
    return number_of_text(operand.text).has_value();
}

// Error 13 for a String that is not a number.
[[nodiscard]] double text_number(const Operand& operand) {
    const auto parsed = number_of_text(operand.text);
    if (!parsed.has_value()) {
        throw Failure{13};
    }
    return *parsed;
}

[[nodiscard]] Value make_integer(const std::int64_t value, const Num type) {
    switch (type) {
        case Num::byte:
            return Value{static_cast<Byte>(value)};
        case Num::integer:
            return Value{static_cast<Int16>(value)};
        default:
            return Value{static_cast<Integer>(value)};
    }
}

[[nodiscard]] std::int64_t low_of(const Num type) {
    return type == Num::byte      ? 0
           : type == Num::integer ? -32768
                                  : -2147483648LL;
}

[[nodiscard]] std::int64_t high_of(const Num type) {
    return type == Num::byte      ? 255
           : type == Num::integer ? 32767
                                  : 2147483647LL;
}

[[nodiscard]] Num num_of(const Operand& operand) {
    switch (operand.kind) {
        case Kind::byte:
            return Num::byte;
        case Kind::integer:
        case Kind::boolean:
        case Kind::empty:
            return Num::integer;
        case Kind::long_integer:
            return Num::long_integer;
        case Kind::single:
            return Num::single;
        case Kind::currency:
            return Num::currency;
        case Kind::date:
            return Num::date;
        default:
            return Num::floating;
    }
}

// The operands of `+ - *` once the Null, Empty and String cases are settled.
struct Pair {
    Num left{};
    Num right{};
    double left_number{};
    double right_number{};
    std::int64_t left_units{};
    std::int64_t right_units{};
    bool typed{};  // neither operand is a Variant
    bool left_date{};
};

[[nodiscard]] std::int64_t units_for(const Operand& operand, const Num type) {
    if (type == Num::currency && operand.kind == Kind::currency) {
        return operand.units;
    }
    return units_of(operand.number);
}

// The type a Variant adopts when it is Empty beside `other`.
[[nodiscard]] Num empty_as(const Num other) {
    return (other == Num::date || other == Num::integer) ? Num::integer : other;
}

[[nodiscard]] Num add_type(const char op, const Num a, const Num b) {
    if (a == Num::date || b == Num::date) {
        if (op == '*') {
            return Num::floating;
        }
        if (op == '-' && a == Num::date && b == Num::date) {
            return Num::floating;
        }
        return Num::date;
    }
    if (a == Num::currency || b == Num::currency) {
        const Num other = a == Num::currency ? b : a;
        if (op == '*' && (other == Num::single || other == Num::floating)) {
            return Num::floating;
        }
        return Num::currency;
    }
    if (a == Num::floating || b == Num::floating) {
        return Num::floating;
    }
    if (a == Num::single || b == Num::single) {
        const Num other = a == Num::single ? b : a;
        return other == Num::long_integer ? Num::floating : Num::single;
    }
    if (a == Num::long_integer || b == Num::long_integer) {
        return Num::long_integer;
    }
    if (a == Num::byte && b == Num::byte) {
        return Num::byte;
    }
    return Num::integer;
}

[[nodiscard]] Value float_result(const double value) {
    if (!std::isfinite(value)) {
        throw Failure{6};
    }
    return Value{value};
}

[[nodiscard]] Value add_values(const char op, Operand a, Operand b) {
    // Null: the other operand still has to be a number or a String that is
    // one (a String joins under `+`, so it never fails there).
    if (is_null(a) || is_null(b)) {
        const Operand& other = is_null(a) ? b : a;
        if (is_text(other) && op != '+' && !numeric_text(other)) {
            throw Failure{13};
        }
        return Value{Null{}};
    }
    if (is_text(a) && is_text(b)) {
        if (op == '+') {
            return Value{a.text + b.text};
        }
    } else if ((is_text(a) && is_empty(b)) || (is_empty(a) && is_text(b))) {
        const Operand& text = is_text(a) ? a : b;
        if (op == '+') {
            return Value{text.text};
        }
    }
    // A String is read as a Double.
    for (Operand* operand : {&a, &b}) {
        if (is_text(*operand)) {
            operand->number = text_number(*operand);
            operand->kind = Kind::floating;
        }
    }
    const bool typed = a.typed && b.typed;
    Num ta = num_of(a);
    Num tb = num_of(b);
    if (is_empty(a) && is_empty(b)) {
        ta = tb = Num::integer;
    } else if (is_empty(a)) {
        ta = empty_as(tb);
    } else if (is_empty(b)) {
        tb = empty_as(ta);
    }
    const Num type = add_type(op, ta, tb);
    const double x = a.number;
    const double y = b.number;
    switch (type) {
        case Num::byte:
        case Num::integer:
        case Num::long_integer: {
            const auto l = static_cast<std::int64_t>(x);
            const auto r = static_cast<std::int64_t>(y);
            const std::int64_t z =
                op == '+' ? l + r : (op == '-' ? l - r : l * r);
            Num result = type;
            while (z < low_of(result) || z > high_of(result)) {
                if (typed) {
                    throw Failure{6};
                }
                if (result == Num::long_integer) {
                    return Value{static_cast<double>(z)};
                }
                result = result == Num::byte ? Num::integer : Num::long_integer;
            }
            return make_integer(z, result);
        }
        case Num::single: {
            const float l = static_cast<float>(x);
            const float r = static_cast<float>(y);
            const float z = op == '+' ? l + r : (op == '-' ? l - r : l * r);
            if (!std::isfinite(z)) {
                throw Failure{6};
            }
            return Value{z};
        }
        case Num::currency: {
            const std::int64_t l = units_for(a, Num::currency);
            const std::int64_t r = units_for(b, Num::currency);
            std::optional<std::int64_t> z;
            if (op == '*') {
                z = currency_multiply(l, r);
            } else {
                const bool overflow =
                    op == '+'
                        ? ((r > 0 &&
                            l > std::numeric_limits<std::int64_t>::max() - r) ||
                           (r < 0 &&
                            l < std::numeric_limits<std::int64_t>::min() - r))
                        : ((r < 0 &&
                            l > std::numeric_limits<std::int64_t>::max() + r) ||
                           (r > 0 &&
                            l < std::numeric_limits<std::int64_t>::min() + r));
                if (!overflow) {
                    z = op == '+' ? l + r : l - r;
                }
            }
            if (!z.has_value()) {
                throw Failure{6};
            }
            return Value{Currency{*z}};
        }
        case Num::date: {
            const double z = op == '+' ? x + y : x - y;
            if (!(z >= -657434.0 && z <= 2958465.999999)) {
                if (!typed) {
                    return Value{z};
                }
                throw Failure{ta == Num::date ? 6 : 5};
            }
            return Value{DateValue{z}};
        }
        default:
            return float_result(op == '+' ? x + y
                                          : (op == '-' ? x - y : x * y));
    }
}

// The Null case of `/ \ Mod ^` and the other operators that read their
// operands as numbers: a String that is not a number is still error 13.
[[nodiscard]] bool null_result(const Operand& a, const Operand& b) {
    if (!is_null(a) && !is_null(b)) {
        return false;
    }
    const Operand& other = is_null(a) ? b : a;
    if (is_text(other) && !numeric_text(other)) {
        throw Failure{13};
    }
    return true;
}

[[nodiscard]] double number_in(const Operand& operand) {
    return is_text(operand) ? text_number(operand) : operand.number;
}

[[nodiscard]] Value divide_values(const Operand& a, const Operand& b) {
    if (null_result(a, b)) {
        return Value{Null{}};
    }
    const double x = number_in(a);
    const double y = number_in(b);
    if (y == 0.0) {
        throw Failure{x != 0.0 && a.typed && b.typed ? 11 : 6};
    }
    const auto small = [](const Operand& operand) {
        switch (operand.kind) {
            case Kind::single:
            case Kind::integer:
            case Kind::byte:
            case Kind::boolean:
            case Kind::empty:
                return true;
            default:
                return false;
        }
    };
    if (small(a) && small(b) &&
        (a.kind == Kind::single || b.kind == Kind::single)) {
        const float z = static_cast<float>(x) / static_cast<float>(y);
        if (!std::isfinite(z)) {
            throw Failure{6};
        }
        return Value{z};
    }
    return float_result(x / y);
}

// An operand read as a whole number: the type it counts as, and its value.
// "True" is a Boolean when the other operand is a Variant.
struct Whole {
    Num type{Num::floating};
    bool boolean{};
    std::int64_t value{};
};

[[nodiscard]] std::int64_t rounded_long(const double value) {
    const double rounded = std::nearbyint(value);
    if (!(rounded >= -2147483648.0 && rounded <= 2147483647.0)) {
        throw Failure{6};
    }
    return static_cast<std::int64_t>(rounded);
}

[[nodiscard]] Whole whole_of(const Operand& operand, const Operand& other) {
    Whole whole;
    switch (operand.kind) {
        case Kind::empty:
            whole.type = Num::integer;
            return whole;
        case Kind::string: {
            const auto parsed = number_of_text(operand.text);
            if (parsed.has_value()) {
                whole.value = rounded_long(*parsed);
                return whole;
            }
            if (operand.text == "True" && !other.typed) {
                whole.type = Num::integer;
                whole.boolean = true;
                whole.value = -1;
                return whole;
            }
            throw Failure{13};
        }
        case Kind::boolean:
            whole.type = Num::integer;
            whole.boolean = true;
            whole.value = operand.number != 0.0 ? -1 : 0;
            return whole;
        case Kind::byte:
            whole.type = Num::byte;
            whole.value = static_cast<std::int64_t>(operand.number);
            return whole;
        case Kind::integer:
            whole.type = Num::integer;
            whole.value = static_cast<std::int64_t>(operand.number);
            return whole;
        case Kind::long_integer:
            whole.type = Num::long_integer;
            whole.value = static_cast<std::int64_t>(operand.number);
            return whole;
        default:
            whole.value = rounded_long(operand.number);
            return whole;
    }
}

[[nodiscard]] Value whole_division(const char op, const Operand& a,
                                   const Operand& b) {
    if (is_null(a) || is_null(b)) {
        const Operand& other = is_null(a) ? b : a;
        if (is_text(other) && !numeric_text(other) && other.text != "True") {
            throw Failure{13};
        }
        return Value{Null{}};
    }
    // A typed Boolean beside a typed String is a Boolean, whatever the String
    // says, when the String is a number or "True".
    if (a.kind == Kind::boolean && a.typed && is_text(b) && b.typed &&
        (numeric_text(b) || b.text == "True")) {
        // Measured: `\` gives the left Boolean again and Mod gives False.
        return Value{op == '\\' && a.number != 0.0};
    }
    const Whole x = whole_of(a, b);
    const Whole y = whole_of(b, a);
    if (y.value == 0) {
        throw Failure{11};
    }
    if (x.value == -2147483648LL && y.value == -1) {
        throw Failure{6};
    }
    const std::int64_t z = op == '\\' ? x.value / y.value : x.value % y.value;
    const auto whole_type = [](const Whole& w) {
        return w.boolean || w.type == Num::integer || w.type == Num::byte;
    };
    Num type = Num::long_integer;
    if (whole_type(x) && whole_type(y)) {
        type = x.type == Num::byte && y.type == Num::byte && !x.boolean &&
                       !y.boolean
                   ? Num::byte
                   : Num::integer;
    }
    if (z < low_of(type) || z > high_of(type)) {
        throw Failure{6};
    }
    return make_integer(z, type);
}

[[nodiscard]] Value power_values(const Operand& a, const Operand& b) {
    if (null_result(a, b)) {
        return Value{Null{}};
    }
    const double x = number_in(a);
    const double y = number_in(b);
    if ((x == 0.0 && y < 0.0) || (x < 0.0 && y != std::trunc(y))) {
        throw Failure{5};
    }
    return float_result(std::pow(x, y));
}

// A value of the type of an operand.
[[nodiscard]] Value of_type(const Whole& w, const std::int64_t value) {
    if (w.boolean) {
        return Value{value != 0};
    }
    switch (w.type) {
        case Num::byte:
            return Value{static_cast<Byte>(value & 0xFF)};
        case Num::integer:
            return Value{static_cast<Int16>(value)};
        default:
            return Value{static_cast<Integer>(value)};
    }
}

[[nodiscard]] Value logical_values(const char op, const Operand& a,
                                   const Operand& b) {
    Whole x;
    Whole y;
    if (!is_null(a)) {
        x = whole_of(a, b);
    }
    if (!is_null(b)) {
        y = whole_of(b, a);
    }
    if (is_null(a) && is_null(b)) {
        return Value{Null{}};
    }
    if (is_null(a) || is_null(b)) {
        // The other operand decides the result only when it has to.
        const bool null_left = is_null(a);
        const Whole& w = null_left ? y : x;
        const std::int64_t v = w.value;
        switch (op) {
            case 'A':
                return v == 0 ? of_type(w, 0) : Value{Null{}};
            case 'O':
                return v != 0 ? of_type(w, v) : Value{Null{}};
            case 'I': {
                if (null_left) {
                    return v != 0 ? of_type(w, v) : Value{Null{}};
                }
                std::int64_t n = ~v;
                if (w.boolean) {
                    n = v != 0 ? 0 : -1;
                } else if (w.type == Num::byte) {
                    n &= 0xFF;
                }
                return n != 0 ? of_type(w, n) : Value{Null{}};
            }
            default:
                return Value{Null{}};
        }
    }
    std::int64_t r{};
    switch (op) {
        case 'A':
            r = x.value & y.value;
            break;
        case 'O':
            r = x.value | y.value;
            break;
        case 'X':
            r = x.value ^ y.value;
            break;
        case 'E':
            r = ~(x.value ^ y.value);
            break;
        default:
            r = ~x.value | y.value;
            break;
    }
    const auto small = [](const Whole& w) {
        return w.boolean || w.type == Num::byte || w.type == Num::integer;
    };
    if (x.boolean && y.boolean) {
        return Value{r != 0};
    }
    if (x.type == Num::byte && y.type == Num::byte && !x.boolean &&
        !y.boolean) {
        return Value{static_cast<Byte>(r & 0xFF)};
    }
    if (small(x) && small(y)) {
        return Value{static_cast<Int16>(r)};
    }
    return Value{static_cast<Integer>(r)};
}

}  // namespace

std::optional<Value> Interpreter::arithmetic(
    const Value& left_in, const bool left_variant, const Value& right_in,
    const bool right_variant, const char operation,
    const std::size_t operator_offset) {
    Value left = left_in;
    Value right = right_in;
    if (execute_ && (std::holds_alternative<ObjectInstance>(left) ||
                     std::holds_alternative<ObjectInstance>(right))) {
        if (!resolve_default_value(left, operator_offset) ||
            !resolve_default_value(right, operator_offset)) {
            return std::nullopt;
        }
    }
    // A Decimal, an array or an object has no place in the measured rules.
    const auto exotic = [](const Value& v) {
        return std::holds_alternative<Decimal>(v) ||
               operand_of(v, true).kind == Kind::other;
    };
    if (exotic(left) || exotic(right)) {
        switch (operation) {
            case '+':
            case '-':
            case '*':
            case '/':
                return numeric_binary(left, right, operation, operator_offset);
            case '\\':
            case '%':
                return integer_binary(left, right, operation, operator_offset);
            case '^':
                break;
            default:
                return logical_binary(left, right, operation, operator_offset);
        }
    }
    try {
        const Operand a = operand_of(left, left_variant);
        const Operand b = operand_of(right, right_variant);
        switch (operation) {
            case '+':
            case '-':
            case '*':
                return add_values(operation, a, b);
            case '/':
                return divide_values(a, b);
            case '\\':
            case '%':
                return whole_division(operation, a, b);
            case '^':
                return power_values(a, b);
            default:
                return logical_values(operation, a, b);
        }
    } catch (const Failure& failure) {
        if (!execute_) {
            return Value{Integer{}};  // code that is skipped runs nothing
        }
        static_cast<void>(raise_runtime(
            failure.number,
            vb_error_description(static_cast<Integer>(failure.number)),
            operator_offset));
        return std::nullopt;
    }
}

std::optional<Value> Interpreter::unary_operation(const Value& value_in,
                                                  const bool variant,
                                                  const bool negate,
                                                  const std::size_t offset) {
    Value value = value_in;
    if (execute_ && std::holds_alternative<ObjectInstance>(value) &&
        !resolve_default_value(value, offset)) {
        return std::nullopt;
    }
    if (const auto* decimal = std::get_if<Decimal>(&value);
        decimal != nullptr && negate) {
        Decimal negated = *decimal;
        if (!is_zero_big(negated.mantissa)) {
            negated.negative = !negated.negative;
        }
        return Value{negated};
    }
    if (!std::holds_alternative<Decimal>(value) &&
        operand_of(value, true).kind == Kind::other) {
        if (!execute_) {
            return Value{Integer{}};
        }
        static_cast<void>(raise_runtime(13, vb_error_description(13), offset));
        return std::nullopt;
    }
    try {
        const Operand a = operand_of(value, variant);
        if (is_null(a)) {
            return Value{Null{}};
        }
        if (negate) {
            switch (a.kind) {
                case Kind::empty:
                    return Value{Int16{}};
                case Kind::byte:
                case Kind::boolean:
                    return Value{static_cast<Int16>(-a.number)};
                case Kind::integer:
                    if (a.number == -32768.0) {
                        if (a.typed) {
                            throw Failure{6};
                        }
                        return Value{Integer{32768}};
                    }
                    return Value{static_cast<Int16>(-a.number)};
                case Kind::long_integer:
                    if (a.number == -2147483648.0) {
                        if (a.typed) {
                            throw Failure{6};
                        }
                        return Value{2147483648.0};
                    }
                    return Value{static_cast<Integer>(-a.number)};
                case Kind::single:
                    return Value{-static_cast<float>(a.number)};
                case Kind::currency:
                    if (a.units == std::numeric_limits<std::int64_t>::min()) {
                        throw Failure{6};
                    }
                    return Value{Currency{-a.units}};
                case Kind::date:
                    return Value{DateValue{-a.number}};
                case Kind::string:
                    return Value{-text_number(a)};
                default:
                    return Value{-a.number};
            }
        }
        Operand typed_other;
        typed_other.typed = true;
        const Whole w = whole_of(a, typed_other);
        if (a.kind == Kind::boolean) {
            return Value{w.value == 0};
        }
        if (a.kind == Kind::byte) {
            return Value{static_cast<Byte>(~w.value & 0xFF)};
        }
        if (a.kind == Kind::integer || a.kind == Kind::empty) {
            return Value{static_cast<Int16>(~w.value)};
        }
        return Value{static_cast<Integer>(~w.value)};
    } catch (const Failure& failure) {
        if (!execute_) {
            return Value{Integer{}};
        }
        static_cast<void>(raise_runtime(
            failure.number,
            vb_error_description(static_cast<Integer>(failure.number)),
            offset));
        return std::nullopt;
    }
}

}  // namespace wfc::detail
