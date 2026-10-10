// Interpreter: Statement dispatch and control flow (If, loops, Select, With,
// Exit, On Error). Internal to the WFC evaluator; not part of the public API.
// Split out of src/evaluator.cpp; see src/interpreter/README.md.

#include "interpreter.hpp"

namespace wfc::detail {

void Interpreter::skip_to_statement_end() noexcept {
    bool in_string = false;
    while (!at_end()) {
        const char c = current();
        if (c == '"') {
            in_string = !in_string;
        } else if (!in_string && (c == ':' || c == '\r' || c == '\n')) {
            return;
        } else if (!in_string && c == '\'') {
            skip_comment();
            return;
        }
        advance();
    }
}

bool Interpreter::recover_runtime_error(const std::size_t statement_start) {
    const Integer number = runtime_error_number();
    if (number == 0) {
        return false;
    }
    Scope& frame = scopes_.back();
    if (frame.on_error_mode == 0 || frame.in_error_handler) {
        return false;
    }
    if (std::string_view(error_.diagnostic).substr(0, 7) != "WFC0300") {
        err_number_ = number;
        err_source_.clear();
        err_description_ = vb_error_description(number);
    }
    skip_to_statement_end();
    frame.error_resume_next = offset_;
    frame.error_retry = statement_start;
    error_ = wfc::Evaluation{};
    if (frame.on_error_mode == 1) {
        execute_ = true;
        return true;
    }
    frame.in_error_handler = true;
    const bool in_procedure_context = in_procedure_body() &&
                                      current_procedure_def_ != nullptr &&
                                      frame.handler_depth == 0;
    if (!in_procedure_context) {
        jump_pending_ = true;
        jump_target_ = frame.on_error_label;
        set_error("WFC0999", "internal jump", statement_start);
        return false;
    }
    // Run the handler right here, in the failing statement's context, so
    // `Resume Next` / `Resume` continue inside any enclosing loop.
    const auto resume_point = offset_;
    const auto end_limit = current_procedure_def_->body_end;
    offset_ = frame.on_error_label;
    ++frame.handler_depth;
    frame.resume_signal = 0;
    bool completed = true;
    while (true) {
        skip_program_leading_trivia();
        if (offset_ >= end_limit || at_end()) {
            exit_sub_requested_ = true;
            exit_function_requested_ = true;
            offset_ = resume_point;
            break;
        }
        if (!parse_statement() || !consume_statement_end()) {
            completed = false;
            break;
        }
        if (frame.resume_signal != 0 || exit_sub_requested_ ||
            exit_function_requested_) {
            offset_ = resume_point;
            break;
        }
    }
    --frame.handler_depth;
    if (!completed) {
        return false;
    }
    if (frame.resume_signal == 2) {
        retry_statement_ = true;
    }
    frame.resume_signal = 0;
    execute_ = true;
    return true;
}

bool Interpreter::parse_statement() {
    while (true) {
        const auto start = offset_;
        if (parse_statement_once()) {
            if (!retry_statement_) {
                return true;
            }
            retry_statement_ = false;
            offset_ = start;
            continue;
        }
        return false;
    }
}

bool Interpreter::parse_statement_once() {
    const auto start = offset_;
    const bool entry_execute = execute_;
    if (parse_statement_core()) {
        return true;
    }
    if (!entry_execute || jump_pending_) {
        return false;
    }
    // Block statements (If/For/While/Do/Select/With) are not recovered
    // as a whole: their nested statements recover individually.
    {
        const auto saved = offset_;
        offset_ = start;
        skip_horizontal_whitespace();
        const bool is_block =
            consume_keyword("if") || consume_keyword("for") ||
            consume_keyword("while") || consume_keyword("do") ||
            consume_keyword("select") || consume_keyword("with");
        offset_ = saved;
        if (is_block) {
            return false;
        }
    }
    execute_ = entry_execute;
    return recover_runtime_error(start);
}

bool Interpreter::take_pending_jump() {
    if (!jump_pending_) {
        return false;
    }
    jump_pending_ = false;
    error_ = wfc::Evaluation{};
    offset_ = jump_target_;
    execute_ = true;
    exit_sub_requested_ = false;
    exit_function_requested_ = false;
    return true;
}

std::size_t Interpreter::find_label(const std::string& label) const {
    std::size_t begin = 0;
    std::size_t end = source_.size();
    if (in_procedure_body() && current_procedure_def_ != nullptr) {
        begin = current_procedure_def_->body_start;
        end = std::min(end, current_procedure_def_->body_end);
    }
    std::size_t position = begin;
    while (position < end) {
        std::size_t line_end = source_.find('\n', position);
        if (line_end == std::string_view::npos || line_end > end) {
            line_end = end;
        }
        std::size_t i = position;
        while (i < line_end && (source_[i] == ' ' || source_[i] == '\t')) {
            ++i;
        }
        if (!label.empty() &&
            std::isdigit(static_cast<unsigned char>(label[0])) != 0) {
            std::size_t k = i;
            while (k < line_end &&
                   std::isdigit(static_cast<unsigned char>(source_[k])) != 0) {
                ++k;
            }
            if (k > i && source_.substr(i, k - i) == label) {
                return i;
            }
            position = line_end + 1;
            continue;
        }
        std::size_t j = 0;
        while (j < label.size() && i + j < line_end &&
               ascii_lower(source_[i + j]) == label[j]) {
            ++j;
        }
        if (j == label.size() && i + j < line_end && source_[i + j] == ':' &&
            (i + j + 1 >= line_end || source_[i + j + 1] != '=')) {
            return i;
        }
        position = line_end + 1;
    }
    return std::string::npos;
}

std::optional<std::string> Interpreter::parse_label_name() {
    skip_horizontal_whitespace();
    if (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0) {
        std::string digits;
        while (!at_end() &&
               std::isdigit(static_cast<unsigned char>(current())) != 0) {
            digits.push_back(current());
            advance();
        }
        return digits;
    }
    return parse_identifier();
}

bool Interpreter::in_procedure_body() const noexcept {
    return scopes_.size() > 1U;
}

std::optional<bool> Interpreter::parse_error_handling_statement(
    const std::size_t statement_offset, const bool allow_label) {
    const auto start = offset_;
    if (consume_keyword("end") || consume_keyword("stop")) {
        skip_horizontal_whitespace();
        if (!at_statement_end()) {
            offset_ = start;
            return std::nullopt;
        }
        if (execute_) {
            end_requested_ = true;
            set_error("WFC0998", "program ended", statement_offset);
            return false;
        }
        return true;
    }
    if (consume_keyword("error")) {
        skip_horizontal_whitespace();
        if (at_statement_end()) {
            offset_ = start;
            return std::nullopt;
        }
        const auto number_offset = offset_;
        auto number = parse_expression();
        if (!number.has_value()) {
            return false;
        }
        if (!coerce_numeric_value(*number, Value{Integer{}}.index(),
                                  number_offset) ||
            !std::holds_alternative<Integer>(*number)) {
            set_error("WFC0073", "Error requires a Long number", number_offset);
            return false;
        }
        if (!execute_) {
            return true;
        }
        const Integer raised = std::get<Integer>(*number);
        if (raised < 1 || raised > 65535) {
            return raise_runtime(5, "Invalid procedure call or argument",
                                 statement_offset);
        }
        return raise_runtime(raised, vb_error_description(raised),
                             statement_offset);
    }
    if (consume_keyword("lset") || consume_keyword("rset")) {
        const bool right = ascii_lower(source_[start + 0]) == 'r';
        skip_horizontal_whitespace();
        const auto variable_offset = offset_;
        auto name = parse_identifier();
        if (!name.has_value()) {
            set_error("WFC0011", "expected variable name", variable_offset);
            return false;
        }
        const auto variable = find_variable(*name);
        if (variable.value == nullptr) {
            set_error("WFC0015", "undeclared variable", variable_offset);
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
        if (!execute_) {
            return true;
        }
        auto* const target = std::get_if<std::string>(variable.value);
        const auto* text = std::get_if<std::string>(&*value);
        if (target == nullptr || text == nullptr) {
            set_error("WFC0016", "LSet/RSet require String operands",
                      variable_offset);
            return false;
        }
        const std::size_t width = utf16_length(*target);
        std::string aligned = *text;
        if (!is_ascii_text(aligned)) {
            auto wide = to_utf16_units(aligned);
            wide.resize(std::min(wide.size(), width));
            aligned = from_utf16_units(wide);
        } else {
            aligned.resize(std::min(aligned.size(), width));
        }
        const std::size_t used = utf16_length(aligned);
        if (used < width) {
            const std::string padding(width - used, ' ');
            aligned = right ? padding + aligned : aligned + padding;
        }
        *target = std::move(aligned);
        return true;
    }
    if (consume_keyword("return")) {
        skip_horizontal_whitespace();
        if (!at_statement_end()) {
            offset_ = start;
            return std::nullopt;
        }
        if (!execute_) {
            return true;
        }
        Scope& frame = scopes_.back();
        if (frame.gosub_stack.empty()) {
            return raise_runtime(3, "Return without GoSub", statement_offset);
        }
        jump_pending_ = true;
        jump_target_ = frame.gosub_stack.back();
        frame.gosub_stack.pop_back();
        set_error("WFC0999", "internal jump", statement_offset);
        return false;
    }
    if (consume_keyword("gosub")) {
        skip_horizontal_whitespace();
        const auto label_offset = offset_;
        auto label = parse_label_name();
        if (!label.has_value()) {
            set_error("WFC0011", "expected label after GoSub", label_offset);
            return false;
        }
        if (!execute_) {
            return true;
        }
        const auto target = find_label(*label);
        if (target == std::string::npos) {
            set_error("WFC0301", "label not defined", label_offset);
            return false;
        }
        skip_to_statement_end();
        scopes_.back().gosub_stack.push_back(offset_);
        jump_pending_ = true;
        jump_target_ = target;
        set_error("WFC0999", "internal jump", statement_offset);
        return false;
    }
    if (consume_keyword("on")) {
        skip_horizontal_whitespace();
        if (!consume_keyword("error")) {
            // `On expr GoTo|GoSub label1, label2, ...`
            auto selector = parse_expression();
            if (!selector.has_value()) {
                return false;
            }
            skip_horizontal_whitespace();
            const bool is_gosub = consume_keyword("gosub");
            if (!is_gosub && !consume_keyword("goto")) {
                set_error("WFC0010", "expected GoTo or GoSub", offset_);
                return false;
            }
            if (!coerce_numeric_value(*selector, Value{Integer{}}.index(),
                                      statement_offset) ||
                !std::holds_alternative<Integer>(*selector)) {
                set_error("WFC0073", "On ... GoTo selector must be numeric",
                          statement_offset);
                return false;
            }
            const Integer choice = std::get<Integer>(*selector);
            std::vector<std::pair<std::string, std::size_t>> labels;
            while (true) {
                skip_horizontal_whitespace();
                const auto label_offset = offset_;
                auto label = parse_label_name();
                if (!label.has_value()) {
                    set_error("WFC0011", "expected label", label_offset);
                    return false;
                }
                labels.emplace_back(std::move(*label), label_offset);
                skip_horizontal_whitespace();
                if (!consume(',')) {
                    break;
                }
            }
            if (!execute_) {
                return true;
            }
            if (choice < 0 || choice > 255) {
                return raise_runtime(5, "Invalid procedure call or argument",
                                     statement_offset);
            }
            if (choice == 0 ||
                static_cast<std::size_t>(choice) > labels.size()) {
                return true;
            }
            const auto& chosen = labels[static_cast<std::size_t>(choice) - 1U];
            const auto target = find_label(chosen.first);
            if (target == std::string::npos) {
                set_error("WFC0301", "label not defined", chosen.second);
                return false;
            }
            if (is_gosub) {
                skip_to_statement_end();
                scopes_.back().gosub_stack.push_back(offset_);
            }
            jump_pending_ = true;
            jump_target_ = target;
            set_error("WFC0999", "internal jump", statement_offset);
            return false;
        }
        skip_horizontal_whitespace();
        Scope& frame = scopes_.back();
        if (consume_keyword("resume")) {
            skip_horizontal_whitespace();
            if (!consume_keyword("next")) {
                set_error("WFC0010", "expected Next after On Error Resume",
                          offset_);
                return false;
            }
            if (execute_) {
                frame.on_error_mode = 1;
                frame.in_error_handler = false;
            }
            return true;
        }
        if (!consume_keyword("goto")) {
            set_error("WFC0010", "expected GoTo or Resume after On Error",
                      offset_);
            return false;
        }
        skip_horizontal_whitespace();
        if (!at_end() && (current() == '0' || current() == '-')) {
            skip_to_statement_end();
            if (execute_) {
                frame.on_error_mode = 0;
                frame.in_error_handler = false;
            }
            return true;
        }
        const auto label_offset = offset_;
        auto label = parse_label_name();
        if (!label.has_value()) {
            set_error("WFC0011", "expected label after GoTo", label_offset);
            return false;
        }
        if (execute_) {
            const auto target = find_label(*label);
            if (target == std::string::npos) {
                set_error("WFC0301", "label not defined", label_offset);
                return false;
            }
            frame.on_error_mode = 2;
            frame.on_error_label = target;
            frame.in_error_handler = false;
        }
        return true;
    }
    if (consume_keyword("resume")) {
        skip_horizontal_whitespace();
        Scope& frame = scopes_.back();
        std::size_t target{};
        if (consume_keyword("next")) {
            target = frame.error_resume_next;
        } else if (at_end() || current() == '\r' || current() == '\n' ||
                   current() == ':' || current() == '\'') {
            target = frame.error_retry;
        } else {
            const auto label_offset = offset_;
            auto label = parse_label_name();
            if (!label.has_value()) {
                set_error("WFC0011", "expected label after Resume",
                          label_offset);
                return false;
            }
            target = execute_ ? find_label(*label) : 0;
            if (execute_ && target == std::string::npos) {
                set_error("WFC0301", "label not defined", label_offset);
                return false;
            }
        }
        if (execute_) {
            if (!frame.in_error_handler) {
                return raise_runtime(20, "Resume without error",
                                     statement_offset);
            }
            frame.in_error_handler = false;
            if (frame.handler_depth > 0 && (target == frame.error_resume_next ||
                                            target == frame.error_retry)) {
                frame.resume_signal = target == frame.error_retry &&
                                              target != frame.error_resume_next
                                          ? 2
                                          : 1;
                skip_to_statement_end();
                return true;
            }
            jump_pending_ = true;
            jump_target_ = target;
            set_error("WFC0999", "internal jump", statement_offset);
            return false;
        }
        return true;
    }
    if (consume_keyword("goto")) {
        skip_horizontal_whitespace();
        const auto label_offset = offset_;
        auto label = parse_label_name();
        if (!label.has_value()) {
            set_error("WFC0011", "expected label after GoTo", label_offset);
            return false;
        }
        if (execute_) {
            const auto target = find_label(*label);
            if (target == std::string::npos) {
                set_error("WFC0301", "label not defined", label_offset);
                return false;
            }
            jump_pending_ = true;
            jump_target_ = target;
            set_error("WFC0999", "internal jump", statement_offset);
            return false;
        }
        return true;
    }
    if (consume_keyword("err")) {
        skip_horizontal_whitespace();
        if (!consume('.')) {
            offset_ = start;
            return std::nullopt;
        }
        const auto member_offset = offset_;
        auto member = parse_identifier();
        if (member == "clear") {
            if (execute_) {
                err_number_ = 0;
                err_description_.clear();
                err_source_.clear();
            }
            return true;
        }
        if (member == "raise") {
            skip_horizontal_whitespace();
            const bool parenthesized = consume('(');
            skip_horizontal_whitespace();
            auto number = parse_expression();
            if (!number.has_value()) {
                return false;
            }
            if (!coerce_numeric_value(*number, Value{Integer{}}.index(),
                                      member_offset) ||
                !std::holds_alternative<Integer>(*number)) {
                set_error("WFC0073", "Err.Raise requires a Long number",
                          member_offset);
                return false;
            }
            std::string description;
            std::string source_text;
            bool has_description = false;
            for (int argument = 0; argument < 4; ++argument) {
                skip_horizontal_whitespace();
                if (!consume(',')) {
                    break;
                }
                skip_horizontal_whitespace();
                if (!at_end() && (current() == ',' || current() == ')')) {
                    continue;  // omitted argument
                }
                auto value = parse_expression();
                if (!value.has_value()) {
                    return false;
                }
                if (argument == 0) {
                    if (const auto* text = std::get_if<std::string>(&*value)) {
                        source_text = *text;
                    }
                }
                if (argument == 1) {
                    if (const auto* text = std::get_if<std::string>(&*value)) {
                        description = *text;
                        has_description = true;
                    }
                }
            }
            if (parenthesized) {
                skip_horizontal_whitespace();
                if (!consume(')')) {
                    set_error("WFC0005", "expected closing parenthesis",
                              offset_);
                    return false;
                }
            }
            if (execute_) {
                const Integer raised = std::get<Integer>(*number);
                if (raised == 0) {
                    return raise_runtime(
                        5, "Invalid procedure call or argument", member_offset);
                }
                err_number_ = raised;
                err_source_ = source_text;
                err_description_ = has_description
                                       ? description
                                       : vb_error_description(raised);
                set_error("WFC0300", err_description_, statement_offset);
                return false;
            }
            return true;
        }
        offset_ = start;
        return std::nullopt;
    }
    // `label:` (an identifier immediately followed by ':' that is not `:=`).
    {
        auto label = parse_identifier();
        if (allow_label && label.has_value() && !at_end() && current() == ':' &&
            !(offset_ + 1 < source_.size() && source_[offset_ + 1] == '=') &&
            !is_reserved_identifier(*label) && *label != "else") {
            return true;  // the ':' is left for the statement separator
        }
        offset_ = start;
    }
    return std::nullopt;
}

bool Interpreter::parse_statement_core() {
    variant_operand_seen_ = false;
    variant_string_seen_ = false;
    variant_number_seen_ = false;
    skip_horizontal_whitespace();
    if (!at_end() && current() == ':') {
        return true;  // an empty statement (`a = 1 : : b = 2`)
    }
    // A leading line number (`10  x = 1`) is a label that also feeds Erl.
    if (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0) {
        std::size_t line_start = offset_;
        while (line_start > 0 && (source_[line_start - 1] == ' ' ||
                                  source_[line_start - 1] == '\t')) {
            --line_start;
        }
        if (line_start == 0 || source_[line_start - 1] == '\n') {
            Integer number{};
            while (!at_end() &&
                   std::isdigit(static_cast<unsigned char>(current())) != 0) {
                number = static_cast<Integer>(std::min<long long>(
                    number * 10LL + (current() - '0'), 2147483647LL));
                advance();
            }
            if (execute_) {
                erl_ = number;
            }
            if (!at_end() && current() == ':' &&
                !(offset_ + 1 < source_.size() &&
                  source_[offset_ + 1] == '=')) {
                advance();
            }
            skip_horizontal_whitespace();
            if (at_statement_end()) {
                return true;
            }
        }
    }
    const auto statement_offset = offset_;
    // A statement that once fell through every keyword test is an
    // assignment or call; skip straight to that tail next time (the loop
    // bodies re-parse the same text over and over).
    if (identifier_statements_.contains(source_.data() + statement_offset)) {
        return parse_identifier_statement(statement_offset);
    }
    if (const auto handled = parse_error_handling_statement(statement_offset)) {
        return *handled;
    }
    if (const auto handled = parse_file_statement(statement_offset)) {
        return *handled;
    }
    if (const auto handled = parse_mid_statement(statement_offset)) {
        return *handled;
    }
    if (const auto handled = parse_binary_statement(statement_offset)) {
        return *handled;
    }
    if (consume_keyword("doevents")) {
        return true;
    }
    {
        // DefXxx statements were applied by scan_deftypes.
        const auto line_end = source_.find_first_of("\r\n", offset_);
        const auto length =
            (line_end == std::string_view::npos ? source_.size() : line_end) -
            statement_offset;
        if (apply_deftype_line(source_.substr(statement_offset, length))) {
            offset_ = statement_offset + length;
            return true;
        }
    }
    if (consume_keyword("beep")) {
        return true;
    }
    {
        const auto before_call = offset_;
        if (consume_keyword("callbyname")) {
            skip_horizontal_whitespace();
            const bool parenthesized = !at_end() && current() == '(';
            if (parenthesized) {
                advance();
            }
            std::vector<Value> values;
            skip_horizontal_whitespace();
            while (!at_statement_end() &&
                   !(parenthesized && current() == ')')) {
                auto value = parse_expression();
                if (!value.has_value()) {
                    return false;
                }
                values.push_back(std::move(*value));
                skip_horizontal_whitespace();
                if (!consume(',')) {
                    break;
                }
                skip_horizontal_whitespace();
            }
            if (parenthesized && !consume(')')) {
                set_error("WFC0005", "expected closing parenthesis", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            if (!at_statement_end()) {
                offset_ = before_call;  // `CallByName(...).Member` etc. is not
                                        // a statement
                set_error("WFC0010", "expected statement", statement_offset);
                return false;
            }
            return evaluate_misc_function("callbyname", values,
                                          statement_offset)
                .has_value();
        }
    }
    if (consume_keyword("msgbox") || consume_keyword("appactivate") ||
        consume_keyword("sendkeys")) {
        // The statement form evaluates and ignores its arguments.
        skip_horizontal_whitespace();
        while (!at_statement_end()) {
            if (!parse_expression().has_value()) {
                return false;
            }
            skip_horizontal_whitespace();
            if (!consume(',')) {
                break;
            }
            skip_horizontal_whitespace();
        }
        return true;
    }
    if (const auto shell_start = offset_; consume_keyword("shell")) {
        skip_horizontal_whitespace();
        if (at_end() || current() == '=' || current() == '.' ||
            at_statement_end()) {
            offset_ = shell_start;
        } else {
            std::vector<Value> values;
            while (!at_statement_end()) {
                auto value = parse_expression();
                if (!value.has_value()) {
                    return false;
                }
                values.push_back(std::move(*value));
                skip_horizontal_whitespace();
                if (!consume(',')) {
                    break;
                }
                skip_horizontal_whitespace();
            }
            if (!execute_) {
                return true;
            }
            return evaluate_misc_function("shell", values, statement_offset)
                .has_value();
        }
    }
    if (consume_keyword("savesetting") || consume_keyword("deletesetting") ||
        consume_keyword("setattr") || consume_keyword("chdrive")) {
        const std::string_view word = source_.substr(statement_offset, 4);
        const bool is_save =
            ascii_lower(word[0]) == 's' && ascii_lower(word[1]) == 'a';
        std::vector<Value> values;
        skip_horizontal_whitespace();
        while (!at_statement_end()) {
            auto value = parse_expression();
            if (!value.has_value()) {
                return false;
            }
            values.push_back(std::move(*value));
            skip_horizontal_whitespace();
            if (!consume(',')) {
                break;
            }
            skip_horizontal_whitespace();
        }
        if (execute_ && ascii_lower(word[0]) == 's' &&
            ascii_lower(word[1]) == 'e') {
            // SetAttr path, attributes: only the read-only bit has an effect.
            const auto* path =
                values.empty() ? nullptr : std::get_if<std::string>(&values[0]);
            const auto attributes =
                values.size() < 2U ? std::nullopt : whole_value(values[1]);
            if (path == nullptr || !attributes.has_value()) {
                set_error("WFC0073", "SetAttr requires a path and attributes",
                          statement_offset);
                return false;
            }
            std::error_code ec;
            if (!std::filesystem::exists(*path, ec)) {
                return raise_runtime(53, "File not found", statement_offset);
            }
            std::filesystem::permissions(
                *path,
                std::filesystem::perms::owner_write |
                    std::filesystem::perms::group_write |
                    std::filesystem::perms::others_write,
                (*attributes & 1) != 0 ? std::filesystem::perm_options::remove
                                       : std::filesystem::perm_options::add,
                ec);
            return true;
        }
        if (execute_ && values.size() >= 3U) {
            std::string key;
            bool strings = true;
            for (std::size_t i = 0; i < 3U; ++i) {
                const auto* part = std::get_if<std::string>(&values[i]);
                strings = strings && part != nullptr;
                if (part != nullptr) {
                    key += *part + "\x01";
                }
            }
            if (strings && is_save && values.size() == 4U) {
                if (const auto* text = std::get_if<std::string>(&values[3])) {
                    settings_[key] = *text;
                }
            } else if (strings && ascii_lower(word[0]) == 'd') {
                settings_.erase(key);
            }
        }
        return true;
    }
    {
        // File width / record locking: accepted, no effect.
        const auto before_lock = offset_;
        if (consume_keyword("width") || consume_keyword("lock") ||
            consume_keyword("unlock")) {
            skip_horizontal_whitespace();
            if (!at_end() && current() != '=' && current() != '(' &&
                current() != '.') {
                skip_to_statement_end();
                return true;
            }
            offset_ = before_lock;
        }
    }
    if (consume_keyword("reset")) {
        if (execute_) {
            for (auto& [number, file] : files_) {
                std::fclose(file.handle);
            }
            files_.clear();
        }
        return true;
    }
    {
        const auto before_declare = offset_;
        static_cast<void>(consume_keyword("public") ||
                          consume_keyword("private"));
        skip_horizontal_whitespace();
        if (consume_keyword("declare")) {
            skip_comment();  // handled by scan_procedures
            return true;
        }
        offset_ = before_declare;
    }
    if (consume_keyword("attribute")) {
        skip_comment();  // `Attribute X.VB_... = ...` lines inside bodies
        return true;
    }
    {
        const auto before_debug = offset_;
        if (consume_keyword("debug")) {
            skip_horizontal_whitespace();
            if (consume('.')) {
                skip_horizontal_whitespace();
                if (consume_keyword("print")) {
                    // Immediate-window output: collected in
                    // `Evaluation::debug_output`, not in `output`.
                    const bool saved_discard = discard_print_;
                    discard_print_ = true;
                    const bool ok = parse_print_statement();
                    discard_print_ = saved_discard;
                    return ok;
                }
                if (consume_keyword("assert")) {
                    // Stripped from compiled programs: the condition is not
                    // evaluated.
                    skip_comment_free_statement_text();
                    return true;
                }
            }
            offset_ = before_debug;
        }
    }
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
        skip_horizontal_whitespace();
        if (consume_keyword("each")) {
            return parse_for_each_statement();
        }
        return parse_for_statement();
    }
    if (consume_keyword("select")) {
        return parse_select_statement();
    }
    if (consume_keyword("with")) {
        return parse_with_statement(statement_offset);
    }
    {
        const auto pre_enum_offset = offset_;
        static_cast<void>(consume_keyword("public") ||
                          consume_keyword("private"));
        skip_horizontal_whitespace();
        if (consume_keyword("enum")) {
            return parse_enum_statement(statement_offset);
        }
        if (consume_keyword("type")) {
            return parse_type_statement_skip(statement_offset);
        }
        offset_ = pre_enum_offset;
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
    if (consume_keyword("sub") || consume_keyword("function")) {
        return parse_procedure_declaration_skip(statement_offset);
    }
    if (consume_keyword("property")) {
        return parse_property_declaration_skip(statement_offset);
    }
    {
        // REQ-0248: module-level `Public|Private|Global [Const] name ...`
        // declares a module variable/constant (visibility is not
        // enforced: this evaluator has a single standard module).
        const auto pre_visibility_offset = offset_;
        if (consume_keyword("public") || consume_keyword("private") ||
            consume_keyword("global")) {
            skip_horizontal_whitespace();
            if (consume_keyword("const")) {
                return parse_constant_declaration();
            }
            if (!at_end() && is_identifier_start(current())) {
                const auto probe = offset_;
                char probe_type_character{};
                const auto word = parse_identifier(&probe_type_character);
                offset_ = probe;
                if (word.has_value() && *word != "sub" && *word != "function" &&
                    *word != "property" && *word != "declare" &&
                    *word != "static" && *word != "enum" && *word != "type" &&
                    *word != "event" && *word != "sub" &&
                    procedures_.find(*word) == procedures_.end()) {
                    if (!allow_declarations_) {
                        set_error("WFC0027",
                                  "declarations are not supported in "
                                  "conditional blocks",
                                  statement_offset);
                        return false;
                    }
                    return parse_declaration();
                }
            }
            offset_ = pre_visibility_offset;
        }
    }
    {
        // `Public`/`Private Sub|Function Name(...)` -- the modifier is
        // parsed (matching real VB6 source) but not enforced at module
        // level (see scan_procedures); restore position if what
        // follows isn't actually a procedure declaration; consumed
        // here so `Public`/`Private` (both are reserved keywords) never
        // exists as a fully-unrecognized standalone statement.
        const auto pre_modifier_offset = offset_;
        const bool had_modifier = consume_keyword("public") ||
                                  consume_keyword("private") ||
                                  consume_keyword("friend");
        skip_horizontal_whitespace();
        const bool had_static = consume_keyword("static");
        if (had_modifier || had_static) {
            skip_horizontal_whitespace();
            if (consume_keyword("sub") || consume_keyword("function")) {
                return parse_procedure_declaration_skip(statement_offset);
            }
            if (consume_keyword("property")) {
                return parse_property_declaration_skip(statement_offset);
            }
            offset_ = pre_modifier_offset;
        }
    }
    if (consume_keyword("call")) {
        return parse_call_statement();
    }
    if (consume_keyword("raiseevent")) {
        return parse_raise_event_statement(statement_offset);
    }
    if (consume_keyword("print")) {
        return parse_print_statement();
    }
    if (consume_keyword("randomize")) {
        return parse_randomize_statement(statement_offset);
    }
    if (const auto rnd_start = offset_ - 0U; consume_keyword("rnd")) {
        // `Rnd -1` as a statement: the call's result is discarded.
        skip_horizontal_whitespace();
        if (at_end() || current() == '(' || current() == '=') {
            offset_ = rnd_start;
        } else {
            double argument = 1.0;
            const bool has_argument = current() != ':' && current() != '\r' &&
                                      current() != '\n' && current() != '\'';
            if (has_argument) {
                auto value = parse_expression();
                if (!value.has_value()) {
                    return false;
                }
                if (!is_number(*value) &&
                    !std::holds_alternative<bool>(*value)) {
                    if (!execute_) {
                        return true;
                    }
                    set_error("WFC0073", "Rnd requires a numeric argument",
                              statement_offset);
                    return false;
                }
                argument = std::holds_alternative<bool>(*value)
                               ? (std::get<bool>(*value) ? -1.0 : 0.0)
                               : as_double(*value);
            }
            if (execute_ && !(has_argument && argument == 0.0)) {
                rnd_state_ = (has_argument && argument < 0.0)
                                 ? seed_from_number(argument)
                                 : rnd_step(rnd_state_);
                rnd_last_value_ = static_cast<float>(rnd_value(rnd_state_));
            }
            return true;
        }
    }
    // REQ-0271: Dim/Static/Const are legal inside blocks; a declaration
    // executed again (loop iteration) is a no-op.
    if (consume_keyword("dim")) {
        if (current_procedure_def_ != nullptr &&
            current_procedure_def_->static_locals) {
            return parse_static_declaration(statement_offset);
        }
        return parse_declaration();
    }
    if (consume_keyword("static")) {
        return parse_static_declaration(statement_offset);
    }
    if (consume_keyword("const")) {
        return parse_constant_declaration();
    }
    if (consume_keyword("redim")) {
        // Unlike Dim/Static/Const, ReDim is an executable statement (it
        // resizes an already-declared dynamic array), not a
        // declaration, so it is not gated by allow_declarations_ --
        // real VB6 permits it inside a conditional block.
        return parse_redim_statement();
    }
    if (consume_keyword("erase")) {
        return parse_erase_statement();
    }
    if (consume_keyword("set")) {
        return parse_set_statement();
    }

    offset_ = statement_offset;
    identifier_statements_.insert(source_.data() + statement_offset);
    return parse_identifier_statement(statement_offset);
}

bool Interpreter::parse_identifier_statement(
    const std::size_t statement_offset) {
    offset_ = statement_offset;
    const bool has_let = consume_keyword("let");
    if (has_let) {
        skip_horizontal_whitespace();
    }
    const auto identifier_offset = offset_;
    char type_character{};
    auto identifier = parse_identifier(&type_character);
    if (!identifier.has_value()) {
        set_error("WFC0010", "expected statement", statement_offset);
        return false;
    }
    // A bare `Name` statement (no `Call`, zero arguments; REQ-0217)
    // invokes a module-level or class-sibling Sub/Function, discarding
    // any Function result -- checked only when `Name` is not a
    // variable and nothing else follows it on this statement (no `=`,
    // no `(`, no argument list), so this never competes with an
    // ordinary assignment or array-element write. A bare `Name arg1,
    // arg2` (with arguments) remains unsupported, avoiding the classic
    // ambiguity that form has with other statement shapes; see
    // REQ-0217's Scope.
    if (const auto handled = parse_bare_call(*identifier, identifier_offset,
                                             type_character, has_let)) {
        return *handled;
    }
    return parse_assignment_or_array_element(std::move(*identifier),
                                             type_character);
}

bool Interpreter::at_statement_end() const noexcept {
    return at_end() || current() == '\r' || current() == '\n' ||
           current() == ':' || current() == '\'';
}

bool Interpreter::parse_print_statement() {
    if (!allow_identifiers_) {
        // The expression-only `Print <expression>` entry point keeps its
        // original single-expression grammar.
        skip_horizontal_whitespace();
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        if (execute_) {
            if (has_output_line_) {
                output_.push_back('\n');
            }
            output_ += show_byte_halves(render(*value));
            has_output_line_ = true;
        }
        return true;
    }
    Integer file_number{};
    bool to_file{};
    if (!parse_file_number_prefix(file_number, to_file)) {
        return false;
    }
    std::string text;
    bool newline = true;
    while (true) {
        skip_horizontal_whitespace();
        if (at_statement_end()) {
            break;
        }
        {
            const auto before_else = offset_;
            if (consume_keyword("else")) {
                offset_ = before_else;
                break;
            }
        }
        if (current() == ';') {
            advance();
            newline = false;
            continue;
        }
        if (current() == ',') {
            advance();
            text.append(14U - utf16_length(text) % 14U, ' ');
            newline = false;
            continue;
        }
        const auto save = offset_;
        const bool is_spc = consume_keyword("spc");
        const bool is_tab = !is_spc && consume_keyword("tab");
        if (is_spc || is_tab) {
            skip_horizontal_whitespace();
            if (consume('(')) {
                skip_horizontal_whitespace();
                auto amount = parse_expression();
                if (!amount.has_value()) {
                    return false;
                }
                skip_horizontal_whitespace();
                if (!consume(')')) {
                    set_error("WFC0005", "expected closing parenthesis",
                              offset_);
                    return false;
                }
                const auto count_value = whole_value(*amount);
                const Integer* const count =
                    count_value ? &*count_value : nullptr;
                if (count == nullptr) {
                    set_error("WFC0073", "Spc/Tab requires a Long argument",
                              offset_);
                    return false;
                }
                if (execute_ && *count > 0) {
                    if (is_spc) {
                        text.append(static_cast<std::size_t>(*count), ' ');
                    } else if (const auto column = utf16_length(text);
                               static_cast<std::size_t>(*count - 1) > column) {
                        text.append(
                            static_cast<std::size_t>(*count - 1) - column, ' ');
                    }
                }
                newline = true;
                continue;
            }
            offset_ = save;
        }
        auto value = parse_expression();
        if (!value.has_value() || !resolve_default_value(*value, offset_)) {
            return false;
        }
        if (execute_) {
            if (std::holds_alternative<Null>(*value)) {
                text += "Null";
            } else if (vb_number_spacing_ && is_number(*value) &&
                       !std::holds_alternative<DateValue>(*value)) {
                // VB6 reserves a sign position before a number and adds a
                // trailing space.
                const std::string digits = render(*value);
                if (digits.empty() || digits.front() != '-') {
                    text.push_back(' ');
                }
                text += digits;
                text.push_back(' ');
            } else {
                text += show_byte_halves(render(*value));
            }
        }
        newline = true;
    }
    if (!execute_) {
        return true;
    }
    if (discard_print_) {
        // `Debug.Print`: the Immediate window; kept apart from the output.
        debug_output_ += text;
        if (newline) {
            debug_output_.push_back('\n');
        }
        return true;
    }
    if (to_file) {
        return write_to_file(file_number, newline ? text + "\r\n" : text,
                             offset_);
    }
    if (has_output_line_ && !output_line_open_) {
        output_.push_back('\n');
    }
    output_ += text;
    has_output_line_ = true;
    output_line_open_ = !newline;
    return true;
}

bool Interpreter::parse_randomize_statement(
    const std::size_t statement_offset) {
    skip_horizontal_whitespace();
    if (at_end() || current() == ':' || current() == '\r' ||
        current() == '\n' || current() == '\'') {
        if (execute_) {
            rnd_state_ = seed_from_number(entropy_seed());
        }
        return true;
    }
    auto value = parse_expression();
    if (!value.has_value()) {
        return false;
    }
    if (!is_number(*value) && !std::holds_alternative<bool>(*value)) {
        if (!execute_) {
            return true;
        }
        set_error("WFC0073", "Randomize requires a numeric seed",
                  statement_offset);
        return false;
    }
    if (execute_) {
        const double seed = std::holds_alternative<bool>(*value)
                                ? (std::get<bool>(*value) ? -1.0 : 0.0)
                                : as_double(*value);
        rnd_state_ = seed_from_number(seed);
    }
    return true;
}

bool Interpreter::parse_exit_statement(const std::size_t statement_offset) {
    skip_horizontal_whitespace();
    if (consume_keyword("do")) {
        if (do_depth_ == 0U) {
            set_error("WFC0042", "Exit Do is not inside a Do loop",
                      statement_offset);
            return false;
        }
        if (execute_) {
            exit_do_requested_ = true;
        }
        return true;
    }
    if (consume_keyword("for")) {
        if (for_depth_ == 0U) {
            set_error("WFC0052", "Exit For is not inside a For loop",
                      statement_offset);
            return false;
        }
        if (execute_) {
            exit_for_requested_ = true;
        }
        return true;
    }
    if (consume_keyword("sub")) {
        if (!in_procedure() || current_scope().is_function_frame) {
            set_error("WFC0124", "Exit Sub is not inside a Sub",
                      statement_offset);
            return false;
        }
        if (execute_) {
            exit_sub_requested_ = true;
        }
        return true;
    }
    if (consume_keyword("function")) {
        if (!in_procedure() || !current_scope().is_function_frame) {
            set_error("WFC0125", "Exit Function is not inside a Function",
                      statement_offset);
            return false;
        }
        if (execute_) {
            exit_function_requested_ = true;
        }
        return true;
    }
    if (consume_keyword("property")) {
        if (!in_procedure()) {
            set_error("WFC0124", "Exit Property is not inside a Property",
                      statement_offset);
            return false;
        }
        if (execute_) {
            if (current_scope().is_function_frame) {
                exit_function_requested_ = true;
            } else {
                exit_sub_requested_ = true;
            }
        }
        return true;
    }
    set_error("WFC0041",
              "expected Do, For, Sub, Function, or Property after Exit",
              offset_);
    return false;
}

bool Interpreter::control_exit_requested() const noexcept {
    return exit_do_requested_ || exit_for_requested_ || exit_sub_requested_ ||
           exit_function_requested_;
}

bool Interpreter::parse_if_statement() {
    skip_horizontal_whitespace();
    const auto condition_offset = offset_;
    auto condition = parse_expression();
    if (!condition.has_value()) {
        return false;
    }
    const auto boolean =
        coerce_condition_boolean(*condition, condition_offset, "WFC0021",
                                 "If condition must be Boolean");
    if (!boolean.has_value()) {
        return false;
    }

    skip_horizontal_whitespace();
    if (!consume_keyword("then")) {
        set_error("WFC0022", "expected Then", offset_);
        return false;
    }

    const bool enclosing_execution = execute_;
    skip_horizontal_whitespace();
    if (!at_end() &&
        (current() == '\r' || current() == '\n' || current() == '\'')) {
        return parse_block_if_statement(enclosing_execution, *boolean);
    }
    execute_ = enclosing_execution && *boolean;
    if (!parse_inline_statement_list()) {
        execute_ = enclosing_execution;
        return false;
    }

    skip_horizontal_whitespace();
    if (consume_keyword("else")) {
        execute_ =
            enclosing_execution && !*boolean && !control_exit_requested();
        if (!parse_inline_statement_list()) {
            execute_ = enclosing_execution;
            return false;
        }
    }
    execute_ = control_exit_requested() ? false : enclosing_execution;
    return true;
}

bool Interpreter::parse_inline_statement_list() {
    const bool branch_execution = execute_;
    while (true) {
        if (!parse_inline_statement()) {
            return false;
        }
        if (control_exit_requested()) {
            execute_ = false;
        }
        skip_horizontal_whitespace();
        if (at_end() || current() != ':') {
            execute_ = branch_execution && !control_exit_requested();
            return true;
        }
        const auto colon_offset = offset_;
        advance();
        skip_horizontal_whitespace();
        const auto probe = offset_;
        if (at_end() || current() == '\r' || current() == '\n' ||
            current() == '\'' || consume_keyword("else")) {
            offset_ = at_end() || current() == '\r' || current() == '\n' ||
                              current() == '\''
                          ? colon_offset
                          : probe;
            execute_ = branch_execution && !control_exit_requested();
            return true;
        }
        offset_ = probe;
    }
}

bool Interpreter::parse_block_if_statement(const bool enclosing_execution,
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
                set_error("WFC0030", "ElseIf is not permitted after Else",
                          elseif_offset);
                return false;
            }

            const bool evaluate_condition =
                enclosing_execution && !branch_selected;
            execute_ = evaluate_condition;
            skip_horizontal_whitespace();
            const auto condition_offset = offset_;
            auto elseif_condition = parse_expression();
            if (!elseif_condition.has_value()) {
                execute_ = enclosing_execution;
                return false;
            }
            const auto elseif_boolean = coerce_condition_boolean(
                *elseif_condition, condition_offset, "WFC0028",
                "ElseIf condition must be Boolean");
            if (!elseif_boolean.has_value()) {
                execute_ = enclosing_execution;
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
        const bool consumed_statement_end =
            parsed_statement && consume_statement_end();
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

bool Interpreter::parse_while_statement() {
    const bool enclosing_execution = execute_;
    skip_horizontal_whitespace();
    const auto condition_offset = offset_;
    auto condition = parse_expression();
    if (!condition.has_value()) {
        return false;
    }
    const auto boolean =
        coerce_condition_boolean(*condition, condition_offset, "WFC0031",
                                 "While condition must be Boolean");
    if (!boolean.has_value()) {
        return false;
    }
    if (!consume_loop_header_end()) {
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
        const auto next_boolean = coerce_condition_boolean(
            *next_condition, condition_offset, "WFC0031",
            "While condition must be Boolean");
        if (!next_boolean.has_value()) {
            execute_ = enclosing_execution;
            return false;
        }
        if (!consume_loop_header_end()) {
            execute_ = enclosing_execution;
            return false;
        }
        continue_loop = *next_boolean;
    }

    offset_ = continuation_offset;
    execute_ = enclosing_execution;
    return true;
}

bool Interpreter::parse_with_statement(const std::size_t statement_offset) {
    const bool enclosing_execution = execute_;
    skip_horizontal_whitespace();
    const auto expression_offset = offset_;
    auto value = parse_expression();
    if (!value.has_value()) {
        return false;
    }
    const bool is_object = std::holds_alternative<ObjectInstance>(*value) ||
                           std::holds_alternative<Nothing>(*value);
    if (enclosing_execution && !is_object) {
        set_error("WFC0136", "With requires an object reference",
                  expression_offset);
        return false;
    }
    if (!consume_loop_header_end()) {
        return false;
    }
    const std::string name = "with." + std::to_string(++with_counter_);
    with_scope_.object_variables.insert(name);
    with_slots_[name] = is_object ? std::move(*value) : Value{Nothing{}};
    with_names_.push_back(name);
    const auto cleanup = [&] {
        with_names_.pop_back();
        with_slots_.erase(name);
    };
    while (true) {
        skip_program_leading_trivia();
        if (at_end()) {
            cleanup();
            set_error("WFC0025", "expected End With", statement_offset);
            return false;
        }
        if (consume_keyword("end")) {
            skip_horizontal_whitespace();
            if (!consume_keyword("with")) {
                cleanup();
                set_error("WFC0025", "expected With after End", offset_);
                return false;
            }
            cleanup();
            execute_ = control_exit_requested() ? false : enclosing_execution;
            return true;
        }
        const bool enclosing_declaration_permission = allow_declarations_;
        allow_declarations_ = false;
        const bool parsed_statement = parse_statement();
        const bool consumed_statement_end =
            parsed_statement && consume_statement_end();
        allow_declarations_ = enclosing_declaration_permission;
        if (!parsed_statement || !consumed_statement_end) {
            cleanup();
            execute_ = enclosing_execution;
            return false;
        }
        if (control_exit_requested()) {
            execute_ = false;
        }
    }
}

std::size_t Interpreter::find_loop_end(const std::size_t from) const {
    std::size_t position = source_.find('\n', from);
    if (position == std::string_view::npos) {
        return std::string_view::npos;
    }
    ++position;
    int depth = 0;
    while (position < source_.size()) {
        std::size_t line_end = source_.find('\n', position);
        if (line_end == std::string_view::npos) {
            line_end = source_.size();
        }
        std::size_t i = position;
        while (i < line_end && (source_[i] == ' ' || source_[i] == '\t')) {
            ++i;
        }
        const auto word_is = [&](const std::string_view w) {
            if (line_end - i < w.size()) {
                return false;
            }
            for (std::size_t k = 0; k < w.size(); ++k) {
                if (ascii_lower(source_[i + k]) != w[k]) {
                    return false;
                }
            }
            return line_end - i == w.size() ||
                   !is_identifier_part(source_[i + w.size()]);
        };
        if (word_is("do") || word_is("while") || word_is("for")) {
            ++depth;
        } else if (word_is("loop") || word_is("wend") || word_is("next")) {
            if (depth == 0) {
                return position;
            }
            --depth;
            if (word_is("next")) {
                // `Next j, i` closes several loops at once.
                for (std::size_t k = i; k < line_end && source_[k] != '\'';
                     ++k) {
                    if (source_[k] == ',' && depth > 0) {
                        --depth;
                    }
                }
            }
        }
        position = line_end + 1;
    }
    return std::string_view::npos;
}

bool Interpreter::take_local_jump(const std::size_t body_start,
                                  const std::size_t statement_start) {
    if (!jump_pending_) {
        return false;
    }
    const auto target = jump_target_;
    bool local = false;
    if (target >= body_start && target <= statement_start) {
        local = true;
    } else if (target > statement_start) {
        const auto end = find_loop_end(statement_start);
        local = end != std::string_view::npos && target < end;
    }
    if (!local) {
        return false;
    }
    jump_pending_ = false;
    error_ = wfc::Evaluation{};
    offset_ = target;
    execute_ = true;
    return true;
}

bool Interpreter::parse_while_body(std::size_t& continuation_offset) {
    const auto body_start = offset_;
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
        const bool enclosing_declaration_permission = allow_declarations_;
        allow_declarations_ = false;
        const bool parsed_statement = parse_statement();
        const bool consumed_statement_end =
            parsed_statement && consume_statement_end();
        allow_declarations_ = enclosing_declaration_permission;
        if (!parsed_statement &&
            take_local_jump(body_start, statement_offset)) {
            continue;
        }
        if (!parsed_statement || !consumed_statement_end) {
            return false;
        }
        if (control_exit_requested()) {
            execute_ = false;
        }
    }
}

bool Interpreter::parse_do_statement() {
    const bool enclosing_execution = execute_;
    skip_horizontal_whitespace();
    if (!at_end() && (current() == '\r' || current() == '\n' ||
                      current() == ':' || current() == '\'')) {
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
    const auto boolean =
        coerce_condition_boolean(*condition, condition_offset, "WFC0035",
                                 "Do condition must be Boolean");
    if (!boolean.has_value()) {
        return false;
    }
    if (!consume_loop_header_end()) {
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
        if (exit_for_requested_ || exit_sub_requested_ ||
            exit_function_requested_) {
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
        const auto next_boolean =
            coerce_condition_boolean(*next_condition, condition_offset,
                                     "WFC0035", "Do condition must be Boolean");
        if (!next_boolean.has_value()) {
            execute_ = enclosing_execution;
            return false;
        }
        if (!consume_loop_header_end()) {
            execute_ = enclosing_execution;
            return false;
        }
        continue_loop = until ? !*next_boolean : *next_boolean;
    }

    offset_ = continuation_offset;
    execute_ = enclosing_execution;
    return true;
}

bool Interpreter::parse_posttest_do_statement(const bool enclosing_execution) {
    if (!consume_loop_header_end()) {
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
        const bool exit_for_requested = exit_for_requested_ ||
                                        exit_sub_requested_ ||
                                        exit_function_requested_;

        skip_horizontal_whitespace();
        bool until{};
        bool has_condition = true;
        if (consume_keyword("while")) {
            until = false;
        } else if (consume_keyword("until")) {
            until = true;
        } else if (at_end() || current() == '\r' || current() == '\n' ||
                   current() == ':' || current() == '\'') {
            // REQ-0225: an unconditional `Do ... Loop` (no `While`/
            // `Until` on either the `Do` or the `Loop` line) repeats
            // forever, relying entirely on `Exit Do`/`Exit For` to end
            // it -- the same convention `Do While True` already lets a
            // caller express, just without writing a condition at all.
            has_condition = false;
        } else {
            execute_ = enclosing_execution;
            set_error("WFC0040", "expected While or Until after Loop", offset_);
            return false;
        }

        if (has_condition) {
            skip_horizontal_whitespace();
            const auto condition_offset = offset_;
            auto condition = parse_expression();
            if (!condition.has_value()) {
                execute_ = enclosing_execution;
                return false;
            }
            const auto boolean = coerce_condition_boolean(
                *condition, condition_offset, "WFC0035",
                "Do condition must be Boolean");
            if (!boolean.has_value()) {
                execute_ = enclosing_execution;
                return false;
            }
            continuation_offset = offset_;
            if (exit_do_requested) {
                exit_do_requested_ = false;
                continue_loop = false;
            } else if (exit_for_requested) {
                continue_loop = false;
            } else {
                continue_loop =
                    enclosing_execution && (until ? !*boolean : *boolean);
            }
        } else {
            continuation_offset = offset_;
            if (exit_do_requested) {
                exit_do_requested_ = false;
                continue_loop = false;
            } else if (exit_for_requested) {
                continue_loop = false;
            } else {
                // No condition ever ends this loop on its own; only
                // `enclosing_execution` (a dead branch parses the body
                // once, no repeat -- matching every other loop kind's
                // own dry-run behavior) or `Exit Do`/`Exit For` above
                // can stop it.
                continue_loop = enclosing_execution;
            }
        }
    } while (continue_loop);

    offset_ = continuation_offset;
    execute_ = enclosing_execution;
    return true;
}

bool Interpreter::parse_do_body(std::size_t& continuation_offset) {
    const auto body_start = offset_;
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
        const bool enclosing_declaration_permission = allow_declarations_;
        allow_declarations_ = false;
        const bool parsed_statement = parse_statement();
        const bool consumed_statement_end =
            parsed_statement && consume_statement_end();
        allow_declarations_ = enclosing_declaration_permission;
        if (!parsed_statement &&
            take_local_jump(body_start, statement_offset)) {
            continue;
        }
        if (!parsed_statement || !consumed_statement_end) {
            return false;
        }
        if (control_exit_requested()) {
            execute_ = false;
        }
    }
}

bool Interpreter::parse_for_statement() {
    const bool enclosing_execution = execute_;
    skip_horizontal_whitespace();
    const auto variable_offset = offset_;
    char type_character{};
    auto identifier = parse_identifier(&type_character);
    if (!identifier.has_value()) {
        set_error("WFC0043", "expected For control variable", variable_offset);
        return false;
    }
    const auto variable = find_variable(*identifier);
    if (variable.value == nullptr) {
        set_error("WFC0015", "undeclared variable", variable_offset);
        return false;
    }
    if (!type_character_matches(*variable.value, type_character,
                                variable_offset)) {
        return false;
    }
    const bool is_variant_variable =
        variable.scope->variant_variables.contains(*identifier);
    enum class Slot { integer, int16, byte, floating_double, floating_single };
    Slot slot{};
    if (std::holds_alternative<Integer>(*variable.value)) {
        slot = Slot::integer;
    } else if (std::holds_alternative<Int16>(*variable.value)) {
        slot = Slot::int16;
    } else if (std::holds_alternative<Byte>(*variable.value)) {
        slot = Slot::byte;
    } else if (std::holds_alternative<double>(*variable.value)) {
        slot = Slot::floating_double;
    } else if (std::holds_alternative<float>(*variable.value)) {
        slot = Slot::floating_single;
    } else if (!is_variant_variable) {
        set_error("WFC0045", "For control variable must be numeric",
                  variable_offset);
        return false;
    }
    // Otherwise a Variant holding a non-numeric value keeps slot's default,
    // Slot::integer, and is refined from the bounds below.

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
    Value step_value{Integer{1}};
    skip_horizontal_whitespace();
    if (consume_keyword("step")) {
        skip_horizontal_whitespace();
        auto parsed_step = parse_expression();
        if (!parsed_step.has_value()) {
            return false;
        }
        step_value = std::move(*parsed_step);
    }
    const auto numeric = [this](const Value& value) -> std::optional<double> {
        if (const auto* v = std::get_if<Integer>(&value)) {
            return static_cast<double>(*v);
        }
        if (const auto* v = std::get_if<Int16>(&value)) {
            return static_cast<double>(*v);
        }
        if (const auto* v = std::get_if<Byte>(&value)) {
            return static_cast<double>(*v);
        }
        if (const auto* v = std::get_if<double>(&value)) {
            return *v;
        }
        if (const auto* v = std::get_if<float>(&value)) {
            return static_cast<double>(*v);
        }
        if (std::holds_alternative<Currency>(value) ||
            std::holds_alternative<Decimal>(value)) {
            return as_double(value);
        }
        if (const auto* v = std::get_if<DateValue>(&value)) {
            return v->serial;
        }
        if (std::holds_alternative<Empty>(value)) {
            return 0.0;
        }
        if (const auto* v = std::get_if<bool>(&value)) {
            return *v ? -1.0 : 0.0;
        }
        if (!execute_) {
            return 0.0;  // a placeholder operand of a not-taken branch
        }
        return std::nullopt;
    };
    const auto start_number = numeric(*start_value);
    const auto end_number = numeric(*end_value);
    const auto step_number = numeric(step_value);
    if (!start_number || !end_number || !step_number) {
        set_error("WFC0045", "For bounds and Step must be numeric",
                  variable_offset);
        return false;
    }
    if (is_variant_variable) {
        const bool any_floating =
            std::holds_alternative<double>(*start_value) ||
            std::holds_alternative<float>(*start_value) ||
            std::holds_alternative<double>(*end_value) ||
            std::holds_alternative<float>(*end_value) ||
            std::holds_alternative<double>(step_value) ||
            std::holds_alternative<float>(step_value);
        slot = any_floating ? Slot::floating_double : Slot::integer;
    }
    if (*step_number == 0.0) {
        set_error("WFC0047", "For Step cannot be zero", variable_offset);
        return false;
    }
    if (!consume_loop_header_end()) {
        return false;
    }

    const bool floating =
        slot == Slot::floating_double || slot == Slot::floating_single;
    std::int64_t whole_min = std::numeric_limits<Integer>::min();
    std::int64_t whole_max = std::numeric_limits<Integer>::max();
    if (slot == Slot::int16) {
        whole_min = std::numeric_limits<Int16>::min();
        whole_max = std::numeric_limits<Int16>::max();
    } else if (slot == Slot::byte) {
        whole_min = 0;
        whole_max = 255;
    }
    const auto round_whole = [](const double number) {
        return static_cast<std::int64_t>(std::nearbyint(number));
    };
    const auto store = [&](const double number) {
        switch (slot) {
            case Slot::integer:
                *variable.value = static_cast<Integer>(round_whole(number));
                break;
            case Slot::int16:
                *variable.value = static_cast<Int16>(round_whole(number));
                break;
            case Slot::byte:
                *variable.value = static_cast<Byte>(round_whole(number));
                break;
            case Slot::floating_double:
                *variable.value = number;
                break;
            case Slot::floating_single:
                *variable.value = static_cast<float>(number);
                break;
        }
    };
    const auto read = [&]() -> double {
        const auto current = numeric(*variable.value);
        return current.has_value() ? *current : 0.0;
    };
    double limit = *end_number;
    double step = *step_number;
    double current_value = *start_number;
    if (!floating) {
        limit = static_cast<double>(round_whole(limit));
        step = static_cast<double>(round_whole(step));
        current_value = static_cast<double>(round_whole(current_value));
        if (step == 0.0) {
            set_error("WFC0047", "For Step cannot be zero", variable_offset);
            return false;
        }
        if (enclosing_execution &&
            (current_value < static_cast<double>(whole_min) ||
             current_value > static_cast<double>(whole_max))) {
            set_error("WFC0009", "numeric overflow", variable_offset);
            return false;
        }
    } else if (slot == Slot::floating_single) {
        limit = static_cast<float>(limit);
        step = static_cast<float>(step);
        current_value = static_cast<float>(current_value);
    }
    const auto should_continue = [&](const double current) {
        return step > 0.0 ? current <= limit : current >= limit;
    };

    const auto body_offset = offset_;
    std::size_t continuation_offset{};
    if (enclosing_execution) {
        store(current_value);
    }
    bool continue_loop = enclosing_execution && should_continue(current_value);
    if (!continue_loop) {
        execute_ = false;
        ++for_depth_;
        const bool parsed_body =
            parse_for_body(*identifier, continuation_offset);
        --for_depth_;
        execute_ = enclosing_execution;
        return parsed_body;
    }

    while (continue_loop) {
        store(current_value);
        offset_ = body_offset;
        execute_ = enclosing_execution;
        ++for_depth_;
        const bool parsed_body =
            parse_for_body(*identifier, continuation_offset);
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

        // The body may have assigned the control variable.
        double next = read() + step;
        if (slot == Slot::floating_single) {
            next = static_cast<float>(next);
        }
        if (!floating && (next < static_cast<double>(whole_min) ||
                          next > static_cast<double>(whole_max))) {
            set_error("WFC0047", "For control variable overflow",
                      variable_offset);
            execute_ = enclosing_execution;
            return false;
        }
        current_value = next;
        continue_loop = should_continue(current_value);
    }

    store(current_value);
    offset_ = continuation_offset;
    execute_ = enclosing_execution;
    return true;
}

bool Interpreter::parse_for_body(const std::string_view identifier,
                                 std::size_t& continuation_offset) {
    pending_next_comma_ = false;
    const auto body_start = offset_;
    while (true) {
        if (pending_next_comma_) {
            // REQ-0248: the nested loop's `Next j, i` left `, i` for us.
            skip_horizontal_whitespace();
            if (consume(',')) {
                pending_next_comma_ = false;
                skip_horizontal_whitespace();
                const auto name_offset = offset_;
                char name_type_character{};
                auto name = parse_identifier(&name_type_character);
                if (!name.has_value() || *name != identifier) {
                    set_error("WFC0049", "Next variable does not match For",
                              name_offset);
                    return false;
                }
                skip_horizontal_whitespace();
                if (!at_end() && current() == ',') {
                    pending_next_comma_ = true;
                }
                continuation_offset = offset_;
                return true;
            }
        }
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
                set_error("WFC0049", "Next variable does not match For",
                          next_identifier_offset);
                return false;
            }
            if (next_identifier.has_value()) {
                const auto variable = find_variable(*next_identifier);
                if (variable.value == nullptr ||
                    !type_character_matches(*variable.value,
                                            next_type_character,
                                            next_identifier_offset)) {
                    return false;
                }
            }
            skip_horizontal_whitespace();
            if (!at_end() && current() == ',') {
                pending_next_comma_ = true;
            }
            continuation_offset = offset_;
            return true;
        }

        const auto statement_offset = offset_;
        const bool enclosing_declaration_permission = allow_declarations_;
        allow_declarations_ = false;
        const bool parsed_statement = parse_statement();
        const bool consumed_statement_end =
            parsed_statement && consume_statement_end();
        allow_declarations_ = enclosing_declaration_permission;
        if (!parsed_statement &&
            take_local_jump(body_start, statement_offset)) {
            continue;
        }
        if (!parsed_statement || !consumed_statement_end) {
            return false;
        }
        if (control_exit_requested()) {
            execute_ = false;
        }
    }
}

bool Interpreter::parse_for_each_statement() {
    const bool enclosing_execution = execute_;
    skip_horizontal_whitespace();
    const auto variable_offset = offset_;
    char type_character{};
    auto identifier = parse_identifier(&type_character);
    if (!identifier.has_value()) {
        set_error("WFC0043", "expected For Each control variable",
                  variable_offset);
        return false;
    }
    const auto variable = find_variable(*identifier);
    if (variable.value == nullptr) {
        set_error("WFC0015", "undeclared variable", variable_offset);
        return false;
    }
    if (!type_character_matches(*variable.value, type_character,
                                variable_offset)) {
        return false;
    }
    skip_horizontal_whitespace();
    if (!consume_keyword("in")) {
        set_error("WFC0147", "expected In after For Each control variable",
                  offset_);
        return false;
    }
    skip_horizontal_whitespace();
    const auto collection_offset = offset_;
    auto collection_value = parse_expression();
    if (!collection_value.has_value()) {
        return false;
    }
    // REQ-0243: For Each over a Collection (or any class exposing a
    // `WfcItems` method) iterates the array that method returns.
    if (execute_) {
        if (const auto* holder =
                std::get_if<ObjectInstance>(&*collection_value)) {
            auto class_iterator =
                class_definitions_.find(holder->data->class_name);
            // A class exposing a `NewEnum` method (VB_UserMemId -4): iterate
            // what it returns.
            if (class_iterator != class_definitions_.end() &&
                !class_iterator->second.methods.contains("wfcitems") &&
                class_iterator->second.methods.contains("newenum")) {
                auto enumerator =
                    call_class_method(*holder->data, class_iterator->second,
                                      "newenum", collection_offset,
                                      /*require_function=*/true);
                if (!enumerator.has_value()) {
                    return false;
                }
                collection_value = std::move(enumerator);
                holder = std::get_if<ObjectInstance>(&*collection_value);
                class_iterator =
                    holder != nullptr
                        ? class_definitions_.find(holder->data->class_name)
                        : class_definitions_.end();
            }
            if (holder != nullptr &&
                class_iterator != class_definitions_.end() &&
                class_iterator->second.methods.contains("wfcitems")) {
                auto items =
                    call_class_method(*holder->data, class_iterator->second,
                                      "wfcitems", collection_offset,
                                      /*require_function=*/true);
                if (!items.has_value()) {
                    return false;
                }
                collection_value = std::move(items);
            }
        }
    }
    const auto* array = std::get_if<ArrayValue>(&*collection_value);
    if (array == nullptr && !execute_) {
        static const ArrayValue
            empty_array{};  // a placeholder in a not-taken branch
        array = &empty_array;
    }
    if (array == nullptr) {
        set_error("WFC0147", "For Each requires an array", collection_offset);
        return false;
    }
    if (!consume_loop_header_end()) {
        return false;
    }

    const bool target_is_variant =
        variable.scope->variant_variables.contains(*identifier);
    const auto count = array->elements.size();
    const auto assign_element = [&](const std::size_t index) -> bool {
        Value element_value = array->elements[index];
        if (variable.scope->object_variables.contains(*identifier) &&
            is_object_reference(element_value)) {
            *variable.value = std::move(element_value);
            return true;
        }
        if (target_is_variant) {
            *variable.value = std::move(element_value);
            return true;
        }
        if (!coerce_numeric_value(element_value, variable.value->index(),
                                  variable_offset)) {
            return false;
        }
        if (element_value.index() != variable.value->index()) {
            set_error("WFC0016", "assignment type mismatch", variable_offset);
            return false;
        }
        *variable.value = std::move(element_value);
        return true;
    };

    const auto body_offset = offset_;
    std::size_t continuation_offset{};
    std::size_t index = 0;
    bool continue_loop = enclosing_execution && index < count;
    if (continue_loop && !assign_element(index)) {
        execute_ = enclosing_execution;
        return false;
    }
    if (!continue_loop) {
        execute_ = false;
        ++for_depth_;
        const bool parsed_body =
            parse_for_body(*identifier, continuation_offset);
        --for_depth_;
        execute_ = enclosing_execution;
        return parsed_body;
    }

    while (continue_loop) {
        offset_ = body_offset;
        execute_ = enclosing_execution;
        ++for_depth_;
        const bool parsed_body =
            parse_for_body(*identifier, continuation_offset);
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
        ++index;
        continue_loop = index < count;
        if (continue_loop && !assign_element(index)) {
            execute_ = enclosing_execution;
            return false;
        }
    }

    offset_ = continuation_offset;
    execute_ = enclosing_execution;
    return true;
}

bool Interpreter::parse_select_statement() {
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
    if (!consume_loop_header_end()) {
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
                if (!consume_case_line_end()) {
                    execute_ = enclosing_execution;
                    return false;
                }
                execute_ = enclosing_execution && !branch_selected;
                branch_selected = true;
                continue;
            }
            if (has_else) {
                execute_ = enclosing_execution;
                set_error("WFC0057", "Case is not permitted after Case Else",
                          case_offset);
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
                        set_error("WFC0061",
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
                const bool both_numeric =
                    is_number(*case_value) && is_number(*selector);
                if (case_value->index() != selector->index() && !both_numeric &&
                    enclosing_execution &&
                    !std::holds_alternative<Empty>(*selector) &&
                    !std::holds_alternative<Empty>(*case_value)) {
                    execute_ = enclosing_execution;
                    set_error("WFC0053", "Case value must match selector type",
                              value_offset);
                    return false;
                }
                skip_horizontal_whitespace();
                bool item_matches{};
                if (!relational_operator.empty()) {
                    auto comparison =
                        compare(*selector, *case_value, relational_operator,
                                value_offset);
                    if (!comparison.has_value()) {
                        execute_ = enclosing_execution;
                        return false;
                    }
                    // A Null selector/case value makes `compare` return
                    // Null itself (three-valued logic), which is never a
                    // match, matching Select Case Null never selecting
                    // any Case clause in real VB6.
                    const auto* comparison_boolean =
                        std::get_if<bool>(&*comparison);
                    item_matches =
                        comparison_boolean != nullptr && *comparison_boolean;
                } else if (consume_keyword("to")) {
                    skip_horizontal_whitespace();
                    const auto upper_offset = offset_;
                    auto upper_value = parse_expression();
                    if (!upper_value.has_value()) {
                        execute_ = enclosing_execution;
                        return false;
                    }
                    if ((upper_value->index() != selector->index() &&
                         !(is_number(*upper_value) && is_number(*selector))) ||
                        std::holds_alternative<bool>(*selector) ||
                        (!is_number(*selector) &&
                         !std::holds_alternative<std::string>(*selector))) {
                        execute_ = enclosing_execution;
                        set_error("WFC0060",
                                  "Case range requires same-type Long or "
                                  "String values",
                                  upper_offset);
                        return false;
                    }
                    if (is_number(*selector)) {
                        const double selected_number = as_double(*selector);
                        item_matches =
                            as_double(*case_value) <= selected_number &&
                            selected_number <= as_double(*upper_value);
                    } else {
                        const auto& selected_string =
                            std::get<std::string>(*selector);
                        item_matches =
                            compare_strings(std::get<std::string>(*case_value),
                                            selected_string) <= 0 &&
                            compare_strings(
                                selected_string,
                                std::get<std::string>(*upper_value)) <= 0;
                    }
                    skip_horizontal_whitespace();
                } else {
                    item_matches =
                        both_numeric
                            ? as_double(*case_value) == as_double(*selector)
                            : values_equal(*case_value, *selector);
                }
                case_matches = case_matches || item_matches;
                if (!consume(',')) {
                    break;
                }
                skip_horizontal_whitespace();
            }
            if (!consume_case_line_end()) {
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
            set_error("WFC0054", "expected Case or End Select",
                      selector_offset);
            return false;
        }

        const bool enclosing_declaration_permission = allow_declarations_;
        allow_declarations_ = false;
        const bool parsed_statement = parse_statement();
        const bool consumed_statement_end =
            parsed_statement && consume_statement_end();
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

std::optional<bool> Interpreter::parse_bare_call(
    const std::string& identifier, const std::size_t identifier_offset,
    const char type_character, const bool has_let) {
    if (!has_let && type_character == '\0' &&
        find_variable(identifier).value == nullptr) {
        const auto saved_offset = offset_;
        skip_horizontal_whitespace();
        const bool bare_statement_end = at_end() || current() == '\r' ||
                                        current() == '\n' || current() == ':' ||
                                        current() == '\'';
        const bool arguments_follow = !bare_statement_end && current() != '=' &&
                                      current() != '(' && current() != '.';
        // `Name (arg)` / `Name(a, b)` as a statement: a parenthesized
        // list ending the statement. One argument is passed by value
        // (VB evaluates `(x)` as an expression); several are an
        // ordinary argument list.
        bool parenthesized_call = false;
        bool parenthesized_list = false;
        if (!bare_statement_end && !at_end() && current() == '(' &&
            (procedures_.contains(identifier) ||
             (current_instance() != nullptr && current_class_def() != nullptr &&
              current_class_def()->methods.contains(identifier)))) {
            const auto group = scan_statement_paren_group(offset_);
            parenthesized_call = group.ends_statement;
            parenthesized_list = group.is_list;
        }
        offset_ = saved_offset;
        if (parenthesized_call) {
            bare_call_arguments_ = !parenthesized_list;
            std::optional<Value> result;
            if (procedures_.contains(identifier)) {
                result = call_procedure(identifier, identifier_offset, false);
            } else {
                result =
                    call_class_method(*current_instance(), *current_class_def(),
                                      identifier, identifier_offset, false);
            }
            bare_call_arguments_ = false;
            return result.has_value();
        }
        if (bare_statement_end || arguments_follow) {
            bare_call_arguments_ = arguments_follow;
            if (procedures_.contains(identifier)) {
                const auto result =
                    call_procedure(identifier, identifier_offset, false);
                bare_call_arguments_ = false;
                return result.has_value();
            }
            if (auto* const instance = current_instance()) {
                if (const auto* const class_def = current_class_def()) {
                    if (class_def->methods.contains(identifier)) {
                        const auto result =
                            call_class_method(*instance, *class_def, identifier,
                                              identifier_offset, false);
                        bare_call_arguments_ = false;
                        return result.has_value();
                    }
                }
            }
            bare_call_arguments_ = false;
        }
    }
    return std::nullopt;
}

bool Interpreter::parse_inline_statement() {
    while (true) {
        const auto start = offset_;
        const bool entry_execute = execute_;
        if (parse_inline_statement_core()) {
            return true;
        }
        if (!entry_execute || jump_pending_) {
            return false;
        }
        execute_ = entry_execute;
        if (!recover_runtime_error(start)) {
            return false;
        }
        if (!retry_statement_) {
            return true;
        }
        retry_statement_ = false;
        offset_ = start;
    }
}

bool Interpreter::parse_inline_statement_core() {
    skip_horizontal_whitespace();
    const auto statement_offset = offset_;
    if (at_end() || current() == '\r' || current() == '\n' ||
        current() == ':' || consume_keyword("else")) {
        offset_ = statement_offset;
        set_error("WFC0023", "expected Print or assignment branch",
                  statement_offset);
        return false;
    }
    if (consume_keyword("print")) {
        return parse_print_statement();
    }
    if (consume_keyword("set")) {
        return parse_set_statement();
    }
    if (consume_keyword("call")) {
        return parse_call_statement();
    }
    if (consume_keyword("exit")) {
        return parse_exit_statement(statement_offset);
    }
    if (const auto handled =
            parse_error_handling_statement(statement_offset, false)) {
        return *handled;
    }

    {
        // Other simple statements run through the ordinary dispatcher
        // (`If a Then If b Then ...`, `If a Then Close #1`, `GoTo`...).
        const auto probe = offset_;
        char probe_type_character{};
        const auto word = parse_identifier(&probe_type_character);
        offset_ = probe;
        static const std::set<std::string, std::less<>> delegated = {
            "if",         "goto",     "gosub",       "return",        "resume",
            "redim",      "erase",    "open",        "close",         "write",
            "input",      "line",     "get",         "put",           "seek",
            "kill",       "name",     "mkdir",       "rmdir",         "chdir",
            "randomize",  "lset",     "rset",        "end",           "stop",
            "raiseevent", "mid",      "savesetting", "deletesetting", "chdrive",
            "unlock",     "lock",     "reset",       "load",          "unload",
            "beep",       "doevents", "date",        "time",          "dim",
            "static",     "const",    "error",       "debug"};
        if (word.has_value() &&
            (probe_type_character == '\0' ||
             (probe_type_character == '$' && *word == "mid")) &&
            delegated.contains(*word) && *word != "date" && *word != "time") {
            return parse_statement_core();
        }
    }
    const bool has_let = consume_keyword("let");
    if (has_let) {
        skip_horizontal_whitespace();
    }
    const auto inline_identifier_offset = offset_;
    char type_character{};
    auto identifier = parse_identifier(&type_character);
    if (!identifier.has_value() || is_reserved_identifier(*identifier)) {
        set_error("WFC0023", "expected Print or assignment branch",
                  statement_offset);
        return false;
    }
    if (const auto handled = parse_bare_call(
            *identifier, inline_identifier_offset, type_character, has_let)) {
        return *handled;
    }
    return parse_assignment_or_array_element(std::move(*identifier),
                                             type_character);
}

bool Interpreter::parse_call_statement() {
    skip_horizontal_whitespace();
    const auto identifier_offset = offset_;
    char type_character{};
    auto identifier = parse_identifier(&type_character);
    if (!identifier.has_value() || type_character != '\0') {
        set_error("WFC0011", "expected procedure name after Call",
                  identifier_offset);
        return false;
    }
    // `Call Me.Method(args)`.
    if (*identifier == "me") {
        skip_horizontal_whitespace();
        if (!at_end() && current() == '.') {
            auto base = me_value(identifier_offset);
            if (!base.has_value()) {
                return false;
            }
            advance();
            const auto result = parse_member_access_after_dot(
                *base, identifier_offset, /*require_function=*/false);
            return result.has_value();
        }
        set_error("WFC0015", "undeclared procedure", identifier_offset);
        return false;
    }
    // `Call obj.Method(args)` -- a method call on an object reference,
    // dispatched the same way an expression's `obj.Method(args)` would
    // be, but allowing a Sub (its result is simply discarded, like any
    // other Call target).
    const auto after_identifier_offset = offset_;
    skip_horizontal_whitespace();
    if (!at_end() && current() == '.') {
        const auto variable = find_variable(*identifier);
        if (variable.value != nullptr &&
            (std::holds_alternative<Nothing>(*variable.value) ||
             std::holds_alternative<ObjectInstance>(*variable.value))) {
            const auto base = *variable.value;
            // REQ-0233: `Call obj.Method(args)` dispatches through
            // `obj`'s own declared class the same way an expression's
            // `obj.Method(args)` does (`parse_primary`'s own
            // lookahead) -- `obj` is already a resolved variable here,
            // so its declared class (if any) is read directly instead
            // of needing a speculative lookahead.
            std::string declared_interface_class;
            const auto declared_class =
                variable.scope->object_class_names.find(*identifier);
            if (declared_class != variable.scope->object_class_names.end()) {
                declared_interface_class = declared_class->second;
            }
            advance();
            const auto result = parse_member_access_after_dot(
                base, identifier_offset, /*require_function=*/false,
                declared_interface_class);
            return result.has_value();
        }
    }
    offset_ = after_identifier_offset;
    if (!procedures_.contains(*identifier)) {
        // An unqualified `Call Method(args)` for a sibling method of
        // the class currently executing (see current_instance/
        // current_class_def) -- the Call-statement counterpart of the
        // same implicit-Me convenience parse_primary_base gives
        // expressions.
        if (auto* const instance = current_instance()) {
            if (const auto* const class_def = current_class_def()) {
                if (class_def->methods.contains(*identifier)) {
                    const auto result = call_class_method(
                        *instance, *class_def, *identifier, identifier_offset,
                        /*require_function=*/false);
                    return result.has_value();
                }
            }
        }
        set_error("WFC0015", "undeclared procedure", identifier_offset);
        return false;
    }
    const auto result = call_procedure(*identifier, identifier_offset, false);
    return result.has_value();
}

}  // namespace wfc::detail
