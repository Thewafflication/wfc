#include "wfc/evaluator.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>

namespace {

using Integer = std::int32_t;
using Value = std::variant<Integer, std::string, bool>;

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
           identifier == "while" || identifier == "xor";
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
        auto identifier = parse_identifier();
        if (!identifier.has_value()) {
            set_error("WFC0010", "expected statement", statement_offset);
            return false;
        }
        return parse_assignment(std::move(*identifier));
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
        auto identifier = parse_identifier();
        if (!identifier.has_value()) {
            set_error("WFC0043", "expected For control variable", variable_offset);
            return false;
        }
        const auto variable = variables_.find(*identifier);
        if (variable == variables_.end()) {
            set_error("WFC0015", "undeclared variable", variable_offset);
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
                auto next_identifier = parse_identifier();
                if (next_identifier.has_value() && *next_identifier != identifier) {
                    set_error("WFC0049", "Next variable does not match For", next_identifier_offset);
                    return false;
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
                        if (const auto* selected_integer = std::get_if<Integer>(&*selector)) {
                            item_matches =
                                std::get<Integer>(*case_value) <= *selected_integer &&
                                *selected_integer <= std::get<Integer>(*upper_value);
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
        auto identifier = parse_identifier();
        if (!identifier.has_value() || is_reserved_identifier(*identifier)) {
            set_error("WFC0023", "expected Print or assignment branch", statement_offset);
            return false;
        }
        return parse_assignment(std::move(*identifier));
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
            set_error("WFC0012", "expected As Long, As String, or As Boolean", offset_);
            return false;
        }
        skip_horizontal_whitespace();

        Value initial_value;
        if (consume_keyword("long")) {
            initial_value = Integer{};
        } else if (consume_keyword("string")) {
            initial_value = std::string{};
        } else if (consume_keyword("boolean")) {
            initial_value = false;
        } else {
            set_error("WFC0012", "expected As Long, As String, or As Boolean", offset_);
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

    [[nodiscard]] bool parse_constant_declaration() {
        skip_horizontal_whitespace();
        const auto identifier_offset = offset_;
        auto identifier = parse_identifier();
        if (!identifier.has_value()) {
            set_error("WFC0011", "expected constant name", identifier_offset);
            return false;
        }
        if (is_reserved_identifier(*identifier)) {
            set_error("WFC0017", "reserved keyword cannot be a constant name", identifier_offset);
            return false;
        }
        if (variables_.contains(*identifier)) {
            set_error("WFC0013", "duplicate variable or constant declaration", identifier_offset);
            return false;
        }

        skip_horizontal_whitespace();
        if (!consume_keyword("as")) {
            set_error("WFC0012", "expected As Long, As String, or As Boolean", offset_);
            return false;
        }
        skip_horizontal_whitespace();

        std::size_t expected_type{};
        if (consume_keyword("long")) {
            expected_type = Value{Integer{}}.index();
        } else if (consume_keyword("string")) {
            expected_type = Value{std::string{}}.index();
        } else if (consume_keyword("boolean")) {
            expected_type = Value{false}.index();
        } else {
            set_error("WFC0012", "expected As Long, As String, or As Boolean", offset_);
            return false;
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
        if (value->index() != expected_type) {
            set_error("WFC0016", "constant initializer type mismatch", identifier_offset);
            return false;
        }

        variables_.emplace(*identifier, std::move(*value));
        constants_.insert(std::move(*identifier));
        return true;
    }

    [[nodiscard]] bool parse_assignment(std::string identifier) {
        const auto identifier_offset = offset_ - identifier.size();
        const auto variable = variables_.find(identifier);
        if (variable == variables_.end()) {
            set_error("WFC0015", "undeclared variable", identifier_offset);
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
        if (variable->second.index() != value->index()) {
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
        if (std::isdigit(static_cast<unsigned char>(current())) != 0) {
            return parse_integer();
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
            auto identifier = parse_identifier();
            skip_horizontal_whitespace();
            if (!at_end() && current() == '(') {
                return parse_function_call(*identifier, identifier_offset);
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
        if (identifier != "len") {
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
        if (consume(')')) {
            set_error("WFC0072", "Len requires one argument", identifier_offset);
            return std::nullopt;
        }
        auto argument = parse_expression();
        if (!argument.has_value()) {
            return std::nullopt;
        }
        skip_horizontal_whitespace();
        if (consume(',')) {
            set_error("WFC0072", "Len requires one argument", identifier_offset);
            return std::nullopt;
        }
        if (!consume(')')) {
            set_error("WFC0005", "expected closing parenthesis", offset_);
            return std::nullopt;
        }

        const auto* string = std::get_if<std::string>(&*argument);
        if (string == nullptr) {
            set_error("WFC0073", "Len requires a String argument", identifier_offset);
            return std::nullopt;
        }
        if (string->size() > static_cast<std::size_t>(std::numeric_limits<Integer>::max())) {
            set_error("WFC0009", "integer overflow", identifier_offset);
            return std::nullopt;
        }
        return Value{static_cast<Integer>(string->size())};
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
        if (const auto* left_integer = std::get_if<Integer>(&left)) {
            const auto right_integer = std::get<Integer>(right);
            less = *left_integer < right_integer;
            greater = *left_integer > right_integer;
        } else {
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
        if (!execute_) {
            return Value{Integer{}};
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
