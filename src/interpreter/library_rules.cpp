// Interpreter: rules of the VBA library measured under Visual Basic 6 with
// evidence/reference/library-probe (REQ-0316): what a function answers for an
// operand of each kind (Null, Empty, Boolean, Date, a String that is a number).
// Internal to the WFC evaluator; not part of the public API.

#include <cmath>

#include "interpreter.hpp"

namespace wfc::detail {

namespace {

// The numbers of the VBA functions that take a Variant that holds Null as an
// error (94) and not as a Null.
bool null_is_invalid(const std::string_view name) {
    static const std::set<std::string, std::less<>> names = {
        "chr",  "chr$", "chrw", "chrw$", "space", "space$",
        "hex$", "oct$", "sgn",  "val",   "str$",  "choose"};
    return names.contains(std::string(name));
}

// The size in bytes that Len gives for a variable of the type of `value`.
std::optional<Int16> size_of_type(const Value& value) {
    if (std::holds_alternative<Int16>(value) ||
        std::holds_alternative<bool>(value)) {
        return static_cast<Int16>(2);
    }
    if (std::holds_alternative<Integer>(value) ||
        std::holds_alternative<float>(value)) {
        return static_cast<Int16>(4);
    }
    if (std::holds_alternative<double>(value) ||
        std::holds_alternative<Currency>(value) ||
        std::holds_alternative<DateValue>(value)) {
        return static_cast<Int16>(8);
    }
    if (std::holds_alternative<Byte>(value)) {
        return static_cast<Int16>(1);
    }
    return std::nullopt;
}

}  // namespace

bool Interpreter::measured_library_call(const std::string_view identifier,
                                        std::vector<Value>& arguments,
                                        const std::vector<bool>& variant,
                                        const std::size_t offset,
                                        std::optional<Value>& result) {
    if (!execute_ || arguments.empty()) {
        return false;
    }
    const std::string key(identifier);
    const auto is_null = [&](const std::size_t index) {
        return index < arguments.size() &&
               std::holds_alternative<Null>(arguments[index]);
    };
    const auto invalid_use_of_null = [&]() {
        set_error("WFC0104", "Invalid use of Null", offset);
        result = std::nullopt;
        return true;
    };
    const auto typed = [&](const std::size_t index) {
        return index < variant.size() && !variant[index];
    };

    // Null.
    if ((null_is_invalid(key) && is_null(0)) ||
        (key == "dateserial" && (is_null(0) || is_null(1) || is_null(2))) ||
        (key == "replace" && is_null(0))) {
        return invalid_use_of_null();
    }
    if (key == "join") {
        if (const auto* list = std::get_if<ArrayValue>(&arguments[0])) {
            for (const auto& element : list->elements) {
                if (std::holds_alternative<Null>(element)) {
                    return invalid_use_of_null();
                }
            }
        }
    }
    if ((key == "str" || key == "round") && is_null(0)) {
        result = Value{Null{}};
        return true;
    }
    if (key == "string" && is_null(1)) {
        result = Value{Null{}};
        return true;
    }
    if (key == "instr") {
        const std::size_t first = arguments.size() >= 3U ? 1U : 0U;
        if (is_null(first) || is_null(first + 1U)) {
            result = Value{Null{}};
            return true;
        }
    }
    const bool number_format = key == "formatnumber" ||
                               key == "formatcurrency" ||
                               key == "formatpercent";
    if (number_format &&
        (is_null(0) || std::holds_alternative<Empty>(arguments[0]))) {
        result = Value{std::string{}};
        return true;
    }
    if ((key == "format" || key == "format$") && arguments.size() >= 2U &&
        is_null(0)) {
        // Measured with the picture "0.00"; a picture of digits and signs
        // only, as that one is.
        const auto* picture = std::get_if<std::string>(&arguments[1]);
        if (picture != nullptr &&
            picture->find_first_not_of("0#.,%;+-() $") == std::string::npos) {
            result = Value{std::string{}};
            return true;
        }
    }

    // A Date or a String that is a number, where a number is wanted.
    const auto as_serial = [&](const std::size_t index) {
        if (const auto* date = std::get_if<DateValue>(&arguments[index])) {
            arguments[index] = Value{date->serial};
        } else if (const auto* text =
                       std::get_if<std::string>(&arguments[index])) {
            const auto number = parse_numeric_string(*text);
            if (number.status == NumericStringStatus::valid) {
                arguments[index] = Value{number.value};
            }
        }
    };
    if (number_format) {
        as_serial(0);
    }
    if ((key == "format" || key == "format$") && arguments.size() >= 2U) {
        if (const auto* pattern = std::get_if<std::string>(&arguments[1])) {
            const bool text_format =
                pattern->find_first_of("@&<>") != std::string::npos;
            const bool date_format =
                pattern->find_first_of("dDmMyYhHnNsStTaApP") !=
                std::string::npos;
            if (!text_format && !date_format) {
                as_serial(0);
            }
        }
    }
    if (key == "sqr" || key == "sin" || key == "cos" || key == "tan" ||
        key == "atn" || key == "exp" || key == "log") {
        if (std::holds_alternative<DateValue>(arguments[0])) {
            as_serial(0);
        }
    }
    if (key == "hex" || key == "oct" || key == "hex$" || key == "oct$") {
        if (const auto* date = std::get_if<DateValue>(&arguments[0])) {
            arguments[0] =
                Value{static_cast<Integer>(std::nearbyint(date->serial))};
        }
    }
    if (key == "abs" || key == "int" || key == "fix" || key == "round") {
        if (std::holds_alternative<Byte>(arguments[0])) {
            result = arguments[0];
            return true;
        }
        if (const auto* date = std::get_if<DateValue>(&arguments[0])) {
            double serial = date->serial;
            if (key == "abs") {
                serial = std::fabs(serial);
            } else if (key == "int") {
                serial = std::floor(serial);
            } else if (key == "fix") {
                serial = std::trunc(serial);
            } else {
                const double scale =
                    arguments.size() >= 2U
                        ? std::pow(10.0, as_double(arguments[1]))
                        : 1.0;
                serial = std::nearbyint(serial * scale) / scale;
            }
            result = Value{DateValue{serial}};
            return true;
        }
    }
    if (key == "choose") {
        if (const auto* date = std::get_if<DateValue>(&arguments[0])) {
            arguments[0] =
                Value{static_cast<Integer>(std::nearbyint(date->serial))};
        }
    }
    if (key == "chr" || key == "chr$" || key == "chrw" || key == "chrw$" ||
        key == "space" || key == "space$") {
        if (const auto* date = std::get_if<DateValue>(&arguments[0])) {
            arguments[0] =
                Value{static_cast<Integer>(std::nearbyint(date->serial))};
        } else if (const auto* flag = std::get_if<bool>(&arguments[0])) {
            arguments[0] = Value{static_cast<Integer>(*flag ? -1 : 0)};
        } else if (std::holds_alternative<Empty>(arguments[0])) {
            arguments[0] = Value{Integer{0}};
        } else if (const auto* small = std::get_if<Int16>(&arguments[0])) {
            arguments[0] = Value{static_cast<Integer>(*small)};
        }
        if (key[0] == 'c' && key.find('w') != std::string::npos) {
            if (const auto* code = std::get_if<Integer>(&arguments[0])) {
                if (*code < 0 && *code >= -32768) {
                    arguments[0] = Value{static_cast<Integer>(*code + 65536)};
                }
            }
        }
    }
    if ((key == "string" || key == "string$") && arguments.size() >= 2U) {
        // A number is the code of the character, modulo 256; a Date does not
        // fit the Integer that String converts it to.
        std::optional<double> code;
        if (const auto* date = std::get_if<DateValue>(&arguments[1])) {
            code = date->serial;
        } else if (const auto* flag = std::get_if<bool>(&arguments[1])) {
            code = *flag ? -1.0 : 0.0;
        } else if (std::holds_alternative<Empty>(arguments[1])) {
            code = 0.0;
        } else if (is_number(arguments[1]) &&
                   !std::holds_alternative<Integer>(arguments[1])) {
            code = as_double(arguments[1]);
        }
        if (code.has_value()) {
            const double rounded = std::nearbyint(*code);
            if (rounded < -32768.0 || rounded > 32767.0) {
                set_error("WFC0009", "numeric overflow", offset);
                result = std::nullopt;
                return true;
            }
            const auto whole = static_cast<Integer>(rounded);
            arguments[1] =
                Value{static_cast<Integer>(((whole % 256) + 256) % 256)};
        }
    }

    // The size of a variable, and of the text of a Variant.
    if ((key == "len" || key == "lenb") && typed(0)) {
        if (const auto size = size_of_type(arguments[0])) {
            result = Value{*size};
            return true;
        }
    }
    if (key == "lenb" && !typed(0) &&
        !std::holds_alternative<Empty>(arguments[0]) &&
        !std::holds_alternative<std::string>(arguments[0]) &&
        size_of_type(arguments[0]).has_value()) {
        result = Value{static_cast<Integer>(2 * render(arguments[0]).size())};
        return true;
    }

    // Str: a leading space for the sign; Empty is 0; a String that is a
    // number, a Date and a Boolean are written as what they hold.
    if (key == "str" || key == "str$") {
        if (std::holds_alternative<Empty>(arguments[0])) {
            result = Value{std::string{" 0"}};
            return true;
        }
        if (const auto* date = std::get_if<DateValue>(&arguments[0])) {
            result = Value{typed(0) ? " " + render(Value{*date})
                                    : render(Value{*date})};
            return true;
        }
        if (const auto* flag = std::get_if<bool>(&arguments[0])) {
            const std::string text = *flag ? "True" : "False";
            result = Value{typed(0) ? " " + text : text};
            return true;
        }
        as_serial(0);
    }
    return false;
}

}  // namespace wfc::detail
