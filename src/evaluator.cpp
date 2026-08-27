#include "wfc/evaluator.hpp"

#include <cctype>
#include <string>
#include <utility>

namespace {

class Cursor final {
public:
    explicit Cursor(const std::string_view source) : source_(source) {}

    [[nodiscard]] bool at_end() const noexcept { return offset_ == source_.size(); }

    [[nodiscard]] char current() const noexcept { return source_[offset_]; }

    [[nodiscard]] std::size_t offset() const noexcept { return offset_; }

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

private:
    std::string_view source_;
    std::size_t offset_{};
};

[[nodiscard]] char ascii_lower(const char character) noexcept {
    if (character >= 'A' && character <= 'Z') {
        return static_cast<char>(character + ('a' - 'A'));
    }
    return character;
}

[[nodiscard]] bool consume_keyword(Cursor& cursor, const std::string_view keyword) {
    for (const char expected : keyword) {
        if (cursor.at_end() || ascii_lower(cursor.current()) != expected) {
            return false;
        }
        cursor.advance();
    }

    if (!cursor.at_end()) {
        const auto next = static_cast<unsigned char>(cursor.current());
        if (std::isalnum(next) != 0 || cursor.current() == '_') {
            return false;
        }
    }
    return true;
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

}  // namespace

namespace wfc {

Evaluation evaluate_print_statement(const std::string_view source) {
    Cursor cursor(source);
    cursor.skip_whitespace();

    const auto statement_start = cursor.offset();
    if (!consume_keyword(cursor, "print")) {
        return failure("WFC0001", "expected Print statement", statement_start);
    }

    cursor.skip_whitespace();
    if (cursor.at_end() || cursor.current() != '"') {
        return failure("WFC0002", "expected string literal", cursor.offset());
    }
    cursor.advance();

    std::string value;
    bool terminated = false;
    while (!cursor.at_end()) {
        if (cursor.current() != '"') {
            value.push_back(cursor.current());
            cursor.advance();
            continue;
        }

        cursor.advance();
        if (!cursor.at_end() && cursor.current() == '"') {
            value.push_back('"');
            cursor.advance();
            continue;
        }

        terminated = true;
        break;
    }

    if (!terminated) {
        return failure("WFC0003", "unterminated string literal", cursor.offset());
    }

    cursor.skip_whitespace();
    if (!cursor.at_end()) {
        return failure("WFC0004", "unexpected trailing input", cursor.offset());
    }

    Evaluation result;
    result.success = true;
    result.output = std::move(value);
    return result;
}

}  // namespace wfc
