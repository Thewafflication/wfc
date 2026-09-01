#include "wfc/evaluator.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace {

using Integer = std::int32_t;
using Value = std::variant<Integer, std::string, bool, double>;

enum class NumericStringStatus { valid, malformed, out_of_range };

struct NumericStringResult {
    NumericStringStatus status{NumericStringStatus::malformed};
    double value{};
};

// Parse the strict, locale-independent numeric String form shared by the VBA
// conversion intrinsics. Surrounding ASCII whitespace and a leading plus are
// accepted; the entire remaining decimal/exponent spelling must be consumed.
[[nodiscard]] NumericStringResult parse_numeric_string(const std::string_view text) {
    std::size_t first{};
    std::size_t last = text.size();
    const auto is_ascii_whitespace = [](const char character) {
        return character == ' ' || character == '\t' || character == '\r' ||
               character == '\n';
    };
    while (first < last && is_ascii_whitespace(text[first])) {
        ++first;
    }
    while (last > first && is_ascii_whitespace(text[last - 1U])) {
        --last;
    }
    if (first < last && text[first] == '+') {
        ++first;
    }
    if (first == last) {
        return {};
    }

    double value{};
    const auto conversion =
        std::from_chars(text.data() + first, text.data() + last, value);
    if (conversion.ec == std::errc::result_out_of_range) {
        return {NumericStringStatus::out_of_range, value};
    }
    if (conversion.ec != std::errc{} || conversion.ptr != text.data() + last ||
        !std::isfinite(value)) {
        return {};
    }
    return {NumericStringStatus::valid, value};
}

// A value participates in numeric operators when it is a Long or a Double.
[[nodiscard]] inline bool is_number(const Value& value) noexcept {
    return std::holds_alternative<Integer>(value) ||
           std::holds_alternative<double>(value);
}

// Widen a Long or Double value to double for mixed-type numeric evaluation.
[[nodiscard]] inline double as_double(const Value& value) noexcept {
    if (const auto* integer = std::get_if<Integer>(&value)) {
        return static_cast<double>(*integer);
    }
    return std::get<double>(value);
}

[[nodiscard]] char ascii_lower(const char character) noexcept {
    if (character >= 'A' && character <= 'Z') {
        return static_cast<char>(character + ('a' - 'A'));
    }
    return character;
}

