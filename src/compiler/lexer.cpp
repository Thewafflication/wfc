#include "lexer.hpp"

#include <cctype>

namespace wfc::compiler {

namespace {

[[nodiscard]] char lower(const char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

[[nodiscard]] bool is_letter(const char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
}

[[nodiscard]] bool is_digit(const char c) {
    return c >= '0' && c <= '9';
}

[[nodiscard]] bool is_hex_digit(const char c) {
    return std::isxdigit(static_cast<unsigned char>(c)) != 0;
}

[[nodiscard]] int hex_value(const char c) {
    if (is_digit(c)) {
        return c - '0';
    }
    return lower(c) - 'a' + 10;
}

class Lexer final {
public:
    explicit Lexer(const std::string_view source) : text_(source) {}

    LexResult run() {
        while (position_ < text_.size() && result_.error.empty()) {
            step();
        }
        if (result_.error.empty()) {
            // A final statement end keeps the parser's life simple.
            push_end_of_statement();
            Token end;
            end.kind = TokenKind::end_of_file;
            end.line = line_;
            end.column = column();
            result_.tokens.push_back(end);
        }
        return std::move(result_);
    }

private:
    [[nodiscard]] std::size_t column() const {
        return position_ - line_start_ + 1U;
    }

    [[nodiscard]] char peek(const std::size_t ahead = 0) const {
        return position_ + ahead < text_.size() ? text_[position_ + ahead]
                                                : '\0';
    }

    void fail(const std::string& message) {
        result_.error = message;
        result_.error_line = line_;
        result_.error_column = column();
    }

    void push_end_of_statement(const bool colon = false) {
        if (result_.tokens.empty() ||
            result_.tokens.back().kind == TokenKind::end_of_statement) {
            return;
        }
        Token token;
        token.kind = TokenKind::end_of_statement;
        token.text = colon ? ":" : "\n";
        token.line = line_;
        token.column = column();
        result_.tokens.push_back(token);
    }

    void new_line() {
        ++position_;
        ++line_;
        line_start_ = position_;
    }

    void skip_comment() {
        while (position_ < text_.size() && text_[position_] != '\n') {
            ++position_;
        }
    }

    void step() {
        const char c = text_[position_];
        if (c == ' ' || c == '\t' || c == '\r') {
            ++position_;
            return;
        }
        if (c == '\n') {
            push_end_of_statement();
            at_line_start_ = true;
            new_line();
            return;
        }
        if (c == '\'') {
            skip_comment();
            return;
        }
        if (c == '_' && is_continuation()) {
            // A line continuation: " _" before the line break.
            while (position_ < text_.size() && text_[position_] != '\n') {
                ++position_;
            }
            if (position_ < text_.size()) {
                new_line();
            }
            return;
        }
        if (c == ':') {
            push_end_of_statement(true);
            ++position_;
            return;
        }
        if (is_letter(c)) {
            word();
            return;
        }
        if (is_digit(c) || (c == '.' && is_digit(peek(1)))) {
            number();
            return;
        }
        if (c == '&' && (lower(peek(1)) == 'h' || lower(peek(1)) == 'o') &&
            is_hex_digit(peek(2))) {
            based_number();
            return;
        }
        if (c == '"') {
            string_literal();
            return;
        }
        symbol();
    }

    // `_` ends a line when it follows white space (or starts the line) and
    // only white space or a comment follows it.
    [[nodiscard]] bool is_continuation() const {
        if (position_ > line_start_ &&
            !(text_[position_ - 1U] == ' ' || text_[position_ - 1U] == '\t')) {
            return false;
        }
        std::size_t at = position_ + 1U;
        while (at < text_.size() &&
               (text_[at] == ' ' || text_[at] == '\t' || text_[at] == '\r')) {
            ++at;
        }
        return at >= text_.size() || text_[at] == '\n' || text_[at] == '\'';
    }

    void word() {
        const auto start = position_;
        const auto start_column = column();
        while (position_ < text_.size() &&
               (is_letter(text_[position_]) || is_digit(text_[position_]))) {
            ++position_;
        }
        Token token;
        token.kind = TokenKind::identifier;
        token.spelling = std::string(text_.substr(start, position_ - start));
        for (const char c : token.spelling) {
            token.text.push_back(lower(c));
        }
        token.line = line_;
        token.column = start_column;
        token.line_start = at_line_start_;
        at_line_start_ = false;
        // `Rem` starts a comment.
        if (token.text == "rem") {
            skip_comment();
            return;
        }
        // A type-declaration suffix (`x%`, `s$`) is part of the name.
        const char suffix = peek();
        if (suffix == '%' || suffix == '&' || suffix == '!' || suffix == '#' ||
            suffix == '@' || suffix == '$') {
            if (!(suffix == '&' && peek(1) == '&')) {
                token.text.push_back(suffix);
                token.spelling.push_back(suffix);
                ++position_;
            }
        }
        result_.tokens.push_back(std::move(token));
    }

    void based_number() {
        at_line_start_ = false;
        const auto start_column = column();
        const bool is_hex = lower(peek(1)) == 'h';
        position_ += 2U;
        std::int64_t value = 0;
        while (position_ < text_.size() &&
               (is_hex
                    ? is_hex_digit(text_[position_])
                    : (text_[position_] >= '0' && text_[position_] <= '7'))) {
            value =
                value * (is_hex ? 16 : 8) +
                (is_hex ? hex_value(text_[position_]) : text_[position_] - '0');
            ++position_;
        }
        Token token;
        token.kind = TokenKind::integer;
        token.line = line_;
        token.column = start_column;
        // `&HFFFF` is the Integer -1; `&HFFFF&` is the Long 65535.
        bool explicit_long = false;
        if (peek() == '&') {
            explicit_long = true;
            ++position_;
        }
        if (!explicit_long && value >= 0x8000 && value <= 0xFFFF) {
            value -= 0x10000;
        } else if (explicit_long) {
            token.long_literal = true;
        } else if (value > 0xFFFF) {
            token.long_literal = true;
            if (value >= 0x80000000LL && value <= 0xFFFFFFFFLL) {
                value -= 0x100000000LL;
            }
        }
        token.number = value;
        result_.tokens.push_back(std::move(token));
    }

    void number() {
        at_line_start_ = false;
        const auto start = position_;
        const auto start_column = column();
        bool real = false;
        while (position_ < text_.size() && is_digit(text_[position_])) {
            ++position_;
        }
        if (peek() == '.') {
            real = true;
            ++position_;
            while (position_ < text_.size() && is_digit(text_[position_])) {
                ++position_;
            }
        }
        if (lower(peek()) == 'e' &&
            (is_digit(peek(1)) ||
             ((peek(1) == '+' || peek(1) == '-') && is_digit(peek(2))))) {
            real = true;
            position_ += 2U;
            while (position_ < text_.size() && is_digit(text_[position_])) {
                ++position_;
            }
        }
        Token token;
        token.line = line_;
        token.column = start_column;
        token.text = std::string(text_.substr(start, position_ - start));
        // `5!` and `5#` are floating-point literals too.
        if (!real && (peek() == '!' || peek() == '#')) {
            real = true;
        }
        if (real) {
            token.kind = TokenKind::real;
            if (peek() == '!') {
                token.single_literal = true;
                ++position_;
            } else if (peek() == '#') {
                ++position_;
            }
        } else {
            token.kind = TokenKind::integer;
            std::int64_t value = 0;
            for (const char c : token.text) {
                value = value * 10 + (c - '0');
                if (value > 0x7FFFFFFFFFLL) {
                    fail("integer literal is too large");
                    return;
                }
            }
            token.number = value;
            if (peek() == '&' && peek(1) != '&') {
                token.long_literal = true;
                ++position_;
            } else if (peek() == '%') {
                ++position_;
            }
            if (value > 32767) {
                token.long_literal = true;
            }
        }
        result_.tokens.push_back(std::move(token));
    }

    void string_literal() {
        at_line_start_ = false;
        const auto start_column = column();
        ++position_;
        std::string contents;
        while (true) {
            if (position_ >= text_.size() || text_[position_] == '\n') {
                fail("unterminated string literal");
                return;
            }
            const char c = text_[position_];
            if (c == '"') {
                if (peek(1) == '"') {
                    contents.push_back('"');
                    position_ += 2U;
                    continue;
                }
                ++position_;
                break;
            }
            contents.push_back(c);
            ++position_;
        }
        Token token;
        token.kind = TokenKind::string;
        token.text = std::move(contents);
        token.line = line_;
        token.column = start_column;
        result_.tokens.push_back(std::move(token));
    }

    void symbol() {
        at_line_start_ = false;
        const auto start_column = column();
        const char c = text_[position_];
        std::string text(1, c);
        const char next = peek(1);
        if ((c == '<' && (next == '=' || next == '>')) ||
            (c == '>' && next == '=')) {
            text.push_back(next);
        }
        const std::string_view single = "+-*/\\^&=<>(),;.#!@%";
        if (text.size() == 1U && single.find(c) == std::string_view::npos) {
            fail(std::string("unexpected character '") + c + "'");
            return;
        }
        position_ += text.size();
        Token token;
        token.kind = TokenKind::symbol;
        token.text = std::move(text);
        token.line = line_;
        token.column = start_column;
        result_.tokens.push_back(std::move(token));
    }

    std::string_view text_;
    bool at_line_start_{true};
    std::size_t position_{};
    std::size_t line_{1};
    std::size_t line_start_{};
    LexResult result_;
};

}  // namespace

LexResult tokenize(const std::string_view source) {
    Lexer lexer(source);
    return lexer.run();
}

}  // namespace wfc::compiler
