#include "wfc/evaluator.hpp"

#include <charconv>
#include <cctype>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace {

using Integer = std::int32_t;
using Value = std::variant<Integer, std::string>;

[[nodiscard]] char ascii_lower(const char character) noexcept {
    if (character >= 'A' && character <= 'Z') {
        return static_cast<char>(character + ('a' - 'A'));
    }
    return character;
}

[[nodiscard]] wfc::Evaluation failure(
    const std::string_view code,
    const std::string_view message,
    const std::size_t offset) {
    wfc::Evaluation result;
    result.diagnostic = std::string(code) + " at byte " +
                        std::to_string(offset + 1) + ": " + std::string(message);
    result.error_offset = offset;
    return result;
}

class Parser final {
public:
    explicit Parser(const std::string_view source) : source_(source) {}

    [[nodiscard]] wfc::Evaluation evaluate() {
        skip_whitespace();
        const auto statement_start = offset_;
        if (!consume_keyword("print")) {
            return failure("WFC0001", "expected Print statement", statement_start);
        }

        skip_whitespace();
        auto value = parse_concatenation();
        if (!value.has_value()) {
            return std::move(error_);
        }

        skip_whitespace();
        if (!at_end()) {
            return failure("WFC0004", "unexpected trailing input", offset_);
        }

        wfc::Evaluation result;
        result.success = true;
        result.output = render(*value);
        return result;
    }

private:
    [[nodiscard]] bool at_end() const noexcept { return offset_ == source_.size(); }
    [[nodiscard]] char current() const noexcept { return source_[offset_]; }
    void advance() noexcept { ++offset_; }

    void skip_whitespace() noexcept {
        while (!at_end()) {
            const auto character = static_cast<unsigned char>(current());
            if (std::isspace(character) == 0) {
                break;
            }
            advance();
        }
    }

    [[nodiscard]] bool consume(const char character) noexcept {
        if (at_end() || current() != character) {
            return false;
        }
        advance();
        return true;
    }