[[nodiscard]] char ascii_upper(const char character) noexcept {
    if (character >= 'a' && character <= 'z') {
        return static_cast<char>(character - ('a' - 'A'));
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

// Source-visible VBA constant enumerations. Each name resolves to the value in
// its `REQ-008x`/`REQ-0089` type-library contract and is a reserved identifier
// so source cannot shadow it. Members are exposed for source compatibility;
// they carry no runtime behavior beyond their integer value.
[[nodiscard]] std::optional<Integer> vba_constant_value(
    const std::string_view identifier) {
    static const std::unordered_map<std::string_view, Integer> table = {
        // VbCompareMethod (REQ-0089)
        {"vbbinarycompare", 0}, {"vbtextcompare", 1}, {"vbdatabasecompare", 2},
        // VbVarType (REQ-0080)
        {"vbempty", 0}, {"vbnull", 1}, {"vbinteger", 2}, {"vblong", 3},
        {"vbsingle", 4}, {"vbdouble", 5}, {"vbcurrency", 6}, {"vbdate", 7},
        {"vbstring", 8}, {"vbobject", 9}, {"vberror", 10}, {"vbboolean", 11},
        {"vbvariant", 12}, {"vbdataobject", 13}, {"vbdecimal", 14},
        {"vbbyte", 17}, {"vbuserdefinedtype", 36}, {"vbarray", 8192},
        // VbStrConv (REQ-0084)
        {"vbuppercase", 1}, {"vblowercase", 2}, {"vbpropercase", 3},
        {"vbwide", 4}, {"vbnarrow", 8}, {"vbkatakana", 16}, {"vbhiragana", 32},
        {"vbunicode", 64}, {"vbfromunicode", 128},
        // VbTriState (REQ-0092)
        {"vbusedefault", -2}, {"vbtrue", -1}, {"vbfalse", 0},
        // VbCallType (REQ-0093)
        {"vbmethod", 1}, {"vbget", 2}, {"vblet", 4}, {"vbset", 8},
        // VbFileAttribute (REQ-0083)
        {"vbnormal", 0}, {"vbreadonly", 1}, {"vbhidden", 2}, {"vbsystem", 4},
        {"vbvolume", 8}, {"vbdirectory", 16}, {"vbarchive", 32}, {"vbalias", 64},
        // VbMsgBoxResult (REQ-0082)
        {"vbok", 1}, {"vbcancel", 2}, {"vbabort", 3}, {"vbretry", 4},
        {"vbignore", 5}, {"vbyes", 6}, {"vbno", 7},
        // VbDayOfWeek (REQ-0085)
        {"vbusesystemdayofweek", 0}, {"vbsunday", 1}, {"vbmonday", 2},
        {"vbtuesday", 3}, {"vbwednesday", 4}, {"vbthursday", 5},
        {"vbfriday", 6}, {"vbsaturday", 7},
        // VbMsgBoxStyle (REQ-0081)
        {"vbokonly", 0}, {"vbokcancel", 1}, {"vbabortretryignore", 2},
        {"vbyesnocancel", 3}, {"vbyesno", 4}, {"vbretrycancel", 5},
        {"vbcritical", 16}, {"vbquestion", 32}, {"vbexclamation", 48},
        {"vbinformation", 64}, {"vbdefaultbutton1", 0}, {"vbdefaultbutton2", 256},
        {"vbdefaultbutton3", 512}, {"vbdefaultbutton4", 768},
        {"vbapplicationmodal", 0}, {"vbsystemmodal", 4096},
        {"vbmsgboxhelpbutton", 16384}, {"vbmsgboxright", 524288},
        {"vbmsgboxrtlreading", 1048576}, {"vbmsgboxsetforeground", 65536},
        // VbAppWinStyle (REQ-0088)
        {"vbhide", 0}, {"vbnormalfocus", 1}, {"vbminimizedfocus", 2},
        {"vbmaximizedfocus", 3}, {"vbnormalnofocus", 4}, {"vbminimizednofocus", 6},
        // VbFirstWeekOfYear (REQ-0086)
        {"vbusesystem", 0}, {"vbfirstjan1", 1}, {"vbfirstfourdays", 2},
        {"vbfirstfullweek", 3},
        // VbCalendar (REQ-0090)
        {"vbcalgreg", 0}, {"vbcalhijri", 1},
        // VbDateTimeFormat (REQ-0091)
        {"vbgeneraldate", 0}, {"vblongdate", 1}, {"vbshortdate", 2},
        {"vblongtime", 3}, {"vbshorttime", 4},
        // VbIMEStatus (REQ-0087)
        {"vbimenoop", 0}, {"vbimemodenocontrol", 0}, {"vbimeon", 1},
        {"vbimemodeon", 1}, {"vbimeoff", 2}, {"vbimemodeoff", 2},
        {"vbimedisable", 3}, {"vbimemodedisable", 3}, {"vbimehiragana", 4},
        {"vbimemodehiragana", 4}, {"vbimekatakanadbl", 5},
        {"vbimemodekatakana", 5}, {"vbimekatakanasng", 6},
        {"vbimemodekatakanahalf", 6}, {"vbimealphadbl", 7},
        {"vbimemodealphafull", 7}, {"vbimealphasng", 8}, {"vbimemodealpha", 8},
        {"vbimemodehangulfull", 9}, {"vbimemodehangul", 10},
        // General constants (REQ-0094), integer member
        {"vbobjecterror", -2147221504},
    };
    const auto entry = table.find(identifier);
    if (entry == table.end()) {
        return std::nullopt;
    }
    return entry->second;
}

// Source-visible VBA string constants (REQ-0094). Each resolves to its exact
// stored bytes and is reserved so source cannot shadow it.
[[nodiscard]] std::optional<std::string> vba_string_constant(
    const std::string_view identifier) {
    static const std::unordered_map<std::string_view, std::string> table = {
        {"vbnullstring", std::string{}},
        {"vbnullchar", std::string(1, '\0')},
        {"vbcrlf", "\r\n"}, {"vbnewline", "\r\n"},
        {"vbcr", "\r"}, {"vblf", "\n"}, {"vbback", "\b"},
        {"vbformfeed", "\f"}, {"vbtab", "\t"}, {"vbverticaltab", "\v"},
    };
    const auto entry = table.find(identifier);
    if (entry == table.end()) {
        return std::nullopt;
    }
    return entry->second;
}

[[nodiscard]] bool is_reserved_identifier(const std::string_view identifier) noexcept {
    return identifier == "and" || identifier == "as" || identifier == "boolean" ||
           identifier == "dim" || identifier == "do" || identifier == "eqv" ||
           identifier == "exit" || identifier == "false" || identifier == "for" ||
           identifier == "else" || identifier == "elseif" || identifier == "if" ||
           identifier == "imp" || identifier == "is" || identifier == "let" ||
           identifier == "long" || identifier == "case" || identifier == "const" ||
           identifier == "loop" || identifier == "mod" ||
           identifier == "next" ||
           identifier == "not" ||
           identifier == "option" || identifier == "or" ||
           identifier == "print" || identifier == "rem" || identifier == "select" ||
           identifier == "string" || identifier == "then" ||
           identifier == "explicit" || identifier == "step" || identifier == "to" ||
           identifier == "true" ||
           identifier == "until" || identifier == "wend" ||
           identifier == "while" || identifier == "xor" ||
           vba_constant_value(identifier).has_value() ||
           vba_string_constant(identifier).has_value();
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
    [[nodiscard]] char peek(const std::size_t ahead) const noexcept {
        const auto index = offset_ + ahead;
        return index < source_.size() ? source_[index] : '\0';
    }
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

    [[nodiscard]] bool consume_block_line_end() {
        skip_horizontal_whitespace();
        if (!at_end() && current() == '\'') {
            skip_comment();
        }
        if (consume_line_break()) {
            return true;
        }
        set_error("WFC0004", "expected line break", offset_);
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

    [[nodiscard]] std::optional<std::string> parse_identifier(
        char* const type_character = nullptr) {
        if (at_end() || !is_identifier_start(current())) {
            return std::nullopt;
        }

        std::string identifier;
        do {
            identifier.push_back(ascii_lower(current()));
            advance();
        } while (!at_end() && is_identifier_part(current()));
        if (type_character != nullptr) {
            *type_character = '\0';
            if (!at_end() && strchr("$%&!#@", current()) != nullptr) {
                *type_character = current();
                advance();
            }
        }
        return identifier;
    }

    [[nodiscard]] bool validate_type_character(
        const char type_character,
        const std::size_t identifier_offset) {
        if (type_character == '\0' || type_character == '$' ||
            type_character == '&' || type_character == '#') {
            return true;
        }
        set_error(
            "WFC0097",
            "type-declaration character requires an unsupported value type",
            identifier_offset);
        return false;
    }

    [[nodiscard]] std::size_t type_character_index(const char type_character) const {
        if (type_character == '$') {
            return Value{std::string{}}.index();
        }
        if (type_character == '#') {
            return Value{0.0}.index();
        }
        return Value{Integer{}}.index();
    }

    [[nodiscard]] bool type_character_matches(
        const Value& value,
        const char type_character,
        const std::size_t identifier_offset) {
        if (type_character == '\0') {
            return true;
        }
        if (!validate_type_character(type_character, identifier_offset)) {
            return false;
        }
        if (value.index() == type_character_index(type_character)) {
            return true;
        }
        set_error("WFC0016", "identifier type-declaration character mismatch", identifier_offset);
        return false;
    }

    [[nodiscard]] bool parse_statement() {
        skip_horizontal_whitespace();
        const auto statement_offset = offset_;
        if (consume_keyword("option")) {
            return parse_option_statement(statement_offset);
        }
        if (consume_keyword("rem")) {
            skip_comment();
            return true;
        }
        if (allow_declarations_) {
            module_body_started_ = true;
        }
        if (consume_keyword("if")) {
            return parse_if_statement();
        }
        if (consume_keyword("while")) {
            return parse_while_statement();
        }
        if (consume_keyword("do")) {
            return parse_do_statement();
        }
        if (consume_keyword("for")) {
            return parse_for_statement();
        }
        if (consume_keyword("select")) {
            return parse_select_statement();
        }
        if (consume_keyword("case")) {
            set_error("WFC0058", "unexpected Case", statement_offset);
            return false;
        }
        if (consume_keyword("next")) {
            set_error("WFC0048", "unexpected Next", statement_offset);
            return false;
        }
        if (consume_keyword("exit")) {
            return parse_exit_statement(statement_offset);
        }
        if (consume_keyword("loop")) {
            set_error("WFC0038", "unexpected Loop", statement_offset);
            return false;
        }
        if (consume_keyword("wend")) {
            set_error("WFC0033", "unexpected Wend", statement_offset);
            return false;
        }
        if (consume_keyword("print")) {
            return parse_print_statement();
        }
        if (consume_keyword("dim")) {
            if (!allow_declarations_) {
                set_error("WFC0027", "declarations are not supported in conditional blocks", statement_offset);
                return false;
            }
            return parse_declaration();
        }
        if (consume_keyword("const")) {
            if (!allow_declarations_) {
                set_error(
                    "WFC0063",
                    "constants are not supported in control-flow blocks",
                    statement_offset);
                return false;
            }
            return parse_constant_declaration();
        }

        const bool has_let = consume_keyword("let");
        if (has_let) {
            skip_horizontal_whitespace();
        }
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value()) {
            set_error("WFC0010", "expected statement", statement_offset);
            return false;
        }
        return parse_assignment(std::move(*identifier), type_character);
    }

    [[nodiscard]] bool parse_print_statement() {
        skip_horizontal_whitespace();
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        if (execute_) {
            if (has_output_line_) {
                output_.push_back('\n');
            }
            output_ += render(*value);
            has_output_line_ = true;
        }
        return true;
    }

    [[nodiscard]] bool parse_option_statement(const std::size_t statement_offset) {
        if (!allow_declarations_) {
            set_error("WFC0068", "Option directives are only valid at module level", statement_offset);
            return false;
        }
        skip_horizontal_whitespace();
        if (module_body_started_) {
            set_error("WFC0066", "Option directives must precede module statements", statement_offset);
            return false;
        }
        if (consume_keyword("explicit")) {
            if (option_explicit_) {
                set_error("WFC0067", "duplicate Option Explicit", statement_offset);
                return false;
            }
            option_explicit_ = true;
            return true;
        }
        if (consume_keyword("compare")) {
            if (option_compare_set_) {
                set_error("WFC0069", "duplicate Option Compare", statement_offset);
                return false;
            }
            skip_horizontal_whitespace();
            if (consume_keyword("binary")) {
                option_compare_text_ = false;
            } else if (consume_keyword("text")) {
                option_compare_text_ = true;
            } else {
                set_error("WFC0070", "expected Binary or Text after Option Compare", offset_);
                return false;
            }
            option_compare_set_ = true;
            return true;
        }
        set_error("WFC0065", "expected Explicit or Compare after Option", offset_);
        return false;
    }

    [[nodiscard]] bool parse_exit_statement(const std::size_t statement_offset) {
        skip_horizontal_whitespace();
        if (consume_keyword("do")) {
            if (do_depth_ == 0U) {
                set_error("WFC0042", "Exit Do is not inside a Do loop", statement_offset);
                return false;
            }
            if (execute_) {
                exit_do_requested_ = true;
            }
            return true;
        }
        if (consume_keyword("for")) {
            if (for_depth_ == 0U) {
                set_error("WFC0052", "Exit For is not inside a For loop", statement_offset);
                return false;
            }
            if (execute_) {
                exit_for_requested_ = true;
            }
            return true;
        }
        set_error("WFC0041", "expected Do or For after Exit", offset_);
        return false;
    }

    [[nodiscard]] bool control_exit_requested() const noexcept {
        return exit_do_requested_ || exit_for_requested_;
    }

    [[nodiscard]] bool parse_if_statement() {
        skip_horizontal_whitespace();
        const auto condition_offset = offset_;
        auto condition = parse_expression();
        if (!condition.has_value()) {
            return false;
        }
        const auto* boolean = std::get_if<bool>(&*condition);
        if (boolean == nullptr) {
            set_error("WFC0021", "If condition must be Boolean", condition_offset);
            return false;
        }

        skip_horizontal_whitespace();
        if (!consume_keyword("then")) {
            set_error("WFC0022", "expected Then", offset_);
            return false;
        }

        const bool enclosing_execution = execute_;
        skip_horizontal_whitespace();
        if (!at_end() && (current() == '\r' || current() == '\n' || current() == '\'')) {
            return parse_block_if_statement(enclosing_execution, *boolean);
        }
        execute_ = enclosing_execution && *boolean;
        if (!parse_inline_statement()) {
            execute_ = enclosing_execution;
            return false;
        }

        skip_horizontal_whitespace();
        if (consume_keyword("else")) {
            execute_ = enclosing_execution && !*boolean;
            if (!parse_inline_statement()) {
                execute_ = enclosing_execution;
                return false;
            }
        }
        execute_ = enclosing_execution;
        return true;
    }

    [[nodiscard]] bool parse_block_if_statement(
        const bool enclosing_execution,
        const bool condition) {
        if (!consume_block_line_end()) {
            return false;
        }

        bool has_else{};
        bool branch_selected = condition;
        execute_ = enclosing_execution && condition;
        while (true) {
            skip_program_leading_trivia();
            if (at_end()) {
                execute_ = enclosing_execution;
                set_error("WFC0024", "expected End If", offset_);
                return false;
            }
            if (consume_keyword("elseif")) {
                const auto elseif_offset = offset_ - 6U;
                if (has_else) {
                    execute_ = enclosing_execution;
                    set_error("WFC0030", "ElseIf is not permitted after Else", elseif_offset);
                    return false;
                }

                const bool evaluate_condition = enclosing_execution && !branch_selected;
                execute_ = evaluate_condition;
                skip_horizontal_whitespace();
                const auto condition_offset = offset_;
                auto elseif_condition = parse_expression();
                if (!elseif_condition.has_value()) {
                    execute_ = enclosing_execution;
                    return false;
                }
                const auto* elseif_boolean = std::get_if<bool>(&*elseif_condition);
                if (elseif_boolean == nullptr) {
                    execute_ = enclosing_execution;
                    set_error("WFC0028", "ElseIf condition must be Boolean", condition_offset);
                    return false;
                }

                skip_horizontal_whitespace();
                if (!consume_keyword("then")) {
                    execute_ = enclosing_execution;
                    set_error("WFC0029", "expected Then after ElseIf", offset_);
                    return false;
                }
                if (!consume_block_line_end()) {
                    execute_ = enclosing_execution;
                    return false;
                }

                const bool select_branch = !branch_selected && *elseif_boolean;
                branch_selected = branch_selected || *elseif_boolean;
                execute_ = enclosing_execution && select_branch;
                continue;
            }
            if (consume_keyword("else")) {
                if (has_else) {
                    execute_ = enclosing_execution;
                    set_error("WFC0026", "duplicate Else", offset_ - 4U);
                    return false;
                }
                has_else = true;
                if (!consume_block_line_end()) {
                    execute_ = enclosing_execution;
                    return false;
                }
                execute_ = enclosing_execution && !branch_selected;
                continue;
            }
            if (consume_keyword("end")) {
                skip_horizontal_whitespace();
                if (!consume_keyword("if")) {
                    execute_ = enclosing_execution;
                    set_error("WFC0025", "expected If after End", offset_);
                    return false;
                }
                execute_ = control_exit_requested() ? false : enclosing_execution;
                return true;
            }
            const bool enclosing_declaration_permission = allow_declarations_;
            allow_declarations_ = false;
            const bool parsed_statement = parse_statement();
            const bool consumed_statement_end = parsed_statement && consume_statement_end();
            allow_declarations_ = enclosing_declaration_permission;
            if (!parsed_statement || !consumed_statement_end) {
                execute_ = enclosing_execution;
                return false;
            }
            if (control_exit_requested()) {
                execute_ = false;
            }
        }
    }

    [[nodiscard]] bool parse_while_statement() {
        const bool enclosing_execution = execute_;
        skip_horizontal_whitespace();
        const auto condition_offset = offset_;
        auto condition = parse_expression();
        if (!condition.has_value()) {
            return false;
        }
        const auto* boolean = std::get_if<bool>(&*condition);
        if (boolean == nullptr) {
            set_error("WFC0031", "While condition must be Boolean", condition_offset);
            return false;
        }
        if (!consume_block_line_end()) {
            return false;
        }

        const auto body_offset = offset_;
        std::size_t continuation_offset{};
        if (!enclosing_execution || !*boolean) {
            execute_ = false;
            const bool parsed_body = parse_while_body(continuation_offset);
            execute_ = enclosing_execution;
            return parsed_body;
        }

        bool continue_loop = true;
        while (continue_loop) {
            offset_ = body_offset;
            execute_ = enclosing_execution;
            if (!parse_while_body(continuation_offset)) {
                execute_ = enclosing_execution;
                return false;
            }
            if (control_exit_requested()) {
                offset_ = continuation_offset;
                execute_ = false;
                return true;
            }

            offset_ = condition_offset;
            execute_ = enclosing_execution;
            auto next_condition = parse_expression();
            if (!next_condition.has_value()) {
                execute_ = enclosing_execution;
                return false;
            }
            const auto* next_boolean = std::get_if<bool>(&*next_condition);
            if (next_boolean == nullptr) {
                execute_ = enclosing_execution;
                set_error("WFC0031", "While condition must be Boolean", condition_offset);
                return false;
            }
            if (!consume_block_line_end()) {
                execute_ = enclosing_execution;
                return false;
            }
            continue_loop = *next_boolean;
        }

        offset_ = continuation_offset;
        execute_ = enclosing_execution;
        return true;
    }

    [[nodiscard]] bool parse_while_body(std::size_t& continuation_offset) {
        while (true) {
            skip_program_leading_trivia();
            if (at_end()) {
                set_error("WFC0032", "expected Wend", offset_);
                return false;
            }
            if (consume_keyword("wend")) {
                continuation_offset = offset_;
                return true;
            }

            const auto statement_offset = offset_;
            if (consume_keyword("dim")) {
                set_error(
                    "WFC0034",
                    "declarations are not supported in While blocks",
                    statement_offset);
                return false;
            }
            const bool enclosing_declaration_permission = allow_declarations_;
            allow_declarations_ = false;
            const bool parsed_statement = parse_statement();
            const bool consumed_statement_end = parsed_statement && consume_statement_end();
            allow_declarations_ = enclosing_declaration_permission;
            if (!parsed_statement || !consumed_statement_end) {
                return false;
            }
            if (control_exit_requested()) {
                execute_ = false;
            }
        }
    }

    [[nodiscard]] bool parse_do_statement() {
        const bool enclosing_execution = execute_;
        skip_horizontal_whitespace();
        if (!at_end() && (current() == '\r' || current() == '\n' || current() == '\'')) {
            return parse_posttest_do_statement(enclosing_execution);
        }
        bool until{};
        if (consume_keyword("while")) {
            until = false;
        } else if (consume_keyword("until")) {
            until = true;
        } else {
            set_error("WFC0036", "expected While or Until after Do", offset_);
            return false;
        }

        skip_horizontal_whitespace();
        const auto condition_offset = offset_;
        auto condition = parse_expression();
        if (!condition.has_value()) {
            return false;
        }
        const auto* boolean = std::get_if<bool>(&*condition);
        if (boolean == nullptr) {
            set_error("WFC0035", "Do condition must be Boolean", condition_offset);
            return false;
        }
        if (!consume_block_line_end()) {
            return false;
        }

        const auto body_offset = offset_;
        std::size_t continuation_offset{};
        bool continue_loop = until ? !*boolean : *boolean;
        if (!enclosing_execution || !continue_loop) {
            execute_ = false;
            ++do_depth_;
            const bool parsed_body = parse_do_body(continuation_offset);
            --do_depth_;
            execute_ = enclosing_execution;
            return parsed_body;
        }

        while (continue_loop) {
            offset_ = body_offset;
            execute_ = enclosing_execution;
            ++do_depth_;
            const bool parsed_body = parse_do_body(continuation_offset);
            --do_depth_;
            if (!parsed_body) {
                execute_ = enclosing_execution;
                return false;
            }
            if (exit_do_requested_) {
                exit_do_requested_ = false;
                offset_ = continuation_offset;
                execute_ = enclosing_execution;
                return true;
            }
            if (exit_for_requested_) {
                offset_ = continuation_offset;
                execute_ = false;
                return true;
            }

            offset_ = condition_offset;
            execute_ = enclosing_execution;
            auto next_condition = parse_expression();
            if (!next_condition.has_value()) {
                execute_ = enclosing_execution;
                return false;
            }
            const auto* next_boolean = std::get_if<bool>(&*next_condition);
            if (next_boolean == nullptr) {
                execute_ = enclosing_execution;
                set_error("WFC0035", "Do condition must be Boolean", condition_offset);
                return false;
            }
            if (!consume_block_line_end()) {
                execute_ = enclosing_execution;
                return false;
            }
            continue_loop = until ? !*next_boolean : *next_boolean;
        }

        offset_ = continuation_offset;
        execute_ = enclosing_execution;
        return true;
    }

    [[nodiscard]] bool parse_posttest_do_statement(const bool enclosing_execution) {
        if (!consume_block_line_end()) {
            return false;
        }

        const auto body_offset = offset_;
        std::size_t continuation_offset{};
        bool continue_loop{};
        do {
            offset_ = body_offset;
            execute_ = enclosing_execution;
            ++do_depth_;
            const bool parsed_body = parse_do_body(continuation_offset);
            --do_depth_;
            if (!parsed_body) {
                execute_ = enclosing_execution;
                return false;
            }
            const bool exit_do_requested = exit_do_requested_;
            const bool exit_for_requested = exit_for_requested_;

            skip_horizontal_whitespace();
            bool until{};
            if (consume_keyword("while")) {
                until = false;
            } else if (consume_keyword("until")) {
                until = true;
            } else {
                execute_ = enclosing_execution;
                set_error("WFC0040", "expected While or Until after Loop", offset_);
                return false;
            }

            skip_horizontal_whitespace();
            const auto condition_offset = offset_;
            auto condition = parse_expression();
            if (!condition.has_value()) {
                execute_ = enclosing_execution;
                return false;
            }
            const auto* boolean = std::get_if<bool>(&*condition);
            if (boolean == nullptr) {
                execute_ = enclosing_execution;
                set_error("WFC0035", "Do condition must be Boolean", condition_offset);
                return false;
            }
            continuation_offset = offset_;
            if (exit_do_requested) {
                exit_do_requested_ = false;
                continue_loop = false;
            } else if (exit_for_requested) {
                continue_loop = false;
            } else {
                continue_loop = enclosing_execution && (until ? !*boolean : *boolean);
            }
        } while (continue_loop);

        offset_ = continuation_offset;
        execute_ = enclosing_execution;
        return true;
    }

    [[nodiscard]] bool parse_do_body(std::size_t& continuation_offset) {
        while (true) {
            skip_program_leading_trivia();
            if (at_end()) {
                set_error("WFC0037", "expected Loop", offset_);
                return false;
            }
            if (consume_keyword("loop")) {
                continuation_offset = offset_;
                return true;
            }

            const auto statement_offset = offset_;
            if (consume_keyword("dim")) {
                set_error(
                    "WFC0039",
                    "declarations are not supported in Do blocks",
                    statement_offset);
                return false;
            }
            const bool enclosing_declaration_permission = allow_declarations_;
            allow_declarations_ = false;
            const bool parsed_statement = parse_statement();
            const bool consumed_statement_end = parsed_statement && consume_statement_end();
            allow_declarations_ = enclosing_declaration_permission;
            if (!parsed_statement || !consumed_statement_end) {
                return false;
            }
            if (control_exit_requested()) {
                execute_ = false;
            }
        }
    }

    [[nodiscard]] bool parse_for_statement() {
        const bool enclosing_execution = execute_;
        skip_horizontal_whitespace();
        const auto variable_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value()) {
            set_error("WFC0043", "expected For control variable", variable_offset);
            return false;
        }
        const auto variable = variables_.find(*identifier);
        if (variable == variables_.end()) {
            set_error("WFC0015", "undeclared variable", variable_offset);
            return false;
        }
        if (!type_character_matches(variable->second, type_character, variable_offset)) {
            return false;
        }
        if (!std::holds_alternative<Integer>(variable->second)) {
            set_error("WFC0045", "For control variable must be Long", variable_offset);
            return false;
        }

        skip_horizontal_whitespace();
        if (!consume('=')) {
            set_error("WFC0014", "expected assignment operator", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        auto start_value = parse_expression();
        if (!start_value.has_value()) {
            return false;
        }
        const auto* start = std::get_if<Integer>(&*start_value);
        if (start == nullptr) {
            set_error("WFC0045", "For bounds and Step must be Long", variable_offset);
            return false;
        }

        skip_horizontal_whitespace();
        if (!consume_keyword("to")) {
            set_error("WFC0044", "expected To", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        auto end_value = parse_expression();
        if (!end_value.has_value()) {
            return false;
        }
        const auto* end = std::get_if<Integer>(&*end_value);
        if (end == nullptr) {
            set_error("WFC0045", "For bounds and Step must be Long", variable_offset);
            return false;
        }

        Integer step = 1;
        skip_horizontal_whitespace();
        if (consume_keyword("step")) {
            skip_horizontal_whitespace();
            auto step_value = parse_expression();
            if (!step_value.has_value()) {
                return false;
            }
            const auto* parsed_step = std::get_if<Integer>(&*step_value);
            if (parsed_step == nullptr) {
                set_error("WFC0045", "For bounds and Step must be Long", variable_offset);
                return false;
            }
            step = *parsed_step;
        }
        if (step == 0) {
            set_error("WFC0047", "For Step cannot be zero", variable_offset);
            return false;
        }
        if (!consume_block_line_end()) {
            return false;
        }

        const auto body_offset = offset_;
        std::size_t continuation_offset{};
        Integer current_value = *start;
        const auto should_continue = [end = *end, step](const Integer current) {
            return step > 0 ? current <= end : current >= end;
        };
        if (enclosing_execution) {
            variable->second = current_value;
        }
        bool continue_loop = enclosing_execution && should_continue(current_value);
        if (!continue_loop) {
            execute_ = false;
            ++for_depth_;
            const bool parsed_body = parse_for_body(*identifier, continuation_offset);
            --for_depth_;
            execute_ = enclosing_execution;
            return parsed_body;
        }

        while (continue_loop) {
            variable->second = current_value;
            offset_ = body_offset;
            execute_ = enclosing_execution;
            ++for_depth_;
            const bool parsed_body = parse_for_body(*identifier, continuation_offset);
            --for_depth_;
            if (!parsed_body) {
                execute_ = enclosing_execution;
                return false;
            }
            if (exit_for_requested_) {
                exit_for_requested_ = false;
                offset_ = continuation_offset;
                execute_ = enclosing_execution;
                return true;
            }
            if (exit_do_requested_) {
                offset_ = continuation_offset;
                execute_ = false;
                return true;
            }

            const auto next = static_cast<std::int64_t>(current_value) + step;
            if (next < std::numeric_limits<Integer>::min() ||
                next > std::numeric_limits<Integer>::max()) {
                set_error("WFC0047", "For control variable overflow", variable_offset);
                execute_ = enclosing_execution;
                return false;
            }
            current_value = static_cast<Integer>(next);
            continue_loop = should_continue(current_value);
        }

        variable->second = current_value;
        offset_ = continuation_offset;
        execute_ = enclosing_execution;
        return true;
    }

    [[nodiscard]] bool parse_for_body(
        const std::string_view identifier,
        std::size_t& continuation_offset) {
        while (true) {
            skip_program_leading_trivia();
            if (at_end()) {
                set_error("WFC0046", "expected Next", offset_);
                return false;
            }
            if (consume_keyword("next")) {
                skip_horizontal_whitespace();
                const auto next_identifier_offset = offset_;
                char next_type_character{};
                auto next_identifier = parse_identifier(&next_type_character);
                if (next_identifier.has_value() && *next_identifier != identifier) {
                    set_error("WFC0049", "Next variable does not match For", next_identifier_offset);
                    return false;
                }
                if (next_identifier.has_value()) {
                    const auto variable = variables_.find(*next_identifier);
                    if (variable == variables_.end() ||
                        !type_character_matches(
                            variable->second, next_type_character, next_identifier_offset)) {
                        return false;
                    }
                }
                continuation_offset = offset_;
                return true;
            }

            const auto statement_offset = offset_;
            if (consume_keyword("dim")) {
                set_error("WFC0050", "declarations are not supported in For blocks", statement_offset);
                return false;
            }
            const bool enclosing_declaration_permission = allow_declarations_;
            allow_declarations_ = false;
            const bool parsed_statement = parse_statement();
            const bool consumed_statement_end = parsed_statement && consume_statement_end();
            allow_declarations_ = enclosing_declaration_permission;
            if (!parsed_statement || !consumed_statement_end) {
                return false;
            }
            if (control_exit_requested()) {
                execute_ = false;
            }
        }
    }

    [[nodiscard]] bool parse_select_statement() {
        const bool enclosing_execution = execute_;
        skip_horizontal_whitespace();
        if (!consume_keyword("case")) {
            set_error("WFC0054", "expected Case after Select", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        const auto selector_offset = offset_;
        auto selector = parse_expression();
        if (!selector.has_value()) {
            return false;
        }
        if (!consume_block_line_end()) {
            return false;
        }

        bool has_case{};
        bool has_else{};
        bool branch_selected{};
        execute_ = false;
        while (true) {
            skip_program_leading_trivia();
            if (at_end()) {
                execute_ = enclosing_execution;
                set_error("WFC0054", "expected End Select", offset_);
                return false;
            }
            if (consume_keyword("case")) {
                const auto case_offset = offset_ - 4U;
                skip_horizontal_whitespace();
                if (consume_keyword("else")) {
                    if (has_else) {
                        execute_ = enclosing_execution;
                        set_error("WFC0056", "duplicate Case Else", case_offset);
                        return false;
                    }
                    has_else = true;
                    has_case = true;
                    if (!consume_block_line_end()) {
                        execute_ = enclosing_execution;
                        return false;
                    }
                    execute_ = enclosing_execution && !branch_selected;
                    branch_selected = true;
                    continue;
                }
                if (has_else) {
                    execute_ = enclosing_execution;
                    set_error("WFC0057", "Case is not permitted after Case Else", case_offset);
                    return false;
                }

                bool case_matches{};
                while (true) {
                    const bool evaluate_case =
                        enclosing_execution && !branch_selected && !case_matches;
                    execute_ = evaluate_case;
                    const auto value_offset = offset_;
                    std::string relational_operator;
                    if (consume_keyword("is")) {
                        skip_horizontal_whitespace();
                        const auto operator_offset = offset_;
                        if (consume('<')) {
                            relational_operator = "<";
                            if (consume('=')) {
                                relational_operator = "<=";
                            } else if (consume('>')) {
                                relational_operator = "<>";
                            }
                        } else if (consume('>')) {
                            relational_operator = consume('=') ? ">=" : ">";
                        } else if (consume('=')) {
                            relational_operator = "=";
                        } else {
                            execute_ = enclosing_execution;
                            set_error(
                                "WFC0061",
                                "expected relational operator after Case Is",
                                operator_offset);
                            return false;
                        }
                        skip_horizontal_whitespace();
                    }
                    if (at_end() || current() == ',' || current() == '\r' ||
                        current() == '\n') {
                        execute_ = enclosing_execution;
                        set_error("WFC0059", "expected Case value", value_offset);
                        return false;
                    }
                    auto case_value = parse_expression();
                    if (!case_value.has_value()) {
                        execute_ = enclosing_execution;
                        return false;
                    }
                    if (case_value->index() != selector->index()) {
                        execute_ = enclosing_execution;
                        set_error("WFC0053", "Case value must match selector type", value_offset);
                        return false;
                    }
                    skip_horizontal_whitespace();
                    bool item_matches{};
                    if (!relational_operator.empty()) {
                        auto comparison = compare(
                            *selector,
                            *case_value,
                            relational_operator,
                            value_offset);
                        if (!comparison.has_value()) {
                            execute_ = enclosing_execution;
                            return false;
                        }
                        item_matches = std::get<bool>(*comparison);
                    } else if (consume_keyword("to")) {
                        skip_horizontal_whitespace();
                        const auto upper_offset = offset_;
                        auto upper_value = parse_expression();
                        if (!upper_value.has_value()) {
                            execute_ = enclosing_execution;
                            return false;
                        }
                        if (upper_value->index() != selector->index() ||
                            std::holds_alternative<bool>(*selector)) {
                            execute_ = enclosing_execution;
                            set_error(
                                "WFC0060",
                                "Case range requires same-type Long or String values",
                                upper_offset);
                            return false;
                        }
                        if (is_number(*selector)) {
                            const double selected_number = as_double(*selector);
                            item_matches =
                                as_double(*case_value) <= selected_number &&
                                selected_number <= as_double(*upper_value);
                        } else {
                            const auto& selected_string = std::get<std::string>(*selector);
                            item_matches =
                                compare_strings(
                                    std::get<std::string>(*case_value),
                                    selected_string) <= 0 &&
                                compare_strings(
                                    selected_string,
                                    std::get<std::string>(*upper_value)) <= 0;
                        }
                        skip_horizontal_whitespace();
                    } else {
                        item_matches = values_equal(*case_value, *selector);
                    }
                    case_matches = case_matches || item_matches;
                    if (!consume(',')) {
                        break;
                    }
                    skip_horizontal_whitespace();
                }
                if (!consume_block_line_end()) {
                    execute_ = enclosing_execution;
                    return false;
                }
                const bool select_branch = !branch_selected && case_matches;
                branch_selected = branch_selected || case_matches;
                has_case = true;
                execute_ = enclosing_execution && select_branch;
                continue;
            }
            if (consume_keyword("end")) {
                skip_horizontal_whitespace();
                if (!consume_keyword("select")) {
                    execute_ = enclosing_execution;
                    set_error("WFC0055", "expected Select after End", offset_);
                    return false;
                }
                execute_ = control_exit_requested() ? false : enclosing_execution;
                return true;
            }
            if (!has_case) {
                execute_ = enclosing_execution;
                set_error("WFC0054", "expected Case or End Select", selector_offset);
                return false;
            }

            const bool enclosing_declaration_permission = allow_declarations_;
            allow_declarations_ = false;
            const bool parsed_statement = parse_statement();
            const bool consumed_statement_end = parsed_statement && consume_statement_end();
            allow_declarations_ = enclosing_declaration_permission;
            if (!parsed_statement || !consumed_statement_end) {
                execute_ = enclosing_execution;
                return false;
            }
            if (control_exit_requested()) {
                execute_ = false;
            }
        }
    }

    [[nodiscard]] bool parse_inline_statement() {
        skip_horizontal_whitespace();
        const auto statement_offset = offset_;
        if (at_end() || current() == '\r' || current() == '\n' || current() == ':' ||
            consume_keyword("else")) {
            offset_ = statement_offset;
            set_error("WFC0023", "expected Print or assignment branch", statement_offset);
            return false;
        }
        if (consume_keyword("print")) {
            return parse_print_statement();
        }

        const bool has_let = consume_keyword("let");
        if (has_let) {
            skip_horizontal_whitespace();
        }
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value() || is_reserved_identifier(*identifier)) {
            set_error("WFC0023", "expected Print or assignment branch", statement_offset);
            return false;
        }
        return parse_assignment(std::move(*identifier), type_character);
    }

    [[nodiscard]] bool parse_declaration() {
        skip_horizontal_whitespace();
        const auto identifier_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value()) {
            set_error("WFC0011", "expected variable name", identifier_offset);
            return false;
        }
        if (is_reserved_identifier(*identifier)) {
            set_error("WFC0017", "reserved keyword cannot be a variable name", identifier_offset);
            return false;
        }
        if (!validate_type_character(type_character, identifier_offset)) {
            return false;
        }

        skip_horizontal_whitespace();
        Value initial_value;
        if (type_character != '\0') {
            if (consume_keyword("as")) {
                set_error("WFC0012", "type-declaration character cannot be combined with As", offset_);
                return false;
            }
            if (type_character == '$') {
                initial_value = std::string{};
            } else if (type_character == '#') {
                initial_value = 0.0;
            } else {
                initial_value = Integer{};
            }
        } else {
            if (!consume_keyword("as")) {
                set_error("WFC0012", "expected As Long, As Double, As String, or As Boolean", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            if (consume_keyword("long")) {
                initial_value = Integer{};
            } else if (consume_keyword("double")) {
                initial_value = 0.0;
            } else if (consume_keyword("string")) {
                initial_value = std::string{};
            } else if (consume_keyword("boolean")) {
                initial_value = false;
            } else {
                set_error("WFC0012", "expected As Long, As Double, As String, or As Boolean", offset_);
                return false;
            }
        }

        const auto [entry, inserted] = variables_.emplace(*identifier, std::move(initial_value));
        (void)entry;
        if (!inserted) {
            set_error("WFC0013", "duplicate variable declaration", identifier_offset);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool parse_constant_declaration() {
        skip_horizontal_whitespace();
        const auto identifier_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value()) {
            set_error("WFC0011", "expected constant name", identifier_offset);
            return false;
        }
        if (is_reserved_identifier(*identifier)) {
            set_error("WFC0017", "reserved keyword cannot be a constant name", identifier_offset);
            return false;
        }
        if (!validate_type_character(type_character, identifier_offset)) {
            return false;
        }
        if (variables_.contains(*identifier)) {
            set_error("WFC0013", "duplicate variable or constant declaration", identifier_offset);
            return false;
        }

        std::size_t expected_type{};
        skip_horizontal_whitespace();
        if (type_character != '\0') {
            if (consume_keyword("as")) {
                set_error("WFC0012", "type-declaration character cannot be combined with As", offset_);
                return false;
            }
            expected_type = type_character_index(type_character);
        } else {
            if (!consume_keyword("as")) {
                set_error("WFC0012", "expected As Long, As Double, As String, or As Boolean", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            if (consume_keyword("long")) {
                expected_type = Value{Integer{}}.index();
            } else if (consume_keyword("double")) {
                expected_type = Value{0.0}.index();
            } else if (consume_keyword("string")) {
                expected_type = Value{std::string{}}.index();
            } else if (consume_keyword("boolean")) {
                expected_type = Value{false}.index();
            } else {
                set_error("WFC0012", "expected As Long, As Double, As String, or As Boolean", offset_);
                return false;
            }
        }

        skip_horizontal_whitespace();
        if (!consume('=')) {
            set_error("WFC0014", "expected constant initializer", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        constant_expression_ = true;
        auto value = parse_expression();
        constant_expression_ = false;
        if (!value.has_value()) {
            return false;
        }
        if (expected_type == Value{0.0}.index() &&
            std::holds_alternative<Integer>(*value)) {
            *value = static_cast<double>(std::get<Integer>(*value));
        } else if (value->index() != expected_type) {
            set_error("WFC0016", "constant initializer type mismatch", identifier_offset);
            return false;
        }

        variables_.emplace(*identifier, std::move(*value));
        constants_.insert(std::move(*identifier));
        return true;
    }

    [[nodiscard]] bool parse_assignment(
        std::string identifier,
        const char type_character = '\0') {
        const auto identifier_offset = offset_ - identifier.size() -
            (type_character == '\0' ? 0U : 1U);
        const auto variable = variables_.find(identifier);
        if (variable == variables_.end()) {
            set_error("WFC0015", "undeclared variable", identifier_offset);
            return false;
        }
        if (!type_character_matches(variable->second, type_character, identifier_offset)) {
            return false;
        }
        if (constants_.contains(identifier)) {
            set_error("WFC0062", "cannot assign to constant", identifier_offset);
            return false;
        }

        skip_horizontal_whitespace();
        if (!consume('=')) {
            set_error("WFC0014", "expected assignment operator", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        if (std::holds_alternative<double>(variable->second) &&
            std::holds_alternative<Integer>(*value)) {
            *value = static_cast<double>(std::get<Integer>(*value));
        } else if (variable->second.index() != value->index()) {
            set_error("WFC0016", "assignment type mismatch", identifier_offset);
            return false;
        }
        if (execute_) {
            variable->second = std::move(*value);
        }
        return true;
    }

    [[nodiscard]] std::optional<Value> parse_expression() {
        return parse_implication();
    }

    [[nodiscard]] std::optional<Value> parse_implication() {
        auto left = parse_equivalence();
        if (!left.has_value()) {
            return std::nullopt;
        }
        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            if (!consume_keyword("imp")) {
                return left;
            }
            skip_horizontal_whitespace();
            auto right = parse_equivalence();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = logical_binary(*left, *right, 'I', operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_equivalence() {
        auto left = parse_exclusive_or();
        if (!left.has_value()) {
            return std::nullopt;
        }
        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            if (!consume_keyword("eqv")) {
                return left;
            }
            skip_horizontal_whitespace();
            auto right = parse_exclusive_or();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = logical_binary(*left, *right, 'E', operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_exclusive_or() {
        auto left = parse_or();
        if (!left.has_value()) {
            return std::nullopt;
        }
        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            if (!consume_keyword("xor")) {
                return left;
            }
            skip_horizontal_whitespace();
            auto right = parse_or();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = logical_binary(*left, *right, 'X', operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_or() {
        auto left = parse_and();
        if (!left.has_value()) {
            return std::nullopt;
        }
        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            if (!consume_keyword("or")) {
                return left;
            }
            skip_horizontal_whitespace();
            auto right = parse_and();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = logical_binary(*left, *right, 'O', operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_and() {
        auto left = parse_not();
        if (!left.has_value()) {
            return std::nullopt;
        }
        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            if (!consume_keyword("and")) {
                return left;
            }
            skip_horizontal_whitespace();
            auto right = parse_not();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = logical_binary(*left, *right, 'A', operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_not() {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        if (!consume_keyword("not")) {
            return parse_comparison();
        }
        skip_horizontal_whitespace();
        auto value = parse_not();
        if (!value.has_value()) {
            return std::nullopt;
        }
        const auto* boolean = require_boolean(*value, operator_offset);
        if (boolean == nullptr) {
            return std::nullopt;
        }
        return Value{!*boolean};
    }

    [[nodiscard]] std::optional<Value> parse_comparison() {
        auto left = parse_concatenation();
        if (!left.has_value()) {
            return std::nullopt;
        }

        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        std::string_view operation;
        if (consume('=')) {
            operation = "=";
        } else if (consume('<')) {
            if (consume('=')) {
                operation = "<=";
            } else if (consume('>')) {
                operation = "<>";
            } else {
                operation = "<";
            }
        } else if (consume('>')) {
            operation = consume('=') ? ">=" : ">";
        } else {
            return left;
        }

        skip_horizontal_whitespace();
        auto right = parse_concatenation();
        if (!right.has_value()) {
            return std::nullopt;
        }
        return compare(*left, *right, operation, operator_offset);
    }

    [[nodiscard]] std::optional<Value> parse_concatenation() {
        auto left = parse_additive();
        if (!left.has_value()) {
            return std::nullopt;
        }

        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            if (!consume('&')) {
                return left;
            }
            skip_horizontal_whitespace();
            auto right = parse_additive();
            if (!right.has_value()) {
                return std::nullopt;
            }
            if (std::holds_alternative<bool>(*left) ||
                std::holds_alternative<bool>(*right)) {
                set_error(
                    "WFC0020", "concatenation requires String or Long operands", operator_offset);
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
            left = numeric_binary(*left, *right, operation, operator_offset);
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
            bool integer_only = false;
            if (consume('*')) {
                operation = '*';
            } else if (consume('/')) {
                operation = '/';
            } else if (consume('\\')) {
                operation = '\\';
                integer_only = true;
            } else if (consume_keyword("mod")) {
                operation = '%';
                integer_only = true;
            } else {
                return left;
            }

            skip_horizontal_whitespace();
            auto right = parse_unary();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = integer_only
                ? integer_binary(*left, *right, operation, operator_offset)
                : numeric_binary(*left, *right, operation, operator_offset);
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
            if (!value.has_value()) {
                return std::nullopt;
            }
            if (!std::holds_alternative<double>(*value) &&
                require_integer(*value, operator_offset) == nullptr) {
                return std::nullopt;
            }
            return value;
        }
        if (consume('-')) {
            skip_horizontal_whitespace();
            if (!at_end() &&
                (std::isdigit(static_cast<unsigned char>(current())) != 0 ||
                 (current() == '.' &&
                  std::isdigit(static_cast<unsigned char>(peek(1))) != 0))) {
                return parse_negative_number();
            }

            auto value = parse_unary();
            if (!value.has_value()) {
                return std::nullopt;
            }
            if (const auto* number = std::get_if<double>(&*value)) {
                return execute_ ? Value{-*number} : Value{0.0};
            }
            const auto* integer = require_integer(*value, operator_offset);
            if (integer == nullptr) {
                return std::nullopt;
            }
            if (!execute_) {
                return Value{Integer{}};
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
        if (std::isdigit(static_cast<unsigned char>(current())) != 0 ||
            (current() == '.' &&
             std::isdigit(static_cast<unsigned char>(peek(1))) != 0)) {
            return parse_number();
        }
        if (consume_keyword("true")) {
            return Value{true};
        }
        if (consume_keyword("false")) {
            return Value{false};
        }
        if (consume('(')) {
            auto value = parse_expression();
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
            char type_character{};
            auto identifier = parse_identifier(&type_character);
            skip_horizontal_whitespace();
            if (!at_end() && current() == '(') {
                if (type_character != '\0') {
                    identifier->push_back(type_character);
                }
                return parse_function_call(*identifier, identifier_offset);
            }
            if (const auto constant = vba_constant_value(*identifier)) {
                Value value{*constant};
                if (!type_character_matches(value, type_character, identifier_offset)) {
                    return std::nullopt;
                }
                return value;
            }
            if (auto text = vba_string_constant(*identifier)) {
                Value value{std::move(*text)};
                if (!type_character_matches(value, type_character, identifier_offset)) {
                    return std::nullopt;
                }
                return value;
            }
            if (!allow_identifiers_) {
                set_error("WFC0002", "expected expression", identifier_offset);
                return std::nullopt;
            }
            const auto variable = variables_.find(*identifier);
            if (variable == variables_.end()) {
                set_error("WFC0015", "undeclared variable", identifier_offset);
                return std::nullopt;
            }
            if (!type_character_matches(variable->second, type_character, identifier_offset)) {
                return std::nullopt;
            }
            if (constant_expression_ && !constants_.contains(*identifier)) {
                set_error(
                    "WFC0064",
                    "constant initializer cannot reference a variable",
                    identifier_offset);
                return std::nullopt;
            }
            return variable->second;
        }

        set_error("WFC0002", "expected expression", offset_);
        return std::nullopt;
    }

    [[nodiscard]] std::optional<Value> parse_function_call(
        const std::string_view identifier,
        const std::size_t identifier_offset) {
        const bool is_len = identifier == "len" || identifier == "lenb";
        const bool is_lower = identifier == "lcase" || identifier == "lcase$";
        const bool is_upper = identifier == "ucase" || identifier == "ucase$";
        const bool is_left_trim = identifier == "ltrim" || identifier == "ltrim$";
        const bool is_right_trim = identifier == "rtrim" || identifier == "rtrim$";
        const bool is_trim = identifier == "trim" || identifier == "trim$";
        const bool is_left = identifier == "left" || identifier == "left$" ||
                             identifier == "leftb" || identifier == "leftb$";
        const bool is_right = identifier == "right" || identifier == "right$" ||
                              identifier == "rightb" || identifier == "rightb$";
        const bool is_mid = identifier == "mid" || identifier == "mid$" ||
                            identifier == "midb" || identifier == "midb$";
        const bool is_asc = identifier == "asc" || identifier == "ascb" ||
                            identifier == "ascw";
        const bool is_chr_b = identifier == "chrb" || identifier == "chrb$";
        const bool is_chr = identifier == "chr" || identifier == "chr$" ||
                            identifier == "chrw" || is_chr_b;
        const bool is_reverse = identifier == "strreverse";
        const bool is_space = identifier == "space" || identifier == "space$";
        const bool is_string = identifier == "string" || identifier == "string$";
        const bool is_instr = identifier == "instr" || identifier == "instrb";
        const bool is_instr_rev = identifier == "instrrev";
        const bool is_strcomp = identifier == "strcomp";
        const bool is_replace = identifier == "replace";
        const bool is_hex = identifier == "hex" || identifier == "hex$";
        const bool is_oct = identifier == "oct" || identifier == "oct$";
        const bool is_str = identifier == "str" || identifier == "str$";
        const bool is_val = identifier == "val";
        const bool is_abs = identifier == "abs";
        const bool is_sgn = identifier == "sgn";
        const bool is_cstr = identifier == "cstr";
        const bool is_clng = identifier == "clng";
        const bool is_cbool = identifier == "cbool";
        const bool is_cbyte = identifier == "cbyte";
        const bool is_cint = identifier == "cint";
        const bool is_cdbl = identifier == "cdbl";
        const bool is_csng = identifier == "csng";
        const bool is_cvar = identifier == "cvar";
        const bool is_macid = identifier == "macid";
        const bool is_error_message = identifier == "error" || identifier == "error$";
        const bool is_isnumeric = identifier == "isnumeric";
        const bool is_typename = identifier == "typename";
        const bool is_vartype = identifier == "vartype";
        const bool is_iif = identifier == "iif";
        const bool is_choose = identifier == "choose";
        const bool is_switch = identifier == "switch";
        const bool is_int = identifier == "int";
        const bool is_fix = identifier == "fix";
        const bool is_round = identifier == "round";
        const bool is_sqr = identifier == "sqr";
        const bool is_sin = identifier == "sin";
        const bool is_cos = identifier == "cos";
        const bool is_tan = identifier == "tan";
        const bool is_atn = identifier == "atn";
        const bool is_exp = identifier == "exp";
        const bool is_log = identifier == "log";
        const bool is_float_math =
            is_sqr || is_sin || is_cos || is_tan || is_atn || is_exp || is_log;
        const bool is_isarray = identifier == "isarray";
        const bool is_isobject = identifier == "isobject";
        const bool is_isnull = identifier == "isnull";
        const bool is_isempty = identifier == "isempty";
        const bool is_iserror = identifier == "iserror";
        const bool is_ismissing = identifier == "ismissing";
        const bool is_constant_false_predicate = is_isarray || is_isobject ||
                                                 is_isnull || is_isempty ||
                                                 is_iserror || is_ismissing;
        const bool is_qbcolor = identifier == "qbcolor";
        const bool is_rgb = identifier == "rgb";
        const bool is_strconv = identifier == "strconv";
        if (!is_len && !is_lower && !is_upper && !is_left_trim && !is_right_trim &&
            !is_trim && !is_left && !is_right && !is_mid && !is_asc && !is_chr &&
            !is_reverse && !is_space && !is_string && !is_instr && !is_strcomp &&
            !is_instr_rev && !is_replace && !is_hex && !is_oct && !is_str && !is_val &&
            !is_abs && !is_sgn && !is_cstr && !is_clng && !is_cbool && !is_cbyte &&
            !is_cint && !is_isnumeric && !is_typename && !is_vartype && !is_iif &&
            !is_choose && !is_switch && !is_int && !is_fix &&
            !is_constant_false_predicate && !is_qbcolor && !is_rgb && !is_strconv &&
            !is_round && !is_cdbl && !is_csng && !is_cvar && !is_macid &&
            !is_error_message && !is_float_math) {
            set_error("WFC0071", "unsupported function", identifier_offset);
            return std::nullopt;
        }
        if (constant_expression_) {
            set_error(
                "WFC0074",
                "constant initializer cannot call a function",
                identifier_offset);
            return std::nullopt;
        }

        advance();
        skip_horizontal_whitespace();
        std::vector<Value> arguments;
        if (!consume(')')) {
            while (true) {
                if (!at_end() && current() == ',') {
                    set_error("WFC0072", "function received an empty argument", offset_);
                    return std::nullopt;
                }
                auto argument = parse_expression();
                if (!argument.has_value()) {
                    return std::nullopt;
                }
                arguments.push_back(std::move(*argument));
                skip_horizontal_whitespace();
                if (consume(')')) {
                    break;
                }
                if (!consume(',')) {
                    set_error("WFC0005", "expected closing parenthesis", offset_);
                    return std::nullopt;
                }
                skip_horizontal_whitespace();
                if (consume(')')) {
                    set_error("WFC0072", "function received an empty argument", offset_ - 1U);
                    return std::nullopt;
                }
            }
        }

        bool valid_arity{};
        if (is_error_message) {
            valid_arity = arguments.size() <= 1U;
        } else if (is_mid) {
            valid_arity = arguments.size() == 2U || arguments.size() == 3U;
        } else if (is_instr || is_instr_rev) {
            valid_arity = arguments.size() >= 2U && arguments.size() <= 4U;
        } else if (is_replace) {
            valid_arity = arguments.size() >= 3U && arguments.size() <= 6U;
        } else if (is_strcomp) {
            valid_arity = arguments.size() == 2U || arguments.size() == 3U;
        } else if (is_round) {
            valid_arity = arguments.size() == 1U || arguments.size() == 2U;
        } else if (is_iif || is_rgb) {
            valid_arity = arguments.size() == 3U;
        } else if (is_choose) {
            valid_arity = arguments.size() >= 2U;
        } else if (is_switch) {
            valid_arity = arguments.size() >= 2U && arguments.size() % 2U == 0U;
        } else if (is_left || is_right || is_string || is_strconv) {
            valid_arity = arguments.size() == 2U;
        } else {
            valid_arity = arguments.size() == 1U;
        }
        if (!valid_arity) {
            set_error(
                "WFC0072",
                "function received the wrong number of arguments",
                identifier_offset);
            return std::nullopt;
        }

        if (is_chr) {
            const auto* character_code = std::get_if<Integer>(&arguments[0]);
            if (character_code == nullptr) {
                set_error("WFC0073", "Chr requires a Long argument", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            const Integer maximum = is_chr_b ? 255 : 127;
            if (*character_code < 0 || *character_code > maximum) {
                set_error(
                    "WFC0078",
                    is_chr_b ? "ChrB code must be in the byte range"
                             : "Chr code must be in the ASCII range",
                    identifier_offset);
                return std::nullopt;
            }
            return Value{std::string(1U, static_cast<char>(*character_code))};
        }

        if (is_abs || is_sgn) {
            if (!is_number(arguments[0])) {
                set_error(
                    "WFC0073",
                    is_abs ? "Abs requires a numeric argument" : "Sgn requires a numeric argument",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return is_abs ? arguments[0] : Value{Integer{}};
            }
            if (is_sgn) {
                const double number = as_double(arguments[0]);
                return Value{static_cast<Integer>((number > 0.0) - (number < 0.0))};
            }
            if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
                if (*integer == std::numeric_limits<Integer>::min()) {
                    set_error("WFC0009", "integer overflow", identifier_offset);
                    return std::nullopt;
                }
                return Value{*integer < 0 ? static_cast<Integer>(-*integer) : *integer};
            }
            return Value{std::abs(std::get<double>(arguments[0]))};
        }

        if (is_qbcolor) {
            const auto* color = std::get_if<Integer>(&arguments[0]);
            if (color == nullptr) {
                set_error("WFC0073", "QBColor requires a Long color index", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{Integer{}};
            }
            if (*color < 0 || *color > 15) {
                set_error("WFC0092", "QBColor index must be from 0 through 15", identifier_offset);
                return std::nullopt;
            }
            constexpr Integer colors[] = {
                0x000000, 0x800000, 0x008000, 0x808000,
                0x000080, 0x800080, 0x008080, 0xC0C0C0,
                0x808080, 0xFF0000, 0x00FF00, 0xFFFF00,
                0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF};
            return Value{colors[*color]};
        }

        if (is_rgb) {
            Integer component[3]{};
            for (std::size_t index = 0U; index < 3U; ++index) {
                const auto* value = std::get_if<Integer>(&arguments[index]);
                if (value == nullptr) {
                    set_error(
                        "WFC0073",
                        "RGB requires Long red, green, and blue components",
                        identifier_offset);
                    return std::nullopt;
                }
                component[index] = *value;
            }
            if (!execute_) {
                return Value{Integer{}};
            }
            for (Integer& value : component) {
                if (value < 0) {
                    set_error(
                        "WFC0091",
                        "RGB component must be non-negative",
                        identifier_offset);
                    return std::nullopt;
                }
                if (value > 255) {
                    value = 255;  // VB6 assumes any component above 255 is 255.
                }
            }
            return Value{static_cast<Integer>(
                component[0] + component[1] * 256 + component[2] * 65536)};
        }

        if (is_constant_false_predicate) {
            // The current value model contains only initialized Long, Boolean, and
            // String scalars: no arrays, object references, Null, Empty, error, or
            // missing-argument states exist, so each of these predicates is
            // constant False. True results are deferred with the array, object, and
            // Variant models that would introduce those states.
            return Value{false};
        }

        if (is_int || is_fix) {
            if (!is_number(arguments[0])) {
                set_error(
                    "WFC0073",
                    is_int ? "Int requires a numeric argument" : "Fix requires a numeric argument",
                    identifier_offset);
                return std::nullopt;
            }
            if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
                return Value{execute_ ? *integer : Integer{}};
            }
            if (!execute_) {
                return Value{0.0};
            }
            const double number = std::get<double>(arguments[0]);
            return Value{is_int ? std::floor(number) : std::trunc(number)};
        }

        if (is_float_math) {
            if (!is_number(arguments[0])) {
                set_error(
                    "WFC0073",
                    "math function requires a numeric argument",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{0.0};
            }
            const double argument = as_double(arguments[0]);
            double result{};
            if (is_sqr) {
                if (argument < 0.0) {
                    set_error("WFC0096", "Sqr argument must be non-negative", identifier_offset);
                    return std::nullopt;
                }
                result = std::sqrt(argument);
            } else if (is_log) {
                if (argument <= 0.0) {
                    set_error("WFC0096", "Log argument must be positive", identifier_offset);
                    return std::nullopt;
                }
                result = std::log(argument);
            } else if (is_sin) {
                result = std::sin(argument);
            } else if (is_cos) {
                result = std::cos(argument);
            } else if (is_tan) {
                result = std::tan(argument);
            } else if (is_atn) {
                result = std::atan(argument);
            } else {
                result = std::exp(argument);  // is_exp
            }
            if (!std::isfinite(result)) {
                set_error("WFC0009", "numeric overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{result};
        }

        if (is_round) {
            if (!is_number(arguments[0])) {
                set_error("WFC0073", "Round requires a numeric argument", identifier_offset);
                return std::nullopt;
            }
            Integer digits = 0;
            if (arguments.size() == 2U) {
                const auto* requested_digits = std::get_if<Integer>(&arguments[1]);
                if (requested_digits == nullptr) {
                    set_error(
                        "WFC0073",
                        "Round requires a Long digit count",
                        identifier_offset);
                    return std::nullopt;
                }
                digits = *requested_digits;
                if (execute_ && digits < 0) {
                    set_error(
                        "WFC0094",
                        "Round digit count must be non-negative",
                        identifier_offset);
                    return std::nullopt;
                }
            }
            if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
                return Value{execute_ ? *integer : Integer{}};
            }
            if (!execute_) {
                return Value{0.0};
            }
            const double number = std::get<double>(arguments[0]);
            if (digits >= std::numeric_limits<double>::max_digits10) {
                return Value{number};
            }
            const double scale = std::pow(10.0, static_cast<double>(digits));
            if (std::abs(number) > std::numeric_limits<double>::max() / scale) {
                return Value{number};
            }
            return Value{std::nearbyint(number * scale) / scale};
        }

        if (is_cstr) {
            return Value{execute_ ? render(arguments[0]) : std::string{}};
        }

        if (is_typename) {
            if (!execute_) {
                return Value{std::string{}};
            }
            if (std::holds_alternative<Integer>(arguments[0])) {
                return Value{std::string{"Long"}};
            }
            if (std::holds_alternative<double>(arguments[0])) {
                return Value{std::string{"Double"}};
            }
            if (std::holds_alternative<bool>(arguments[0])) {
                return Value{std::string{"Boolean"}};
            }
            return Value{std::string{"String"}};
        }

        if (is_vartype) {
            if (!execute_) {
                return Value{Integer{}};
            }
            if (std::holds_alternative<Integer>(arguments[0])) {
                return Value{Integer{3}};
            }
            if (std::holds_alternative<double>(arguments[0])) {
                return Value{Integer{5}};
            }
            if (std::holds_alternative<bool>(arguments[0])) {
                return Value{Integer{11}};
            }
            return Value{Integer{8}};
        }

        if (is_iif) {
            const auto* condition = std::get_if<bool>(&arguments[0]);
            if (condition == nullptr) {
                set_error("WFC0021", "IIf condition must be Boolean", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return arguments[1];
            }
            return *condition ? arguments[1] : arguments[2];
        }

        if (is_choose) {
            const auto* index = std::get_if<Integer>(&arguments[0]);
            if (index == nullptr) {
                set_error("WFC0073", "Choose requires a Long index", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return arguments[1];
            }
            const auto choice_count = static_cast<Integer>(arguments.size() - 1U);
            if (*index < 1 || *index > choice_count) {
                set_error("WFC0089", "Choose index is out of range", identifier_offset);
                return std::nullopt;
            }
            return arguments[static_cast<std::size_t>(*index)];
        }

        if (is_switch) {
            for (std::size_t pair = 0U; pair < arguments.size(); pair += 2U) {
                const auto* condition = std::get_if<bool>(&arguments[pair]);
                if (condition == nullptr) {
                    set_error(
                        "WFC0021",
                        "Switch expressions must be Boolean",
                        identifier_offset);
                    return std::nullopt;
                }
                if (execute_ && *condition) {
                    return arguments[pair + 1U];
                }
            }
            if (!execute_) {
                return arguments[1];
            }
            set_error("WFC0090", "Switch found no matching expression", identifier_offset);
            return std::nullopt;
        }

        if (is_isnumeric) {
            if (!execute_) {
                return Value{false};
            }
            if (std::holds_alternative<Integer>(arguments[0]) ||
                std::holds_alternative<bool>(arguments[0]) ||
                std::holds_alternative<double>(arguments[0])) {
                return Value{true};
            }

            const auto parsed = parse_numeric_string(std::get<std::string>(arguments[0]));
            return Value{parsed.status == NumericStringStatus::valid};
        }

        if (is_cbyte) {
            if (!is_number(arguments[0]) &&
                !std::holds_alternative<std::string>(arguments[0])) {
                set_error("WFC0073", "CByte requires a numeric argument", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{Integer{}};
            }

            if (const auto* number = std::get_if<Integer>(&arguments[0])) {
                if (*number < 0 || *number > 255) {
                    set_error("WFC0009", "integer overflow", identifier_offset);
                    return std::nullopt;
                }
                return Value{*number};
            }
            if (const auto* number = std::get_if<double>(&arguments[0])) {
                return round_double_to_long(*number, 0, 255, identifier_offset);
            }

            const auto parsed = parse_numeric_string(std::get<std::string>(arguments[0]));
            if (parsed.status == NumericStringStatus::out_of_range) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            if (parsed.status != NumericStringStatus::valid) {
                set_error("WFC0098", "CByte requires a numeric value", identifier_offset);
                return std::nullopt;
            }
            return round_double_to_long(parsed.value, 0, 255, identifier_offset);
        }

        if (is_error_message) {
            if (!execute_) {
                return Value{std::string{}};
            }
            if (arguments.empty()) {
                return Value{std::string{}};
            }
            const auto* number = std::get_if<Integer>(&arguments[0]);
            if (number == nullptr) {
                set_error("WFC0073", "Error requires a Long argument", identifier_offset);
                return std::nullopt;
            }
            if (*number < 0 || *number > 65535) {
                set_error("WFC0101", "Error number is outside the valid range", identifier_offset);
                return std::nullopt;
            }
            switch (*number) {
            case 0: return Value{std::string{}};
            case 5: return Value{std::string{"Invalid procedure call or argument"}};
            case 6: return Value{std::string{"Overflow"}};
            case 7: return Value{std::string{"Out of memory"}};
            case 9: return Value{std::string{"Subscript out of range"}};
            case 11: return Value{std::string{"Division by zero"}};
            case 13: return Value{std::string{"Type mismatch"}};
            case 28: return Value{std::string{"Out of stack space"}};
            case 53: return Value{std::string{"File not found"}};
            case 70: return Value{std::string{"Permission denied"}};
            case 76: return Value{std::string{"Path not found"}};
            case 91:
                return Value{std::string{"Object variable or With block variable not set"}};
            default:
                return Value{std::string{"Application-defined or object-defined error"}};
            }
        }

        if (is_cvar) {
            return arguments[0];
        }

        if (is_cdbl || is_csng) {
            double value{};
            if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
                value = static_cast<double>(*integer);
            } else if (const auto* number = std::get_if<double>(&arguments[0])) {
                value = *number;
            } else if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
                value = *boolean ? -1.0 : 0.0;
            } else {
                if (!execute_) {
                    return Value{0.0};
                }
                const auto parsed =
                    parse_numeric_string(std::get<std::string>(arguments[0]));
                if (parsed.status == NumericStringStatus::out_of_range) {
                    set_error("WFC0009", "numeric overflow", identifier_offset);
                    return std::nullopt;
                }
                if (parsed.status != NumericStringStatus::valid) {
                    set_error(
                        "WFC0095",
                        is_cdbl ? "CDbl requires a numeric value"
                                : "CSng requires a numeric value",
                        identifier_offset);
                    return std::nullopt;
                }
                value = parsed.value;
            }
            if (!execute_) {
                return Value{0.0};
            }
            // The evaluator has no distinct Single type; CSng narrows to float
            // precision and stores the result in the Double slot.
            if (is_csng) {
                const auto narrowed = static_cast<float>(value);
                if (!std::isfinite(narrowed)) {
                    set_error("WFC0009", "numeric overflow", identifier_offset);
                    return std::nullopt;
                }
                value = static_cast<double>(narrowed);
            }
            return Value{value};
        }

        if (is_cint) {
            constexpr Integer int_min{-32768};
            constexpr Integer int_max{32767};
            if (const auto* number = std::get_if<Integer>(&arguments[0])) {
                if (!execute_) {
                    return Value{Integer{}};
                }
                if (*number < int_min || *number > int_max) {
                    set_error("WFC0009", "integer overflow", identifier_offset);
                    return std::nullopt;
                }
                return Value{*number};
            }
            if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
                return Value{execute_ && *boolean ? Integer{-1} : Integer{0}};
            }
            if (const auto* number = std::get_if<double>(&arguments[0])) {
                if (!execute_) {
                    return Value{Integer{}};
                }
                return round_double_to_long(*number, int_min, int_max, identifier_offset);
            }
            if (!execute_) {
                return Value{Integer{}};
            }

            const auto parsed = parse_numeric_string(std::get<std::string>(arguments[0]));
            if (parsed.status == NumericStringStatus::out_of_range) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            if (parsed.status != NumericStringStatus::valid) {
                set_error("WFC0088", "CInt requires a numeric value", identifier_offset);
                return std::nullopt;
            }
            return round_double_to_long(parsed.value, int_min, int_max, identifier_offset);
        }

        if (is_clng) {
            if (const auto* number = std::get_if<Integer>(&arguments[0])) {
                return Value{execute_ ? *number : Integer{}};
            }
            if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
                return Value{execute_ && *boolean ? Integer{-1} : Integer{0}};
            }
            if (const auto* number = std::get_if<double>(&arguments[0])) {
                if (!execute_) {
                    return Value{Integer{}};
                }
                return round_double_to_long(
                    *number,
                    std::numeric_limits<Integer>::min(),
                    std::numeric_limits<Integer>::max(),
                    identifier_offset);
            }
            if (!execute_) {
                return Value{Integer{}};
            }

            const auto parsed = parse_numeric_string(std::get<std::string>(arguments[0]));
            if (parsed.status == NumericStringStatus::out_of_range) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            if (parsed.status != NumericStringStatus::valid) {
                set_error("WFC0086", "CLng requires a numeric value", identifier_offset);
                return std::nullopt;
            }
            return round_double_to_long(parsed.value, std::numeric_limits<Integer>::min(),
                std::numeric_limits<Integer>::max(), identifier_offset);
        }

        if (is_cbool) {
            if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
                return Value{execute_ && *boolean};
            }
            if (const auto* number = std::get_if<Integer>(&arguments[0])) {
                return Value{execute_ && *number != 0};
            }
            if (const auto* number = std::get_if<double>(&arguments[0])) {
                return Value{execute_ && *number != 0.0};
            }
            if (!execute_) {
                return Value{false};
            }

            const auto& text = std::get<std::string>(arguments[0]);
            std::size_t first{};
            std::size_t last = text.size();
            while (first < last &&
                   (text[first] == ' ' || text[first] == '\t' || text[first] == '\r' ||
                    text[first] == '\n')) {
                ++first;
            }
            while (last > first &&
                   (text[last - 1U] == ' ' || text[last - 1U] == '\t' ||
                    text[last - 1U] == '\r' || text[last - 1U] == '\n')) {
                --last;
            }
            std::string normalized;
            normalized.reserve(last - first);
            for (std::size_t index = first; index < last; ++index) {
                normalized.push_back(ascii_lower(text[index]));
            }
            if (normalized == "true") {
                return Value{true};
            }
            if (normalized == "false") {
                return Value{false};
            }

            const auto parsed = parse_numeric_string(normalized);
            if (parsed.status != NumericStringStatus::valid) {
                set_error("WFC0087", "CBool requires a Boolean or numeric value", identifier_offset);
                return std::nullopt;
            }
            return Value{parsed.value != 0.0};
        }

        if (is_space) {
            const auto* count = std::get_if<Integer>(&arguments[0]);
            if (count == nullptr) {
                set_error("WFC0073", "Space requires a Long argument", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            if (*count < 0) {
                set_error("WFC0075", "function length cannot be negative", identifier_offset);
                return std::nullopt;
            }
            return Value{std::string(static_cast<std::size_t>(*count), ' ')};
        }

        if (is_string) {
            const auto* count = std::get_if<Integer>(&arguments[0]);
            const bool fill_is_code = std::holds_alternative<Integer>(arguments[1]);
            const bool fill_is_text = std::holds_alternative<std::string>(arguments[1]);
            if (count == nullptr || (!fill_is_code && !fill_is_text)) {
                set_error(
                    "WFC0073",
                    "String requires a Long count and a Long or String fill",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            if (*count < 0) {
                set_error("WFC0075", "function length cannot be negative", identifier_offset);
                return std::nullopt;
            }
            char fill{};
            if (fill_is_code) {
                const auto code = std::get<Integer>(arguments[1]);
                if (code < 0 || code > 127) {
                    set_error(
                        "WFC0079",
                        "String fill code must be in the ASCII range",
                        identifier_offset);
                    return std::nullopt;
                }
                fill = static_cast<char>(code);
            } else {
                const auto& text = std::get<std::string>(arguments[1]);
                if (text.empty()) {
                    set_error(
                        "WFC0080",
                        "String requires a non-empty fill String",
                        identifier_offset);
                    return std::nullopt;
                }
                fill = text.front();
            }
            return Value{std::string(static_cast<std::size_t>(*count), fill)};
        }

        if (is_instr) {
            const bool has_start = arguments.size() >= 3U;
            const bool has_compare = arguments.size() == 4U;
            Integer start = 1;
            if (has_start) {
                const auto* start_argument = std::get_if<Integer>(&arguments[0]);
                if (start_argument == nullptr) {
                    set_error("WFC0073", "InStr start requires a Long argument", identifier_offset);
                    return std::nullopt;
                }
                start = *start_argument;
            }
            const std::size_t haystack_index = has_start ? 1U : 0U;
            const auto* haystack = std::get_if<std::string>(&arguments[haystack_index]);
            const auto* needle = std::get_if<std::string>(&arguments[haystack_index + 1U]);
            if (haystack == nullptr || needle == nullptr) {
                set_error("WFC0073", "InStr requires String arguments", identifier_offset);
                return std::nullopt;
            }
            const auto* compare_method =
                has_compare ? std::get_if<Integer>(&arguments[3]) : nullptr;
            if (has_compare && compare_method == nullptr) {
                set_error("WFC0073", "InStr compare requires a Long argument", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{Integer{}};
            }
            bool text_compare = option_compare_text_;
            if (compare_method != nullptr) {
                if (*compare_method < 0 || *compare_method > 1) {
                    set_error("WFC0081", "unsupported comparison method", identifier_offset);
                    return std::nullopt;
                }
                text_compare = *compare_method == 1;
            }
            if (start < 1) {
                set_error("WFC0076", "InStr start must be positive", identifier_offset);
                return std::nullopt;
            }
            const auto begin = static_cast<std::size_t>(start - 1);
            if (begin > haystack->size()) {
                return Value{Integer{0}};
            }
            if (needle->empty()) {
                return Value{begin < haystack->size() ? static_cast<Integer>(start) : Integer{0}};
            }
            std::size_t found{};
            if (text_compare) {
                std::string lowered_haystack = *haystack;
                std::string lowered_needle = *needle;
                for (char& character : lowered_haystack) {
                    character = ascii_lower(character);
                }
                for (char& character : lowered_needle) {
                    character = ascii_lower(character);
                }
                found = lowered_haystack.find(lowered_needle, begin);
            } else {
                found = haystack->find(*needle, begin);
            }
            if (found == std::string::npos) {
                return Value{Integer{0}};
            }
            return Value{static_cast<Integer>(found + 1U)};
        }

        if (is_instr_rev) {
            const auto* haystack = std::get_if<std::string>(&arguments[0]);
            const auto* needle = std::get_if<std::string>(&arguments[1]);
            const bool has_start = arguments.size() >= 3U;
            const bool has_compare = arguments.size() == 4U;
            const auto* start_argument =
                has_start ? std::get_if<Integer>(&arguments[2]) : nullptr;
            const auto* compare_method =
                has_compare ? std::get_if<Integer>(&arguments[3]) : nullptr;
            if (haystack == nullptr || needle == nullptr) {
                set_error("WFC0073", "InStrRev requires String arguments", identifier_offset);
                return std::nullopt;
            }
            if ((has_start && start_argument == nullptr) ||
                (has_compare && compare_method == nullptr)) {
                set_error(
                    "WFC0073",
                    "InStrRev start and compare require Long arguments",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{Integer{}};
            }

            const Integer start = start_argument == nullptr ? -1 : *start_argument;
            if (start < -1 || start == 0) {
                set_error("WFC0083", "InStrRev start must be -1 or positive", identifier_offset);
                return std::nullopt;
            }
            bool text_compare = option_compare_text_;
            if (compare_method != nullptr) {
                if (*compare_method < 0 || *compare_method > 1) {
                    set_error("WFC0081", "unsupported comparison method", identifier_offset);
                    return std::nullopt;
                }
                text_compare = *compare_method == 1;
            }
            if (haystack->empty()) {
                return Value{Integer{0}};
            }

            const auto effective_start =
                start == -1 ? haystack->size() : static_cast<std::size_t>(start);
            if (effective_start > haystack->size()) {
                return Value{Integer{0}};
            }
            if (needle->empty()) {
                return Value{static_cast<Integer>(effective_start)};
            }
            if (needle->size() > effective_start) {
                return Value{Integer{0}};
            }

            std::string searchable = *haystack;
            std::string sought = *needle;
            if (text_compare) {
                for (char& character : searchable) {
                    character = ascii_lower(character);
                }
                for (char& character : sought) {
                    character = ascii_lower(character);
                }
            }
            const auto latest_start = effective_start - sought.size();
            const auto found = searchable.rfind(sought, latest_start);
            return Value{
                found == std::string::npos ? Integer{0} : static_cast<Integer>(found + 1U)};
        }

        if (is_strcomp) {
            const auto* left = std::get_if<std::string>(&arguments[0]);
            const auto* right = std::get_if<std::string>(&arguments[1]);
            if (left == nullptr || right == nullptr) {
                set_error("WFC0073", "StrComp requires String arguments", identifier_offset);
                return std::nullopt;
            }
            const bool has_compare = arguments.size() == 3U;
            const auto* compare_method =
                has_compare ? std::get_if<Integer>(&arguments[2]) : nullptr;
            if (has_compare && compare_method == nullptr) {
                set_error("WFC0073", "StrComp compare requires a Long argument", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{Integer{}};
            }
            bool text_compare = option_compare_text_;
            if (compare_method != nullptr) {
                if (*compare_method < 0 || *compare_method > 1) {
                    set_error("WFC0081", "unsupported comparison method", identifier_offset);
                    return std::nullopt;
                }
                text_compare = *compare_method == 1;
            }
            int comparison{};
            if (text_compare) {
                std::string lowered_left = *left;
                std::string lowered_right = *right;
                for (char& character : lowered_left) {
                    character = ascii_lower(character);
                }
                for (char& character : lowered_right) {
                    character = ascii_lower(character);
                }
                comparison = lowered_left.compare(lowered_right);
            } else {
                comparison = left->compare(*right);
            }
            return Value{static_cast<Integer>((comparison > 0) - (comparison < 0))};
        }

        if (is_replace) {
            const auto* expression = std::get_if<std::string>(&arguments[0]);
            const auto* find = std::get_if<std::string>(&arguments[1]);
            const auto* replacement = std::get_if<std::string>(&arguments[2]);
            if (expression == nullptr || find == nullptr || replacement == nullptr) {
                set_error("WFC0073", "Replace requires String arguments", identifier_offset);
                return std::nullopt;
            }
            const bool has_start = arguments.size() >= 4U;
            const bool has_count = arguments.size() >= 5U;
            const bool has_compare = arguments.size() == 6U;
            const auto* start_argument =
                has_start ? std::get_if<Integer>(&arguments[3]) : nullptr;
            const auto* count_argument =
                has_count ? std::get_if<Integer>(&arguments[4]) : nullptr;
            const auto* compare_method =
                has_compare ? std::get_if<Integer>(&arguments[5]) : nullptr;
            if ((has_start && start_argument == nullptr) ||
                (has_count && count_argument == nullptr) ||
                (has_compare && compare_method == nullptr)) {
                set_error(
                    "WFC0073",
                    "Replace start, count, and compare require Long arguments",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            const Integer start = start_argument == nullptr ? 1 : *start_argument;
            const Integer count = count_argument == nullptr ? -1 : *count_argument;
            if (start < 1) {
                set_error("WFC0076", "Replace start must be positive", identifier_offset);
                return std::nullopt;
            }
            if (count < -1) {
                set_error("WFC0082", "Replace count must be -1 or non-negative", identifier_offset);
                return std::nullopt;
            }
            bool text_compare = option_compare_text_;
            if (compare_method != nullptr) {
                if (*compare_method < 0 || *compare_method > 1) {
                    set_error("WFC0081", "unsupported comparison method", identifier_offset);
                    return std::nullopt;
                }
                text_compare = *compare_method == 1;
            }

            const auto begin = static_cast<std::size_t>(start - 1);
            if (begin >= expression->size()) {
                return Value{std::string{}};
            }
            const std::string source = expression->substr(begin);
            if (find->empty() || count == 0) {
                return Value{source};
            }
            std::string haystack = source;
            std::string needle = *find;
            if (text_compare) {
                for (char& character : haystack) {
                    character = ascii_lower(character);
                }
                for (char& character : needle) {
                    character = ascii_lower(character);
                }
            }
            std::string result;
            std::size_t position{};
            Integer replacements{};
            while (true) {
                const std::size_t found = haystack.find(needle, position);
                if (found == std::string::npos || (count >= 0 && replacements >= count)) {
                    result.append(source, position, std::string::npos);
                    break;
                }
                result.append(source, position, found - position);
                result.append(*replacement);
                position = found + needle.size();
                ++replacements;
            }
            return Value{std::move(result)};
        }

        if (is_str) {
            if (!is_number(arguments[0])) {
                set_error("WFC0073", "Str requires a numeric argument", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            std::string digits = render(arguments[0]);
            if (as_double(arguments[0]) == 0.0) {
                digits = "0";
            }
            if (as_double(arguments[0]) >= 0.0) {
                digits.insert(digits.begin(), ' ');
            }
            return Value{std::move(digits)};
        }

        if (is_hex || is_oct) {
            if (!is_number(arguments[0]) &&
                !std::holds_alternative<std::string>(arguments[0])) {
                set_error(
                    "WFC0073",
                    is_hex ? "Hex requires a numeric argument" : "Oct requires a numeric argument",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            std::optional<Integer> number;
            if (const auto* string = std::get_if<std::string>(&arguments[0])) {
                const auto parsed = parse_numeric_string(*string);
                if (parsed.status == NumericStringStatus::out_of_range) {
                    set_error("WFC0009", "integer overflow", identifier_offset);
                    return std::nullopt;
                }
                if (parsed.status != NumericStringStatus::valid) {
                    set_error(
                        "WFC0099",
                        is_hex ? "Hex requires a numeric value" : "Oct requires a numeric value",
                        identifier_offset);
                    return std::nullopt;
                }
                const auto rounded = round_double_to_long(
                    parsed.value,
                    std::numeric_limits<Integer>::min(),
                    std::numeric_limits<Integer>::max(),
                    identifier_offset);
                if (!rounded.has_value()) {
                    return std::nullopt;
                }
                number = std::get<Integer>(*rounded);
            } else {
                number = coerce_long(arguments[0], identifier_offset);
            }
            if (!number.has_value()) {
                return std::nullopt;
            }
            auto magnitude = static_cast<std::uint32_t>(*number);
            if (magnitude == 0U) {
                return Value{std::string{"0"}};
            }
            const std::uint32_t radix = is_hex ? 16U : 8U;
            std::string digits;
            while (magnitude != 0U) {
                const auto value = static_cast<int>(magnitude % radix);
                digits.push_back(
                    value < 10 ? static_cast<char>('0' + value)
                               : static_cast<char>('A' + (value - 10)));
                magnitude /= radix;
            }
            std::reverse(digits.begin(), digits.end());
            return Value{std::move(digits)};
        }

        const auto* string = std::get_if<std::string>(&arguments[0]);
        if (string == nullptr) {
            set_error("WFC0073", "function requires a String argument", identifier_offset);
            return std::nullopt;
        }
        if (is_macid) {
            if (!execute_) {
                return Value{Integer{}};
            }
            if (string->size() != 4U) {
                set_error("WFC0100", "MacID requires exactly four bytes", identifier_offset);
                return std::nullopt;
            }
            std::uint32_t packed{};
            for (const unsigned char byte : *string) {
                packed = (packed << 8U) | byte;
            }
            const std::int64_t signed_value = packed >= 0x80000000U
                                                  ? static_cast<std::int64_t>(packed) - 0x100000000LL
                                                  : static_cast<std::int64_t>(packed);
            return Value{static_cast<Integer>(signed_value)};
        }
        if (is_len) {
            if (!execute_) {
                return Value{Integer{}};
            }
            if (string->size() > static_cast<std::size_t>(std::numeric_limits<Integer>::max())) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{static_cast<Integer>(string->size())};
        }

        if (is_asc) {
            if (!execute_) {
                return Value{Integer{}};
            }
            if (string->empty()) {
                set_error("WFC0077", "Asc requires a non-empty String", identifier_offset);
                return std::nullopt;
            }
            return Value{static_cast<Integer>(static_cast<unsigned char>(string->front()))};
        }

        if (is_strconv) {
            const auto* conversion = std::get_if<Integer>(&arguments[1]);
            if (conversion == nullptr) {
                set_error(
                    "WFC0073",
                    "StrConv requires a Long conversion argument",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            if (*conversion == 1) {  // vbUpperCase
                std::string result = *string;
                for (char& character : result) {
                    character = ascii_upper(character);
                }
                return Value{std::move(result)};
            }
            if (*conversion == 2) {  // vbLowerCase
                std::string result = *string;
                for (char& character : result) {
                    character = ascii_lower(character);
                }
                return Value{std::move(result)};
            }
            if (*conversion == 3) {  // vbProperCase
                std::string result = *string;
                bool word_start = true;
                for (char& character : result) {
                    const bool is_letter =
                        (character >= 'A' && character <= 'Z') ||
                        (character >= 'a' && character <= 'z');
                    if (is_letter) {
                        character = word_start ? ascii_upper(character)
                                               : ascii_lower(character);
                        word_start = false;
                    } else {
                        word_start = true;
                    }
                }
                return Value{std::move(result)};
            }
            set_error(
                "WFC0093",
                "StrConv conversion is not supported in the current model",
                identifier_offset);
            return std::nullopt;
        }

        if (is_val) {
            if (!execute_) {
                return Value{Integer{}};
            }
            std::string compact;
            compact.reserve(string->size());
            for (const char character : *string) {
                if (character != ' ' && character != '\t' && character != '\r' &&
                    character != '\n') {
                    compact.push_back(character);
                }
            }
            if (compact.empty()) {
                return Value{Integer{0}};
            }

            std::size_t digit_start{};
            if (compact[0] == '+' || compact[0] == '-') {
                digit_start = 1U;
            }
            if (digit_start + 1U < compact.size() && compact[digit_start] == '&' &&
                (compact[digit_start + 1U] == 'h' || compact[digit_start + 1U] == 'H' ||
                 compact[digit_start + 1U] == 'o' || compact[digit_start + 1U] == 'O')) {
                const int base = compact[digit_start + 1U] == 'h' ||
                                         compact[digit_start + 1U] == 'H'
                                     ? 16
                                     : 8;
                const std::size_t radix_start = digit_start + 2U;
                std::size_t radix_end = radix_start;
                const auto is_radix_digit = [base](const char character) {
                    if (character >= '0' && character <= '7') {
                        return true;
                    }
                    return base == 16 &&
                           ((character >= '8' && character <= '9') ||
                            (character >= 'a' && character <= 'f') ||
                            (character >= 'A' && character <= 'F'));
                };
                while (radix_end < compact.size() && is_radix_digit(compact[radix_end])) {
                    ++radix_end;
                }
                if (radix_end == radix_start) {
                    return Value{Integer{0}};
                }

                std::uint32_t magnitude{};
                const auto conversion = std::from_chars(
                    compact.data() + radix_start,
                    compact.data() + radix_end,
                    magnitude,
                    base);
                if (conversion.ec == std::errc::result_out_of_range) {
                    set_error("WFC0009", "integer overflow", identifier_offset);
                    return std::nullopt;
                }

                std::int64_t value = magnitude;
                if (magnitude >= 0x8000U && magnitude <= 0xFFFFU) {
                    value -= 0x10000LL;
                } else if (magnitude >= 0x80000000U) {
                    value -= 0x100000000LL;
                }
                if (digit_start == 1U && compact[0] == '-') {
                    value = -value;
                }
                if (value < std::numeric_limits<Integer>::min() ||
                    value > std::numeric_limits<Integer>::max()) {
                    set_error("WFC0009", "integer overflow", identifier_offset);
                    return std::nullopt;
                }
                return Value{static_cast<Integer>(value)};
            }
            const auto is_digit = [](const char character) {
                return std::isdigit(static_cast<unsigned char>(character)) != 0;
            };
            std::size_t pos = digit_start;
            bool had_int_digits = false;
            while (pos < compact.size() && is_digit(compact[pos])) {
                ++pos;
                had_int_digits = true;
            }
            const std::size_t int_end = pos;
            bool is_float = false;
            bool had_frac_digits = false;
            if (pos < compact.size() && compact[pos] == '.') {
                is_float = true;
                ++pos;
                while (pos < compact.size() && is_digit(compact[pos])) {
                    ++pos;
                    had_frac_digits = true;
                }
            }
            if ((had_int_digits || had_frac_digits) && pos < compact.size() &&
                (compact[pos] == 'e' || compact[pos] == 'E')) {
                std::size_t exponent = pos + 1U;
                if (exponent < compact.size() &&
                    (compact[exponent] == '+' || compact[exponent] == '-')) {
                    ++exponent;
                }
                if (exponent < compact.size() && is_digit(compact[exponent])) {
                    is_float = true;
                    pos = exponent;
                    while (pos < compact.size() && is_digit(compact[pos])) {
                        ++pos;
                    }
                }
            }
            if (!had_int_digits && !had_frac_digits) {
                return Value{Integer{0}};
            }

            const std::size_t conversion_start = compact[0] == '+' ? 1U : 0U;
            if (is_float) {
                double value{};
                const auto conversion = std::from_chars(
                    compact.data() + conversion_start,
                    compact.data() + pos,
                    value);
                if (conversion.ec == std::errc::result_out_of_range) {
                    set_error("WFC0009", "numeric overflow", identifier_offset);
                    return std::nullopt;
                }
                return Value{value};
            }
            Integer result{};
            const auto conversion = std::from_chars(
                compact.data() + conversion_start,
                compact.data() + int_end,
                result);
            if (conversion.ec == std::errc::result_out_of_range) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{result};
        }

        if (is_left || is_right) {
            const auto* length = std::get_if<Integer>(&arguments[1]);
            if (length == nullptr) {
                set_error("WFC0073", "function length requires a Long argument", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            if (*length < 0) {
                set_error("WFC0075", "function length cannot be negative", identifier_offset);
                return std::nullopt;
            }
            const auto requested = static_cast<std::size_t>(*length);
            const auto count = std::min(requested, string->size());
            return Value{
                is_left ? string->substr(0U, count) : string->substr(string->size() - count)};
        }

        if (is_mid) {
            const auto* start = std::get_if<Integer>(&arguments[1]);
            const auto* length =
                arguments.size() == 3U ? std::get_if<Integer>(&arguments[2]) : nullptr;
            if (start == nullptr || (arguments.size() == 3U && length == nullptr)) {
                set_error(
                    "WFC0073",
                    "Mid start and length require Long arguments",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            if (*start < 1) {
                set_error("WFC0076", "Mid start must be positive", identifier_offset);
                return std::nullopt;
            }
            if (length != nullptr && *length < 0) {
                set_error("WFC0075", "function length cannot be negative", identifier_offset);
                return std::nullopt;
            }

            const auto first = static_cast<std::size_t>(*start - 1);
            if (first >= string->size()) {
                return Value{std::string{}};
            }
            const auto available = string->size() - first;
            const auto count = length == nullptr
                                   ? available
                                   : std::min(static_cast<std::size_t>(*length), available);
            return Value{string->substr(first, count)};
        }

        if (!execute_) {
            return Value{std::string{}};
        }

        if (is_left_trim || is_right_trim || is_trim) {
            std::size_t first{};
            std::size_t last = string->size();
            if (is_left_trim || is_trim) {
                while (first < last && (*string)[first] == ' ') {
                    ++first;
                }
            }
            if (is_right_trim || is_trim) {
                while (last > first && (*string)[last - 1U] == ' ') {
                    --last;
                }
            }
            return Value{string->substr(first, last - first)};
        }

        if (is_reverse) {
            std::string result = *string;
            std::reverse(result.begin(), result.end());
            return Value{std::move(result)};
        }

        std::string result = *string;
        for (char& character : result) {
            character = is_lower ? ascii_lower(character) : ascii_upper(character);
        }
        return Value{std::move(result)};
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

    // Consume `digits[.digits][(e|E)[+|-]digits]` from the current position,
    // returning true when a fractional or exponent part made it a Double form.
    [[nodiscard]] bool lex_number_span() noexcept {
        const auto is_digit = [](const char character) {
            return std::isdigit(static_cast<unsigned char>(character)) != 0;
        };
        while (!at_end() && is_digit(current())) {
            advance();
        }
        bool is_float = false;
        if (!at_end() && current() == '.' && is_digit(peek(1))) {
            is_float = true;
            advance();
            while (!at_end() && is_digit(current())) {
                advance();
            }
        }
        if (!at_end() && (current() == 'e' || current() == 'E')) {
            const char sign = peek(1);
            const std::size_t digit_ahead = (sign == '+' || sign == '-') ? 2U : 1U;
            if (is_digit(peek(digit_ahead))) {
                is_float = true;
                advance();
                if (!at_end() && (current() == '+' || current() == '-')) {
                    advance();
                }
                while (!at_end() && is_digit(current())) {
                    advance();
                }
            }
        }
        return is_float;
    }

    [[nodiscard]] std::optional<Value> parse_double(
        const std::size_t start,
        const std::size_t end) {
        double value{};
        const auto conversion =
            std::from_chars(source_.data() + start, source_.data() + end, value);
        if (conversion.ec != std::errc{} ||
            conversion.ptr != source_.data() + end) {
            set_error("WFC0006", "numeric literal is malformed", start);
            return std::nullopt;
        }
        return Value{value};
    }

    [[nodiscard]] std::optional<Value> parse_number() {
        const auto start = offset_;
        const bool floating_form = lex_number_span();
        const auto end = offset_;
        char suffix = '\0';
        if (!at_end() && (current() == '#' || current() == '&')) {
            suffix = current();
            advance();
        }
        if (floating_form || suffix == '#') {
            if (suffix == '&') {
                set_error("WFC0006", "Long literal suffix requires an integer", start);
                return std::nullopt;
            }
            return parse_double(start, end);
        }
        Integer value{};
        const auto conversion =
            std::from_chars(source_.data() + start, source_.data() + end, value);
        if (conversion.ec == std::errc::result_out_of_range) {
            set_error("WFC0006", "integer literal out of range", start);
            return std::nullopt;
        }
        return Value{value};
    }

    [[nodiscard]] std::optional<Value> parse_negative_number() {
        const auto start = offset_;
        const bool floating_form = lex_number_span();
        const auto end = offset_;
        char suffix = '\0';
        if (!at_end() && (current() == '#' || current() == '&')) {
            suffix = current();
            advance();
        }
        if (floating_form || suffix == '#') {
            if (suffix == '&') {
                set_error("WFC0006", "Long literal suffix requires an integer", start);
                return std::nullopt;
            }
            auto value = parse_double(start, end);
            if (!value.has_value()) {
                return std::nullopt;
            }
            return Value{-std::get<double>(*value)};
        }

        std::uint64_t magnitude{};
        const auto conversion =
            std::from_chars(source_.data() + start, source_.data() + end, magnitude);
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

    // Round a Double to the nearest Long using banker's rounding (the default
    // IEEE round-to-nearest-even, matching VB6), rejecting out-of-range values.
    [[nodiscard]] std::optional<Value> round_double_to_long(
        const double number,
        const Integer minimum,
        const Integer maximum,
        const std::size_t offset) {
        const double rounded = std::nearbyint(number);
        if (!(rounded >= static_cast<double>(minimum) &&
              rounded <= static_cast<double>(maximum))) {
            set_error("WFC0009", "integer overflow", offset);
            return std::nullopt;
        }
        return Value{static_cast<Integer>(rounded)};
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

    [[nodiscard]] const bool* require_boolean(
        const Value& value,
        const std::size_t operator_offset) {
        const auto* boolean = std::get_if<bool>(&value);
        if (boolean == nullptr) {
            set_error("WFC0019", "logical operator requires Boolean operands", operator_offset);
        }
        return boolean;
    }

    [[nodiscard]] std::optional<Value> logical_binary(
        const Value& left,
        const Value& right,
        const char operation,
        const std::size_t operator_offset) {
        const auto* left_boolean = require_boolean(left, operator_offset);
        if (left_boolean == nullptr) {
            return std::nullopt;
        }
        const auto* right_boolean = require_boolean(right, operator_offset);
        if (right_boolean == nullptr) {
            return std::nullopt;
        }

        bool result{};
        switch (operation) {
        case 'A':
            result = *left_boolean && *right_boolean;
            break;
        case 'O':
            result = *left_boolean || *right_boolean;
            break;
        case 'X':
            result = *left_boolean != *right_boolean;
            break;
        case 'E':
            result = *left_boolean == *right_boolean;
            break;
        case 'I':
            result = !*left_boolean || *right_boolean;
            break;
        default:
            set_error("WFC0004", "unsupported operator", operator_offset);
            return std::nullopt;
        }
        return Value{result};
    }

    [[nodiscard]] int compare_strings(
        const std::string_view left,
        const std::string_view right) const noexcept {
        const auto common_size = std::min(left.size(), right.size());
        for (std::size_t index = 0; index < common_size; ++index) {
            const auto left_character = static_cast<unsigned char>(
                option_compare_text_ ? ascii_lower(left[index]) : left[index]);
            const auto right_character = static_cast<unsigned char>(
                option_compare_text_ ? ascii_lower(right[index]) : right[index]);
            if (left_character < right_character) {
                return -1;
            }
            if (left_character > right_character) {
                return 1;
            }
        }
        if (left.size() < right.size()) {
            return -1;
        }
        if (left.size() > right.size()) {
            return 1;
        }
        return 0;
    }

    [[nodiscard]] bool values_equal(const Value& left, const Value& right) const {
        if (const auto* left_string = std::get_if<std::string>(&left)) {
            return compare_strings(*left_string, std::get<std::string>(right)) == 0;
        }
        return left == right;
    }

    [[nodiscard]] std::optional<Value> compare(
        const Value& left,
        const Value& right,
        const std::string_view operation,
        const std::size_t operator_offset) {
        // Long and Double operands compare numerically, in either combination;
        // int32 widens to double exactly, so the ordering is precise.
        if (is_number(left) && is_number(right)) {
            const double left_value = as_double(left);
            const double right_value = as_double(right);
            if (operation == "=") {
                return Value{left_value == right_value};
            }
            if (operation == "<>") {
                return Value{left_value != right_value};
            }
            if (operation == "<") {
                return Value{left_value < right_value};
            }
            if (operation == "<=") {
                return Value{left_value <= right_value};
            }
            if (operation == ">") {
                return Value{left_value > right_value};
            }
            if (operation == ">=") {
                return Value{left_value >= right_value};
            }
            set_error("WFC0004", "unsupported operator", operator_offset);
            return std::nullopt;
        }

        if (left.index() != right.index()) {
            set_error("WFC0018", "comparison requires operands of the same type", operator_offset);
            return std::nullopt;
        }

        if (operation == "=") {
            return Value{values_equal(left, right)};
        }
        if (operation == "<>") {
            return Value{!values_equal(left, right)};
        }
        if (std::holds_alternative<bool>(left)) {
            set_error("WFC0018", "Boolean ordering is not supported", operator_offset);
            return std::nullopt;
        }

        bool less{};
        bool greater{};
        {
            const auto& left_string = std::get<std::string>(left);
            const auto& right_string = std::get<std::string>(right);
            const auto ordering = compare_strings(left_string, right_string);
            less = ordering < 0;
            greater = ordering > 0;
        }

        if (operation == "<") {
            return Value{less};
        }
        if (operation == "<=") {
            return Value{!greater};
        }
        if (operation == ">") {
            return Value{greater};
        }
        if (operation == ">=") {
            return Value{!less};
        }
        set_error("WFC0004", "unsupported operator", operator_offset);
        return std::nullopt;
    }

    // Evaluate `+`, `-`, `*`, and `/`. Two Long operands under `+`/`-`/`*` keep
    // the exact integer path (including overflow); `/` and any Double operand
    // promote to Double, matching VB6 numeric widening.
    [[nodiscard]] std::optional<Value> numeric_binary(
        const Value& left,
        const Value& right,
        const char operation,
        const std::size_t operator_offset) {
        if (operation != '/' &&
            std::holds_alternative<Integer>(left) &&
            std::holds_alternative<Integer>(right)) {
            return integer_binary(left, right, operation, operator_offset);
        }
        if (!is_number(left) || !is_number(right)) {
            if (require_integer(left, operator_offset) == nullptr) {
                return std::nullopt;
            }
            static_cast<void>(require_integer(right, operator_offset));
            return std::nullopt;
        }
        if (!execute_) {
            return Value{0.0};
        }

        const double left_value = as_double(left);
        const double right_value = as_double(right);
        double result{};
        switch (operation) {
        case '+':
            result = left_value + right_value;
            break;
        case '-':
            result = left_value - right_value;
            break;
        case '*':
            result = left_value * right_value;
            break;
        case '/':
            if (right_value == 0.0) {
                set_error("WFC0008", "division by zero", operator_offset);
                return std::nullopt;
            }
            result = left_value / right_value;
            break;
        default:
            set_error("WFC0004", "unsupported operator", operator_offset);
            return std::nullopt;
        }
        if (!std::isfinite(result)) {
            set_error("WFC0009", "numeric overflow", operator_offset);
            return std::nullopt;
        }
        return Value{result};
    }

    // Coerce a numeric operand to Long for the integer operators, rounding a
    // Double to the nearest even integer (VB6 banker's rounding). Non-numeric
    // operands and out-of-range magnitudes are rejected.
    [[nodiscard]] std::optional<Integer> coerce_long(
        const Value& value,
        const std::size_t operator_offset) {
        if (const auto* integer = std::get_if<Integer>(&value)) {
            return *integer;
        }
        if (const auto* number = std::get_if<double>(&value)) {
            const double rounded = std::nearbyint(*number);
            if (!(rounded >= static_cast<double>(std::numeric_limits<Integer>::min()) &&
                  rounded <= static_cast<double>(std::numeric_limits<Integer>::max()))) {
                set_error("WFC0009", "integer overflow", operator_offset);
                return std::nullopt;
            }
            return static_cast<Integer>(rounded);
        }
        set_error("WFC0007", "operator requires integer operands", operator_offset);
        return std::nullopt;
    }

    [[nodiscard]] std::optional<Value> integer_binary(
        const Value& left,
        const Value& right,
        const char operation,
        const std::size_t operator_offset) {
        const auto left_coerced = coerce_long(left, operator_offset);
        if (!left_coerced.has_value()) {
            return std::nullopt;
        }
        const auto right_coerced = coerce_long(right, operator_offset);
        if (!right_coerced.has_value()) {
            return std::nullopt;
        }
        const Integer left_integer = *left_coerced;
        const Integer right_integer = *right_coerced;
        if (!execute_) {
            return Value{Integer{}};
        }

        if ((operation == '\\' || operation == '%') && right_integer == 0) {
            set_error("WFC0008", "division by zero", operator_offset);
            return std::nullopt;
        }
        if ((operation == '\\' || operation == '%') &&
            left_integer == std::numeric_limits<Integer>::min() && right_integer == -1) {
            set_error("WFC0009", "integer overflow", operator_offset);
            return std::nullopt;
        }

        std::int64_t result{};
        switch (operation) {
        case '+':
            result = static_cast<std::int64_t>(left_integer) + right_integer;
            break;
        case '-':
            result = static_cast<std::int64_t>(left_integer) - right_integer;
            break;
        case '*':
            result = static_cast<std::int64_t>(left_integer) * right_integer;
            break;
        case '\\':
            result = left_integer / right_integer;
            break;
        case '%':
            result = left_integer % right_integer;
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
        if (const auto* number = std::get_if<double>(&value)) {
            char buffer[32];
            const auto result =
                std::to_chars(buffer, buffer + sizeof(buffer), *number);
            return std::string(buffer, result.ptr);
        }
        if (const auto* string = std::get_if<std::string>(&value)) {
            return *string;
        }
        return std::get<bool>(value) ? "True" : "False";
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
    std::unordered_set<std::string> constants_;
    std::string output_;
    bool has_output_line_{};
    bool execute_{true};
    bool allow_declarations_{true};
    bool constant_expression_{};
    bool module_body_started_{};
    bool option_explicit_{};
    bool option_compare_set_{};
    bool option_compare_text_{};
    std::size_t do_depth_{};
    bool exit_do_requested_{};
    std::size_t for_depth_{};
    bool exit_for_requested_{};
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
