#pragma once

// Lexer of the native compiler front end (ADR-0008). It turns VB source text
// into tokens: identifiers (lower-cased, so keywords match without regard to
// case), integer and string literals, operators and statement ends.

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace wfc::compiler {

enum class TokenKind {
    identifier,  // text is lower-cased; original spelling in `spelling`
    integer,     // `number` holds the value
    real,        // a floating-point literal; `text` holds the digits
    string,      // `text` holds the contents without quotes
    date,        // `#1/5/2020#`: `text` holds the serial number
    symbol,      // an operator or punctuation: text is e.g. "<=", "(", ","
    end_of_statement,  // a line break or a colon
    end_of_file,
};

struct Token final {
    TokenKind kind{TokenKind::end_of_file};
    std::string text;
    std::string spelling;
    std::int64_t number{};
    // The literal had an explicit type suffix or was written in hex or octal
    // (`&H10`, `5&`), which makes a whole number a Long.
    bool long_literal{};
    // A floating-point literal with the suffix `!` (a Single); `#` and no
    // suffix make a Double.
    bool single_literal{};
    // The literal has the suffix `@`: a Currency.
    bool currency_literal{};
    // The first token of a line (a label can only start a line).
    bool line_start{};
    std::size_t line{1};
    std::size_t column{1};
};

struct LexResult final {
    std::vector<Token> tokens;
    std::string error;  // empty when the text was tokenized
    std::size_t error_line{};
    std::size_t error_column{};
};

[[nodiscard]] LexResult tokenize(std::string_view source);

}  // namespace wfc::compiler
