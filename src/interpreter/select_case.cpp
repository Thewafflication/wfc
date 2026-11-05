// How Select Case compares its selector with a Case value, as Visual Basic 6
// does. The rules were measured with the probe in
// evidence/reference/select-case-probe (about 6,700 comparisons run under
// Visual Basic 6) and are not the rules of the `=`, `<` and `>` operators:
//
// - A typed (non-Variant) selector converts the Case value to its own type
//   (as CInt, CLng, CByte, CSng, CDbl, CCur, CBool, CDate or CStr would) and
//   compares in that type: a String that is not a number is error 13, Null is
//   error 94, a number that does not fit is error 6. A Single or Double
//   selector compares with a Currency value as the floating-point type
//   sees it (the Currency is divided by 10000 in extended precision).
// - A Variant selector compares by the rules of Variants: Null never matches
//   (no error), Empty is 0 beside a number and "" beside a String, a String
//   Variant beside a typed number, Boolean or Date is converted to that type
//   when it can be and is the greater one when it cannot, and two Variants
//   order every number before every String.
//
// Internal to the WFC evaluator; not part of the public API.

#include "interpreter.hpp"

#include <cmath>
#include <cstdint>
#include <limits>

namespace wfc::detail {

namespace {

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

[[nodiscard]] Operand operand_of(const Value& value, const bool variant) {
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

[[nodiscard]] bool is_number_kind(const Kind kind) {
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

[[nodiscard]] int order(const double left, const double right) {
    return left < right ? -1 : (left > right ? 1 : 0);
}

[[nodiscard]] double float32(const double value) {
    return static_cast<double>(static_cast<float>(value));
}

[[nodiscard]] std::int64_t units_of(const double value) {
    return static_cast<std::int64_t>(std::nearbyint(value * 10000.0));
}

// The Currency of an operand, in units of 1/10000.
[[nodiscard]] std::int64_t currency_units(const Operand& operand) {
    return operand.kind == Kind::currency ? operand.units
                                          : units_of(operand.number);
}

// A String as a number, as CDbl reads it; nullopt when it is not one.
[[nodiscard]] std::optional<double> number_of_text(const std::string& text) {
    const auto parsed = parse_numeric_string(text);
    if (parsed.status == NumericStringStatus::valid) {
        return parsed.value;
    }
    return std::nullopt;
}

// The operand as a number of the type `target` (CInt, CLng, CByte, CSng, CDbl).
[[nodiscard]] double to_number(const Operand& operand, const Kind target) {
    double n = 0.0;
    switch (operand.kind) {
        case Kind::null:
            throw Failure{94};
        case Kind::empty:
            n = 0.0;
            break;
        case Kind::string: {
            const auto parsed = number_of_text(operand.text);
            if (!parsed.has_value()) {
                throw Failure{13};
            }
            n = *parsed;
            break;
        }
        case Kind::other:
            throw Failure{13};
        default:
            n = operand.number;
            break;
    }
    if (operand.kind == Kind::boolean && target == Kind::byte) {
        n = n != 0.0 ? 255.0 : 0.0;  // CByte(True) is 255
    }
    if (target == Kind::integer || target == Kind::long_integer ||
        target == Kind::byte) {
        const double rounded = std::nearbyint(n);
        const double low = target == Kind::integer        ? -32768.0
                           : target == Kind::long_integer ? -2147483648.0
                                                          : 0.0;
        const double high = target == Kind::integer        ? 32767.0
                            : target == Kind::long_integer ? 2147483647.0
                                                           : 255.0;
        if (!(rounded >= low && rounded <= high)) {
            throw Failure{6};
        }
        return rounded;
    }
    if (target == Kind::single) {
        return float32(n);
    }
    return n;
}

[[nodiscard]] double to_boolean(const Operand& operand) {
    switch (operand.kind) {
        case Kind::null:
            throw Failure{94};
        case Kind::empty:
            return 0.0;
        case Kind::string: {
            std::string text;
            for (const char c : operand.text) {
                text.push_back(ascii_lower(c));
            }
            while (!text.empty() && text.front() == ' ') {
                text.erase(0, 1);
            }
            while (!text.empty() && text.back() == ' ') {
                text.pop_back();
            }
            if (text == "true") {
                return -1.0;
            }
            if (text == "false") {
                return 0.0;
            }
            const auto parsed = number_of_text(operand.text);
            if (!parsed.has_value()) {
                throw Failure{13};
            }
            return *parsed != 0.0 ? -1.0 : 0.0;
        }
        case Kind::other:
            throw Failure{13};
        default:
            return operand.number != 0.0 ? -1.0 : 0.0;
    }
}

[[nodiscard]] double to_date(const Operand& operand) {
    switch (operand.kind) {
        case Kind::null:
            throw Failure{94};
        case Kind::empty:
            return 0.0;
        case Kind::string: {
            if (const auto parsed = parse_date_text(operand.text)) {
                return *parsed;
            }
            if (const auto parsed = number_of_text(operand.text)) {
                return *parsed;
            }
            throw Failure{13};
        }
        case Kind::other:
            throw Failure{13};
        default:
            return operand.number;
    }
}

[[nodiscard]] std::string to_text(const Operand& operand) {
    switch (operand.kind) {
        case Kind::null:
            throw Failure{94};
        case Kind::empty:
            return {};
        case Kind::string:
            return operand.text;
        case Kind::other:
            throw Failure{13};
        default:
            return operand.rendered;
    }
}

// The order of `d` and the Currency `units`/10000 as Visual Basic 6 gets it:
// the Currency is turned into a floating-point number in the 80-bit extended
// type (a 64-bit mantissa), not in a double, so 0.1 as a Currency is a hair
// below 0.1 as a Double.
[[nodiscard]] int order_float_currency(const double d,
                                       const std::int64_t units) {
    if (units < 0) {
        return -order_float_currency(-d, -units);
    }
    const double wanted = static_cast<double>(units);
    const double h = wanted / 10000.0;  // the nearest double
    if (d != h) {
        return d < h ? -1 : 1;
    }
    // d is the nearest double to the Currency: find on which side of it the
    // extended value is.
    const double remainder = std::fma(-h, 10000.0, wanted);  // exact
    if (remainder == 0.0) {
        return 0;
    }
    const double delta = remainder / 10000.0;
    if (h == 0.0 || std::fabs(delta) < std::ldexp(1.0, std::ilogb(h) - 64)) {
        return 0;
    }
    return delta > 0.0 ? -1 : 1;
}

// A number against a number of a Variant (or a typed one against a Variant).
[[nodiscard]] int order_variant_numbers(const Operand& s, const Operand& v) {
    if (v.typed && v.kind == Kind::byte && s.kind == Kind::boolean) {
        return order(s.number != 0.0 ? 255.0 : 0.0, v.number);
    }
    if (s.kind == Kind::currency || v.kind == Kind::currency) {
        const auto left = currency_units(s);
        const auto right = currency_units(v);
        return left < right ? -1 : (left > right ? 1 : 0);
    }
    const auto floating = [](const Kind k) {
        return k == Kind::single || k == Kind::floating;
    };
    if ((s.kind == Kind::single || v.kind == Kind::single) &&
        floating(s.kind) && floating(v.kind)) {
        return order(float32(s.number), float32(v.number));
    }
    return order(s.number, v.number);
}

// A typed number selector against any Case value.
[[nodiscard]] int order_typed_number(const Operand& s, const Operand& v) {
    if ((s.kind == Kind::single || s.kind == Kind::floating) &&
        v.kind == Kind::currency) {
        return order_float_currency(s.number, v.units);
    }
    if (s.kind == Kind::single && v.kind == Kind::floating) {
        return v.typed ? order(s.number, float32(v.number))
                       : order(s.number, v.number);
    }
    if ((s.kind == Kind::single || s.kind == Kind::floating) &&
        v.kind == Kind::single) {
        return order(s.number, v.number);
    }
    if (s.kind == Kind::currency) {
        const auto wanted = units_of(to_number(v, Kind::floating));
        return s.units < wanted ? -1 : (s.units > wanted ? 1 : 0);
    }
    return order(s.number, to_number(v, s.kind));
}

}  // namespace

std::optional<int> Interpreter::select_case_order(const Value& selector,
                                                  const bool selector_variant,
                                                  const Value& value,
                                                  const bool value_variant,
                                                  int& error) {
    error = 0;
    Operand s = operand_of(selector, selector_variant);
    Operand v = operand_of(value, value_variant);
    if (s.kind != Kind::string && s.kind != Kind::other) {
        s.rendered = render(selector);
    }
    if (v.kind != Kind::string && v.kind != Kind::other) {
        v.rendered = render(value);
    }
    const auto text_order = [&](const std::string& left,
                                const std::string& right) {
        const int result = compare_strings(left, right);
        return result < 0 ? -1 : (result > 0 ? 1 : 0);
    };
    try {
        if (s.kind == Kind::other || v.kind == Kind::other) {
            throw Failure{13};
        }
        if (s.typed && s.kind != Kind::empty && s.kind != Kind::null) {
            // A typed selector: the Case value is converted to its type.
            if (is_number_kind(s.kind)) {
                return order_typed_number(s, v);
            }
            if (s.kind == Kind::boolean) {
                return order(s.number, to_boolean(v));
            }
            if (s.kind == Kind::date) {
                return order(s.number, to_date(v));
            }
            return text_order(s.text, to_text(v));
        }
        // A Variant selector.
        if (s.kind == Kind::null || v.kind == Kind::null) {
            return std::nullopt;
        }
        if (v.typed) {
            if (s.kind == Kind::empty) {
                if (v.kind == Kind::string) {
                    return text_order({}, v.text);
                }
                return order(0.0, v.number);
            }
            if (v.kind == Kind::string) {
                return text_order(to_text(s), v.text);
            }
            if (s.kind == Kind::string) {
                // A Variant String beside a typed number, Boolean or Date:
                // converted to that type, or the greater one.
                try {
                    if (is_number_kind(v.kind)) {
                        const auto parsed = number_of_text(s.text);
                        if (!parsed.has_value()) {
                            return 1;
                        }
                        return order(*parsed, v.number);
                    }
                    if (v.kind == Kind::boolean) {
                        return order(to_boolean(s), v.number);
                    }
                    if (v.kind == Kind::date) {
                        return order(to_date(s), v.number);
                    }
                } catch (const Failure&) {
                    return 1;
                }
            }
            return order_variant_numbers(s, v);
        }
        // A Variant against a Variant.
        if (s.kind == Kind::empty && v.kind == Kind::empty) {
            return 0;
        }
        if (s.kind == Kind::empty) {
            return v.kind == Kind::string ? text_order({}, v.text)
                                          : order(0.0, v.number);
        }
        if (v.kind == Kind::empty) {
            return s.kind == Kind::string ? text_order(s.text, {})
                                          : order(s.number, 0.0);
        }
        if (s.kind == Kind::string && v.kind == Kind::string) {
            return text_order(s.text, v.text);
        }
        if (s.kind == Kind::string) {
            return 1;  // every String is above every number
        }
        if (v.kind == Kind::string) {
            return -1;
        }
        return order_variant_numbers(s, v);
    } catch (const Failure& failure) {
        error = failure.number;
        return std::nullopt;
    }
}

}  // namespace wfc::detail
