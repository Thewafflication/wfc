// The operands of the arithmetic, logical and comparison operators, seen the
// way Visual Basic 6 sees them: a kind (the type the value has), whether it is
// typed or held by a Variant, and its value as a number or text.
//
// Internal to the WFC evaluator; not part of the public API.

#pragma once

#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

#include "interpreter.hpp"

namespace wfc::detail::operands {

enum class Kind {
    integer,
    long_integer,
    single,
    floating,
    currency,
    byte,
    boolean,
    date,
    string,
    empty,
    null,
    other,
};

// An error of Visual Basic found while converting an operand.
struct Failure {
    int number{};
};

struct Operand {
    Kind kind{Kind::other};
    bool typed{};          // not a Variant
    double number{};       // the value, for the numeric kinds
    std::int64_t units{};  // Currency: the value times 10000
    std::string text;      // String
    std::string rendered;  // CStr of a number, Boolean or Date
};

[[nodiscard]] inline Operand operand_of(const Value& value,
                                        const bool variant) {
    Operand operand;
    operand.typed = !variant;
    if (const auto* i16 = std::get_if<Int16>(&value)) {
        operand.kind = Kind::integer;
        operand.number = *i16;
    } else if (const auto* i32 = std::get_if<Integer>(&value)) {
        operand.kind = Kind::long_integer;
        operand.number = *i32;
    } else if (const auto* u8 = std::get_if<Byte>(&value)) {
        operand.kind = Kind::byte;
        operand.number = *u8;
    } else if (const auto* f32p = std::get_if<float>(&value)) {
        operand.kind = Kind::single;
        operand.number = static_cast<double>(*f32p);
    } else if (const auto* f64p = std::get_if<double>(&value)) {
        operand.kind = Kind::floating;
        operand.number = *f64p;
    } else if (const auto* dec = std::get_if<Decimal>(&value)) {
        operand.kind = Kind::floating;
        operand.number = decimal_to_double(*dec);
    } else if (const auto* cur = std::get_if<Currency>(&value)) {
        operand.kind = Kind::currency;
        operand.units = cur->scaled;
        operand.number = static_cast<double>(cur->scaled) / 10000.0;
    } else if (const auto* flag = std::get_if<bool>(&value)) {
        operand.kind = Kind::boolean;
        operand.number = *flag ? -1.0 : 0.0;
    } else if (const auto* date = std::get_if<DateValue>(&value)) {
        operand.kind = Kind::date;
        operand.number = date->serial;
    } else if (const auto* str = std::get_if<std::string>(&value)) {
        operand.kind = Kind::string;
        operand.text = *str;
    } else if (std::holds_alternative<Empty>(value)) {
        operand.kind = Kind::empty;
        operand.typed = false;
    } else if (std::holds_alternative<Null>(value)) {
        operand.kind = Kind::null;
        operand.typed = false;
    }
    return operand;
}

[[nodiscard]] inline bool is_number_kind(const Kind kind) {
    switch (kind) {
        case Kind::integer:
        case Kind::long_integer:
        case Kind::single:
        case Kind::floating:
        case Kind::currency:
        case Kind::byte:
            return true;
        default:
            return false;
    }
}

[[nodiscard]] inline int order(const double left, const double right) {
    return left < right ? -1 : (left > right ? 1 : 0);
}

[[nodiscard]] inline double float32(const double value) {
    return static_cast<double>(static_cast<float>(value));
}

[[nodiscard]] inline std::int64_t units_of(const double value) {
    return static_cast<std::int64_t>(std::nearbyint(value * 10000.0));
}

// The Currency of an operand, in units of 1/10000.
[[nodiscard]] inline std::int64_t currency_units(const Operand& operand) {
    return operand.kind == Kind::currency ? operand.units
                                          : units_of(operand.number);
}

// A String as a number, as CDbl reads it; nullopt when it is not one.
[[nodiscard]] inline std::optional<double> number_of_text(
    const std::string& text) {
    const auto parsed = parse_numeric_string(text);
    if (parsed.status == NumericStringStatus::valid) {
        return parsed.value;
    }
    return std::nullopt;
}

}  // namespace wfc::detail::operands