    [[nodiscard]] bool consume_keyword(const std::string_view keyword) noexcept {
        const auto start = offset_;
        for (const char expected : keyword) {
            if (at_end() || ascii_lower(current()) != expected) {
                offset_ = start;
                return false;
            }
            advance();
        }

        if (!at_end()) {
            const auto next = static_cast<unsigned char>(current());
            if (std::isalnum(next) != 0 || current() == '_') {
                offset_ = start;
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] std::optional<Value> parse_concatenation() {
        auto left = parse_additive();
        if (!left.has_value()) {
            return std::nullopt;
        }

        while (true) {
            skip_whitespace();
            if (!consume('&')) {
                return left;
            }
            skip_whitespace();
            auto right = parse_additive();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = Value{render(*left) + render(*right)};
        }
    }

    [[nodiscard]] std::optional<Value> parse_additive() {
        auto left = parse_multiplicative();
        if (!left.has_value()) {
            return std::nullopt;
        }

        while (true) {
            skip_whitespace();
            const auto operator_offset = offset_;
            char operation = '\0';
            if (consume('+')) {
                operation = '+';
            } else if (consume('-')) {
                operation = '-';
            } else {
                return left;
            }

            skip_whitespace();
            auto right = parse_multiplicative();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = integer_binary(*left, *right, operation, operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_multiplicative() {
        auto left = parse_unary();
        if (!left.has_value()) {
            return std::nullopt;
        }

        while (true) {
            skip_whitespace();
            const auto operator_offset = offset_;
            char operation = '\0';
            if (consume('*')) {
                operation = '*';
            } else if (consume('\\')) {
                operation = '\\';
            } else if (consume_keyword("mod")) {
                operation = '%';
            } else {
                return left;
            }

            skip_whitespace();
            auto right = parse_unary();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = integer_binary(*left, *right, operation, operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_unary() {
        skip_whitespace();
        const auto operator_offset = offset_;
        if (consume('+')) {
            auto value = parse_unary();
            if (!value.has_value() || require_integer(*value, operator_offset) == nullptr) {
                return std::nullopt;
            }
            return value;
        }
        if (consume('-')) {
            skip_whitespace();
            if (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0) {
                const auto start = offset_;
                while (!at_end() &&
                       std::isdigit(static_cast<unsigned char>(current())) != 0) {
                    advance();
                }

                std::uint64_t magnitude{};
                const auto conversion = std::from_chars(
                    source_.data() + start, source_.data() + offset_, magnitude);
                constexpr auto maximum_magnitude =
                    static_cast<std::uint64_t>(std::numeric_limits<Integer>::max()) + 1U;
                if (conversion.ec == std::errc::result_out_of_range ||
                    magnitude > maximum_magnitude) {
                    set_error("WFC0006", "integer literal out of range", start);
                    return std::nullopt;
                }
                if (magnitude == maximum_magnitude) {
                    return Value{std::numeric_limits<Integer>::min()};
                }
                return Value{static_cast<Integer>(-static_cast<Integer>(magnitude))};
            }

            auto value = parse_unary();
            if (!value.has_value()) {
                return std::nullopt;
            }
            const auto* integer = require_integer(*value, operator_offset);
            if (integer == nullptr) {
                return std::nullopt;
            }
            if (*integer == std::numeric_limits<Integer>::min()) {
                set_error("WFC0009", "integer overflow", operator_offset);
                return std::nullopt;
            }
            return Value{static_cast<Integer>(-*integer)};
        }
        return parse_primary();
    }

    [[nodiscard]] std::optional<Value> parse_primary() {
        skip_whitespace();
        if (at_end()) {
            set_error("WFC0002", "expected expression", offset_);
            return std::nullopt;
        }

        if (current() == '"') {
            return parse_string();
        }
        if (std::isdigit(static_cast<unsigned char>(current())) != 0) {
            return parse_integer();
        }
        if (consume('(')) {
            auto value = parse_concatenation();
            if (!value.has_value()) {
                return std::nullopt;
            }
            skip_whitespace();
            if (!consume(')')) {
                set_error("WFC0005", "expected closing parenthesis", offset_);
                return std::nullopt;
            }
            return value;
        }

        set_error("WFC0002", "expected expression", offset_);
        return std::nullopt;
    }

    [[nodiscard]] std::optional<Value> parse_string() {
        advance();
        std::string value;
        while (!at_end()) {
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

    [[nodiscard]] std::optional<Value> parse_integer() {
        const auto start = offset_;
        while (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0) {
            advance();
        }

        Integer value{};
        const auto first = source_.data() + start;
        const auto last = source_.data() + offset_;
        const auto conversion = std::from_chars(first, last, value);
        if (conversion.ec == std::errc::result_out_of_range) {
            set_error("WFC0006", "integer literal out of range", start);
            return std::nullopt;
        }
        return Value{value};
    }

    [[nodiscard]] const Integer* require_integer(
        const Value& value,
        const std::size_t operator_offset) {
        const auto* integer = std::get_if<Integer>(&value);
        if (integer == nullptr) {
            set_error("WFC0007", "operator requires integer operands", operator_offset);
        }
        return integer;
    }

    [[nodiscard]] std::optional<Value> integer_binary(
        const Value& left,
        const Value& right,
        const char operation,
        const std::size_t operator_offset) {
        const auto* left_integer = require_integer(left, operator_offset);
        if (left_integer == nullptr) {
            return std::nullopt;
        }
        const auto* right_integer = require_integer(right, operator_offset);
        if (right_integer == nullptr) {
            return std::nullopt;
        }

        if ((operation == '\\' || operation == '%') && *right_integer == 0) {
            set_error("WFC0008", "division by zero", operator_offset);
            return std::nullopt;
        }
        if ((operation == '\\' || operation == '%') &&
            *left_integer == std::numeric_limits<Integer>::min() && *right_integer == -1) {
            set_error("WFC0009", "integer overflow", operator_offset);
            return std::nullopt;
        }

        std::int64_t result{};
        switch (operation) {
        case '+':
            result = static_cast<std::int64_t>(*left_integer) + *right_integer;
            break;
        case '-':
            result = static_cast<std::int64_t>(*left_integer) - *right_integer;
            break;
        case '*':
            result = static_cast<std::int64_t>(*left_integer) * *right_integer;
            break;
        case '\\':
            result = *left_integer / *right_integer;
            break;
        case '%':
            result = *left_integer % *right_integer;
            break;
        default:
            set_error("WFC0004", "unsupported operator", operator_offset);
            return std::nullopt;
        }

        if (result < std::numeric_limits<Integer>::min() ||
            result > std::numeric_limits<Integer>::max()) {
            set_error("WFC0009", "integer overflow", operator_offset);
            return std::nullopt;
        }
        return Value{static_cast<Integer>(result)};
    }

    [[nodiscard]] static std::string render(const Value& value) {
        if (const auto* integer = std::get_if<Integer>(&value)) {
            return std::to_string(*integer);
        }
        return std::get<std::string>(value);
    }

    void set_error(
        const std::string_view code,
        const std::string_view message,
        const std::size_t offset) {
        error_ = failure(code, message, offset);
    }

    std::string_view source_;
    std::size_t offset_{};
    wfc::Evaluation error_;
};

}  // namespace

namespace wfc {

Evaluation evaluate_print_statement(const std::string_view source) {
    return Parser(source).evaluate();
}

}  // namespace wfc
