// The statements of the BASIC PDS 7.1 text screen (ADR-0011): CLS, LOCATE,
// COLOR, WIDTH and VIEW PRINT. They act on the shared screen of pds_screen
// and are statements only outside form code (where Cls and Width belong to
// the form) and when the word is not a procedure of the program or followed
// by `=`, `.` or `#` (an assignment, a member, `Width #n`).
#include <cmath>
#include <limits>

#include "interpreter.hpp"
#include "pds_screen.hpp"

namespace wfc::detail {

bool Interpreter::at_pds_statement_end() {
    if (at_statement_end()) {
        return true;
    }
    const auto before = offset_;
    const bool is_else = consume_keyword("else");
    offset_ = before;
    return is_else;
}

bool Interpreter::pds_error(const int number, const std::size_t offset) {
    return raise_runtime(number,
                         number == brun71::kIllegalFunctionCall
                             ? std::string(brun71::error_text(number))
                             : vb_error_description(number),
                         offset);
}

std::optional<int> Interpreter::pds_integer(const Value& value,
                                            const std::size_t offset) {
    double number = 0.0;
    if (std::holds_alternative<Empty>(value)) {
        number = 0.0;
    } else if (const auto* flag = std::get_if<bool>(&value)) {
        number = *flag ? -1.0 : 0.0;
    } else if (is_number(value)) {
        number = as_double(value);
    } else {
        static_cast<void>(pds_error(13, offset));
        return std::nullopt;
    }
    // BASIC rounds to the nearest whole number, halves to the even one.
    const double rounded = std::nearbyint(number);
    if (!(rounded >= -32768.0 && rounded <= 32767.0)) {
        static_cast<void>(pds_error(6, offset));
        return std::nullopt;
    }
    return static_cast<int>(rounded);
}

bool Interpreter::parse_pds_arguments(
    std::vector<std::optional<int>>& arguments) {
    skip_horizontal_whitespace();
    if (at_pds_statement_end()) {
        return true;
    }
    while (true) {
        skip_horizontal_whitespace();
        std::optional<int> item;
        if (!at_pds_statement_end() && current() != ',') {
            const auto at = offset_;
            auto value = parse_expression();
            if (!value.has_value() || !resolve_default_value(*value, at)) {
                return false;
            }
            if (execute_) {
                item = pds_integer(*value, at);
                if (!item) {
                    return false;
                }
            } else {
                item = 0;
            }
        }
        arguments.push_back(item);
        skip_horizontal_whitespace();
        if (!at_end() && current() == ',') {
            advance();
            continue;
        }
        return true;
    }
}

std::optional<bool> Interpreter::parse_pds_statement(
    const std::size_t statement_offset) {
    static constexpr std::string_view kWords[] = {"cls", "locate", "color",
                                                  "width", "view"};
    const auto start = offset_;
    std::string_view word;
    for (const auto candidate : kWords) {
        if (consume_keyword(candidate)) {
            word = candidate;
            break;
        }
    }
    if (word.empty()) {
        return std::nullopt;
    }
    skip_horizontal_whitespace();
    const auto* const instance = current_instance();
    const bool not_a_statement =
        procedures_.contains(std::string(word)) ||
        (instance != nullptr && instance->native != nullptr) ||
        (!at_end() && (current() == '=' || current() == '.' ||
                       current() == '#' || current() == '('));
    if (not_a_statement) {
        offset_ = start;
        return std::nullopt;
    }
    if (word == "view") {
        if (!consume_keyword("print")) {
            offset_ = start;
            return std::nullopt;
        }
        skip_horizontal_whitespace();
        std::optional<int> top;
        std::optional<int> bottom;
        if (!at_pds_statement_end()) {
            const auto at = offset_;
            auto first = parse_expression();
            if (!first.has_value() || !resolve_default_value(*first, at)) {
                return false;
            }
            skip_horizontal_whitespace();
            if (!consume_keyword("to")) {
                set_error("WFC0014", "expected TO", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            const auto second_at = offset_;
            auto second = parse_expression();
            if (!second.has_value() ||
                !resolve_default_value(*second, second_at)) {
                return false;
            }
            if (execute_) {
                top = pds_integer(*first, at);
                if (!top) {
                    return false;
                }
                bottom = pds_integer(*second, second_at);
                if (!bottom) {
                    return false;
                }
            }
        }
        if (!execute_) {
            return true;
        }
        if (const int error = pds::run([&](brun71::TextScreen& screen) {
                screen.view_print(top, bottom);
            });
            error != 0) {
            return pds_error(error, statement_offset);
        }
        return true;
    }

    std::vector<std::optional<int>> arguments;
    if (!parse_pds_arguments(arguments)) {
        return false;
    }
    if (!execute_) {
        return true;
    }
    const auto argument = [&](const std::size_t index) -> std::optional<int> {
        return index < arguments.size() ? arguments[index] : std::nullopt;
    };
    const std::size_t most = word == "cls"     ? 1U
                             : word == "width" ? 2U
                             : word == "color" ? 3U
                                               : 5U;
    if (arguments.size() > most) {
        return pds_error(brun71::kIllegalFunctionCall, statement_offset);
    }
    const int error = pds::run([&](brun71::TextScreen& screen) {
        if (word == "cls") {
            screen.cls(argument(0));
        } else if (word == "locate") {
            screen.locate(argument(0), argument(1), argument(2), argument(3),
                          argument(4));
        } else if (word == "color") {
            screen.color(argument(0), argument(1), argument(2));
        } else {
            screen.width(argument(0), argument(1));
        }
    });
    if (error != 0) {
        return pds_error(error, statement_offset);
    }
    return true;
}

}  // namespace wfc::detail
