#include "wfc/evaluator.hpp"

#include <charconv>
#include <cctype>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
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

[[nodiscard]] bool is_identifier_start(const char character) noexcept {
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z');
}

[[nodiscard]] bool is_identifier_part(const char character) noexcept {
    return is_identifier_start(character) || (character >= '0' && character <= '9') ||
           character == '_';
}

[[nodiscard]] bool is_reserved_identifier(const std::string_view identifier) noexcept {
    return identifier == "as" || identifier == "dim" || identifier == "let" ||
           identifier == "long" || identifier == "mod" || identifier == "print" ||
           identifier == "string";
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

class Interpreter final {
public:
    explicit Interpreter(const std::string_view source, const bool allow_identifiers = true)
        : source_(source), allow_identifiers_(allow_identifiers) {}

    [[nodiscard]] wfc::Evaluation evaluate() {
        skip_program_leading_trivia();
        if (at_end()) {
            return failure("WFC0001", "expected statement", offset_);
        }
        while (!at_end()) {
            if (!parse_statement()) {
                return std::move(error_);
            }
            if (!consume_statement_end()) {
                return std::move(error_);
            }
            skip_program_leading_trivia();
        }

        wfc::Evaluation result;
        result.success = true;
        result.output = std::move(output_);
        return result;
    }

private:
    [[nodiscard]] bool at_end() const noexcept { return offset_ == source_.size(); }
    [[nodiscard]] char current() const noexcept { return source_[offset_]; }
    void advance() noexcept { ++offset_; }

    void skip_horizontal_whitespace() noexcept {
        while (!at_end() && (current() == ' ' || current() == '\t' || current() == '\f' ||
                             current() == '\v')) {
            advance();
        }
    }

    [[nodiscard]] bool consume_line_break() noexcept {
        if (at_end()) {
            return false;
        }
        if (current() == '\r') {
            advance();
            if (!at_end() && current() == '\n') {
                advance();
            }
            return true;
        }
        if (current() == '\n') {
            advance();
            return true;
        }
        return false;
    }

    void skip_comment() noexcept {
        while (!at_end() && current() != '\r' && current() != '\n') {
            advance();
        }
    }

    void skip_program_leading_trivia() noexcept {
        while (true) {
            skip_horizontal_whitespace();
            if (!at_end() && current() == '\'') {
                skip_comment();
            }
            if (!consume_line_break()) {
                return;
            }
        }
    }

    [[nodiscard]] bool consume_statement_end() {
        skip_horizontal_whitespace();
        if (!at_end() && current() == '\'') {
            skip_comment();
        }
        if (at_end()) {
            return true;
        }
        if (current() == ':') {
            advance();
            return true;
        }
        if (consume_line_break()) {
            return true;
        }
        set_error("WFC0004", "unexpected trailing input", offset_);
        return false;
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

        if (!at_end() && is_identifier_part(current())) {
            offset_ = start;
            return false;
        }
        return true;
    }

    [[nodiscard]] std::optional<std::string> parse_identifier() {
        if (at_end() || !is_identifier_start(current())) {
            return std::nullopt;
        }

        std::string identifier;
        do {
            identifier.push_back(ascii_lower(current()));
            advance();
        } while (!at_end() && is_identifier_part(current()));
        return identifier;
    }

    [[nodiscard]] bool parse_statement() {
        skip_horizontal_whitespace();
        const auto statement_offset = offset_;
        if (consume_keyword("print")) {
            return parse_print_statement();
        }
        if (consume_keyword("dim")) {
            return parse_declaration();
        }

        const bool has_let = consume_keyword("let");
        if (has_let) {
            skip_horizontal_whitespace();
        }
        auto identifier = parse_identifier();
        if (!identifier.has_value()) {
            set_error("WFC0010", "expected statement", statement_offset);
            return false;
        }
        return parse_assignment(std::move(*identifier));
    }

    [[nodiscard]] bool parse_print_statement() {
        skip_horizontal_whitespace();
        auto value = parse_concatenation();
        if (!value.has_value()) {
            return false;
        }
        if (has_output_line_) {
            output_.push_back('\n');
        }
        output_ += render(*value);
        has_output_line_ = true;
        return true;
    }

    [[nodiscard]] bool parse_declaration() {
        skip_horizontal_whitespace();
        const auto identifier_offset = offset_;
        auto identifier = parse_identifier();
        if (!identifier.has_value()) {
            set_error("WFC0011", "expected variable name", identifier_offset);
            return false;
        }
        if (is_reserved_identifier(*identifier)) {
            set_error("WFC0017", "reserved keyword cannot be a variable name", identifier_offset);
            return false;
        }

        skip_horizontal_whitespace();
        if (!consume_keyword("as")) {
            set_error("WFC0012", "expected As Long or As String", offset_);
            return false;
        }
        skip_horizontal_whitespace();

        Value initial_value;
        if (consume_keyword("long")) {
            initial_value = Integer{};
        } else if (consume_keyword("string")) {
            initial_value = std::string{};
        } else {
            set_error("WFC0012", "expected As Long or As String", offset_);
            return false;
        }

        const auto [entry, inserted] = variables_.emplace(*identifier, std::move(initial_value));
        (void)entry;
        if (!inserted) {
            set_error("WFC0013", "duplicate variable declaration", identifier_offset);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool parse_assignment(std::string identifier) {
        const auto identifier_offset = offset_ - identifier.size();
        const auto variable = variables_.find(identifier);
        if (variable == variables_.end()) {
            set_error("WFC0015", "undeclared variable", identifier_offset);
            return false;
        }

        skip_horizontal_whitespace();
        if (!consume('=')) {
            set_error("WFC0014", "expected assignment operator", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        auto value = parse_concatenation();
        if (!value.has_value()) {
            return false;
        }
        if (variable->second.index() != value->index()) {
            set_error("WFC0016", "assignment type mismatch", identifier_offset);
            return false;
        }
        variable->second = std::move(*value);
        return true;
    }

    [[nodiscard]] std::optional<Value> parse_concatenation() {
        auto left = parse_additive();
        if (!left.has_value()) {
            return std::nullopt;
        }

        while (true) {
            skip_horizontal_whitespace();
            if (!consume('&')) {
                return left;
            }
            skip_horizontal_whitespace();
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
            skip_horizontal_whitespace();
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

            skip_horizontal_whitespace();
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
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        if (consume('+')) {
            auto value = parse_unary();
            if (!value.has_value() || require_integer(*value, operator_offset) == nullptr) {
                return std::nullopt;
            }
            return value;
        }
        if (consume('-')) {
            skip_horizontal_whitespace();
            if (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0) {
                return parse_negative_integer();
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
        skip_horizontal_whitespace();
        if (at_end() || current() == '\r' || current() == '\n' || current() == ':' ||
            current() == '\'') {
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
            skip_horizontal_whitespace();
            if (!consume(')')) {
                set_error("WFC0005", "expected closing parenthesis", offset_);
                return std::nullopt;
            }
            return value;
        }
        if (is_identifier_start(current())) {
            const auto identifier_offset = offset_;
            if (!allow_identifiers_) {
                set_error("WFC0002", "expected expression", identifier_offset);
                return std::nullopt;
            }
            auto identifier = parse_identifier();
            const auto variable = variables_.find(*identifier);
            if (variable == variables_.end()) {
                set_error("WFC0015", "undeclared variable", identifier_offset);
                return std::nullopt;
            }
            return variable->second;
        }

        set_error("WFC0002", "expected expression", offset_);
        return std::nullopt;
    }

    [[nodiscard]] std::optional<Value> parse_string() {
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

    [[nodiscard]] std::optional<Value> parse_integer() {
        const auto start = offset_;
        while (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0) {
            advance();
        }

        Integer value{};
        const auto conversion =
            std::from_chars(source_.data() + start, source_.data() + offset_, value);
        if (conversion.ec == std::errc::result_out_of_range) {
            set_error("WFC0006", "integer literal out of range", start);
            return std::nullopt;
        }
        return Value{value};
    }

    [[nodiscard]] std::optional<Value> parse_negative_integer() {
        const auto start = offset_;
        while (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0) {
            advance();
        }

        std::uint64_t magnitude{};
        const auto conversion =
            std::from_chars(source_.data() + start, source_.data() + offset_, magnitude);
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
    bool allow_identifiers_;
    std::size_t offset_{};
    std::unordered_map<std::string, Value> variables_;
    std::string output_;
    bool has_output_line_{};
    wfc::Evaluation error_;
};

}  // namespace

namespace wfc {

Evaluation evaluate_program(const std::string_view source) {
    return Interpreter(source).evaluate();
}

Evaluation evaluate_print_statement(const std::string_view source) {
    std::size_t offset{};
    while (offset < source.size() &&
           std::isspace(static_cast<unsigned char>(source[offset])) != 0) {
        ++offset;
    }
    const auto statement_offset = offset;
    constexpr std::string_view keyword = "print";
    for (const char expected : keyword) {
        if (offset == source.size() || ascii_lower(source[offset]) != expected) {
            return failure("WFC0001", "expected Print statement", statement_offset);
        }
        ++offset;
    }
    if (offset < source.size() && is_identifier_part(source[offset])) {
        return failure("WFC0001", "expected Print statement", statement_offset);
    }
    return Interpreter(source, false).evaluate();
}

}  // namespace wfc
