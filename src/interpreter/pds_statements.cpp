// The statements of the BASIC PDS 7.1 text screen (ADR-0011): CLS, LOCATE,
// COLOR, WIDTH and VIEW PRINT, and INPUT and LINE INPUT from the keyboard.
// They act on the shared screen of pds_screen and are statements only
// outside form code (where Cls and Width belong to the form) and when the
// word is not a procedure of the program or followed by `=`, `.` or `#` (an
// assignment, a member, `Width #n`, `Input #n`).
#include <cfloat>
#include <cmath>
#include <limits>
#include <utility>

#include "interpreter.hpp"
#include "pds_screen.hpp"
#include "wfc/brun71/input.hpp"

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

namespace {

// The value an INPUT field gives a variable, or nullopt when INPUT has to
// ask again: a number that is not one, or out of the variable's range.
std::optional<Value> input_value(const Value& target, const bool variant,
                                 const brun71::InputField& field) {
    if (!variant && std::holds_alternative<std::string>(target)) {
        return Value{field.text};
    }
    const auto number =
        field.quoted ? std::nullopt : brun71::parse_input_number(field.text);
    if (variant) {
        if (!field.quoted && field.text.empty()) {
            return Value{Empty{}};
        }
        if (!number) {
            return Value{field.text};
        }
        const double whole = std::floor(*number);
        if (whole == *number && whole >= -32768.0 && whole <= 32767.0) {
            return Value{static_cast<Int16>(whole)};
        }
        if (whole == *number && std::fabs(whole) < 2147483648.0) {
            return Value{static_cast<Integer>(whole)};
        }
        return Value{*number};
    }
    if (!number) {
        return std::nullopt;
    }
    const double rounded = std::nearbyint(*number);
    if (std::holds_alternative<Int16>(target)) {
        if (!(rounded >= -32768.0 && rounded <= 32767.0)) {
            return std::nullopt;
        }
        return Value{static_cast<Int16>(rounded)};
    }
    if (std::holds_alternative<Integer>(target)) {
        if (!(rounded >= -2147483648.0 && rounded <= 2147483647.0)) {
            return std::nullopt;
        }
        return Value{static_cast<Integer>(rounded)};
    }
    if (std::holds_alternative<Byte>(target)) {
        if (!(rounded >= 0.0 && rounded <= 255.0)) {
            return std::nullopt;
        }
        return Value{static_cast<Byte>(rounded)};
    }
    if (std::holds_alternative<float>(target)) {
        if (std::fabs(*number) > FLT_MAX) {
            return std::nullopt;
        }
        return Value{static_cast<float>(*number)};
    }
    if (std::holds_alternative<bool>(target)) {
        return Value{*number != 0.0};
    }
    return Value{*number};
}

}  // namespace

bool Interpreter::parse_pds_input_targets(
    std::vector<std::pair<Value*, bool>>& targets) {
    targets.clear();
    while (true) {
        skip_horizontal_whitespace();
        const auto variable_offset = offset_;
        char type_character{};
        auto name = parse_identifier(&type_character);
        if (!name.has_value()) {
            set_error("WFC0011", "expected variable name", variable_offset);
            return false;
        }
        if (find_variable(*name).value == nullptr) {
            static_cast<void>(declare_implicit(*name, type_character));
        }
        auto variable = find_variable(*name);
        if (variable.value == nullptr) {
            set_error("WFC0015", "undeclared variable", variable_offset);
            return false;
        }
        Value* where = variable.value;
        bool variant = variable.scope->variant_variables.contains(*name);
        if (!at_end() && (current() == '(' || current() == '.')) {
            // An array element or a field: `INPUT names(i)`.
            offset_ = variable_offset;
            LValue target;
            if (!parse_lvalue_path(target)) {
                return false;
            }
            if (execute_ && target.ptr == nullptr) {
                return raise_runtime(13, "Type mismatch", variable_offset);
            }
            where = target.ptr;
            variant = target.variant || (where != nullptr &&
                                         std::holds_alternative<Empty>(*where));
        }
        targets.emplace_back(where, variant);
        skip_horizontal_whitespace();
        if (!consume(',')) {
            return true;
        }
    }
}

std::optional<bool> Interpreter::parse_pds_input_statement(
    const std::size_t statement_offset) {
    const auto start = offset_;
    bool line = false;
    if (consume_keyword("line")) {
        skip_horizontal_whitespace();
        if (!consume_keyword("input")) {
            offset_ = start;
            return std::nullopt;
        }
        line = true;
    } else if (!consume_keyword("input")) {
        return std::nullopt;
    }
    skip_horizontal_whitespace();
    const auto* const instance = current_instance();
    if (at_pds_statement_end() || current() == '#' || current() == '=' ||
        current() == '.' || procedures_.contains("input") ||
        (instance != nullptr && instance->native != nullptr)) {
        offset_ = start;
        return std::nullopt;
    }
    // INPUT; keeps the cursor on the line after Enter.
    const bool keep_line = consume(';');
    skip_horizontal_whitespace();
    std::string prompt;
    bool question = !line;
    if (!at_end() && current() == '"') {
        const auto at = offset_;
        auto text = parse_expression();
        if (!text.has_value()) {
            return false;
        }
        if (const auto* held = std::get_if<std::string>(&*text)) {
            prompt = *held;
        } else {
            set_error("WFC0014", "expected a prompt string", at);
            return false;
        }
        skip_horizontal_whitespace();
        if (consume(',')) {
            question = false;
        } else if (!consume(';')) {
            set_error("WFC0014", "expected ; or , after the prompt", offset_);
            return false;
        }
    }
    const auto targets_offset = offset_;
    std::vector<std::pair<Value*, bool>> targets;
    if (!execute_) {
        return parse_pds_input_targets(targets);
    }
    if (question) {
        prompt += "? ";
    }
    while (true) {
        pds::print_text(prompt);
        std::string typed;
        try {
            typed = pds::read_line();
        } catch (const brun71::BasicError& error) {
            return pds_error(error.number(), statement_offset);
        }
        offset_ = targets_offset;
        if (!parse_pds_input_targets(targets)) {
            return false;
        }
        std::vector<Value> values;
        bool redo = false;
        if (line) {
            if (targets.size() != 1U) {
                set_error("WFC0014", "LINE INPUT takes one variable",
                          targets_offset);
                return false;
            }
            const auto& [where, variant] = targets.front();
            if (!variant && !std::holds_alternative<std::string>(*where)) {
                return raise_runtime(13, "Type mismatch", targets_offset);
            }
            values.emplace_back(std::move(typed));
        } else {
            auto fields = brun71::split_input(typed);
            if (fields && typed.empty()) {
                // An empty line gives every variable its empty value.
                fields->resize(targets.size());
            }
            redo = !fields || fields->size() != targets.size();
            for (std::size_t i = 0; !redo && i < targets.size(); ++i) {
                auto value = input_value(*targets[i].first, targets[i].second,
                                         (*fields)[i]);
                redo = !value.has_value();
                if (value) {
                    values.push_back(std::move(*value));
                }
            }
        }
        if (!keep_line || redo) {
            pds::new_line();
        }
        if (redo) {
            pds::print_text(std::string(brun71::kRedoFromStart));
            pds::new_line();
            continue;
        }
        for (std::size_t i = 0; i < targets.size(); ++i) {
            Value& target = *targets[i].first;
            const bool other_number =
                !targets[i].second && is_number(target) &&
                std::holds_alternative<double>(values[i]) &&
                !std::holds_alternative<double>(target);
            if (other_number && !coerce_numeric_value(values[i], target.index(),
                                                      statement_offset)) {
                // Currency, Decimal: the variable's own conversion.
                return false;
            }
            target = std::move(values[i]);
        }
        return true;
    }
}

std::optional<bool> Interpreter::parse_key_trap_statement(
    const std::size_t start, const std::size_t statement_offset) {
    advance();  // (
    skip_horizontal_whitespace();
    const auto at = offset_;
    auto number = parse_expression();
    if (!number.has_value() || !resolve_default_value(*number, at)) {
        return false;
    }
    skip_horizontal_whitespace();
    if (!consume(')')) {
        set_error("WFC0005", "expected closing parenthesis", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    brun71::TrapState state{};
    if (consume_keyword("on")) {
        state = brun71::TrapState::on;
    } else if (consume_keyword("off")) {
        state = brun71::TrapState::off;
    } else if (consume_keyword("stop")) {
        state = brun71::TrapState::stopped;
    } else {
        offset_ = start;
        return std::nullopt;
    }
    if (!execute_) {
        return true;
    }
    const auto key = pds_integer(*number, at);
    if (!key) {
        return false;
    }
    try {
        pds::key_trap(*key, state);
    } catch (const brun71::BasicError& error) {
        return pds_error(error.number(), statement_offset);
    }
    return true;
}

std::optional<bool> Interpreter::parse_on_key_statement(
    const std::size_t statement_offset) {
    const auto start = offset_;
    if (!consume_keyword("key")) {
        return std::nullopt;
    }
    skip_horizontal_whitespace();
    if (at_end() || current() != '(' || procedures_.contains("key") ||
        find_variable("key").value != nullptr) {
        offset_ = start;
        return std::nullopt;
    }
    advance();  // (
    skip_horizontal_whitespace();
    const auto at = offset_;
    auto number = parse_expression();
    if (!number.has_value() || !resolve_default_value(*number, at)) {
        return false;
    }
    skip_horizontal_whitespace();
    if (!consume(')')) {
        set_error("WFC0005", "expected closing parenthesis", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    if (!consume_keyword("gosub")) {
        set_error("WFC0010", "expected GoSub", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    // GOSUB 0 turns the handler off.
    std::optional<std::string> label;
    const auto label_offset = offset_;
    if (!at_end() && current() == '0' &&
        (offset_ + 1U >= source_.size() ||
         !is_identifier_part(source_[offset_ + 1U]))) {
        advance();
    } else {
        label = parse_label_name();
        if (!label.has_value()) {
            set_error("WFC0011", "expected label after GoSub", label_offset);
            return false;
        }
    }
    if (!execute_) {
        return true;
    }
    const auto key = pds_integer(*number, at);
    if (!key) {
        return false;
    }
    std::size_t target = std::string::npos;
    if (label) {
        target = find_label(*label);
        if (target == std::string::npos) {
            set_error("WFC0301", "label not defined", label_offset);
            return false;
        }
    }
    try {
        pds::key_handler(*key, label.has_value());
    } catch (const brun71::BasicError& error) {
        return pds_error(error.number(), statement_offset);
    }
    if (label) {
        key_handlers_[*key] = target;
    } else {
        key_handlers_.erase(*key);
    }
    return true;
}

bool Interpreter::run_key_events() {
    while (true) {
        std::optional<int> key;
        try {
            key = pds::next_key_event();
        } catch (const brun71::BasicError& error) {
            return pds_error(error.number(), offset_);
        }
        if (!key) {
            return true;
        }
        const auto handler = key_handlers_.find(*key);
        if (handler == key_handlers_.end()) {
            continue;
        }
        pds::begin_event(*key);
        const bool ran = run_gosub(handler->second);
        pds::end_event(*key);
        if (!ran) {
            return false;
        }
    }
}

std::optional<double> Interpreter::pds_number(const Value& value,
                                              const std::size_t offset) {
    if (std::holds_alternative<Empty>(value)) {
        return 0.0;
    }
    if (const auto* flag = std::get_if<bool>(&value)) {
        return *flag ? -1.0 : 0.0;
    }
    if (is_number(value)) {
        return as_double(value);
    }
    static_cast<void>(pds_error(13, offset));
    return std::nullopt;
}

std::optional<bool> Interpreter::parse_pds_sound_statement(
    const std::string_view word, const std::size_t start,
    const std::size_t statement_offset) {
    // The values of the statement, parsed left to right.
    std::vector<std::pair<Value, std::size_t>> values;
    const auto value = [&]() -> bool {
        skip_horizontal_whitespace();
        const auto at = offset_;
        auto parsed = parse_expression();
        if (!parsed.has_value() || !resolve_default_value(*parsed, at)) {
            return false;
        }
        values.emplace_back(std::move(*parsed), at);
        return true;
    };
    const auto expect_comma = [&]() -> bool {
        skip_horizontal_whitespace();
        if (!consume(',')) {
            set_error("WFC0014", "expected comma", offset_);
            return false;
        }
        return true;
    };
    const auto number = [&](const std::size_t index) {
        return pds_number(values[index].first, values[index].second);
    };
    // KEY ON, KEY OFF, KEY LIST, KEY n, text$
    int key_action = 0;  // 1 on, 2 off, 3 list, 4 assign
    if (word == "key") {
        if (consume_keyword("on")) {
            key_action = 1;
        } else if (consume_keyword("off")) {
            key_action = 2;
        } else if (consume_keyword("list")) {
            key_action = 3;
        } else if (at_pds_statement_end()) {
            offset_ = start;
            return std::nullopt;
        } else {
            key_action = 4;
            if (!value() || !expect_comma() || !value()) {
                return false;
            }
        }
    } else if (word == "sleep") {
        if (!at_pds_statement_end() && !value()) {
            return false;
        }
    } else if (word == "sound") {
        if (!value() || !expect_comma() || !value()) {
            return false;
        }
    } else if (!value()) {  // play
        return false;
    }
    if (!execute_) {
        return true;
    }
    try {
        if (word == "sleep") {
            double seconds = 0.0;
            if (!values.empty()) {
                const auto given = number(0);
                if (!given) {
                    return false;
                }
                seconds = std::nearbyint(*given);
            }
            pds::sleep(seconds);
        } else if (word == "sound") {
            const auto frequency = number(0);
            if (!frequency) {
                return false;
            }
            const auto duration = number(1);
            if (!duration) {
                return false;
            }
            pds::sound(*frequency, *duration);
        } else if (word == "play") {
            const auto* music = std::get_if<std::string>(&values[0].first);
            if (music == nullptr) {
                return pds_error(13, values[0].second);
            }
            pds::play(*music);
        } else if (key_action == 1 || key_action == 2) {
            pds::key_line(key_action == 1);
        } else if (key_action == 3) {
            pds::list_keys();
        } else {
            const auto key = pds_integer(values[0].first, values[0].second);
            if (!key) {
                return false;
            }
            const auto* text = std::get_if<std::string>(&values[1].first);
            if (text == nullptr) {
                return pds_error(13, values[1].second);
            }
            pds::assign_key(*key, *text);
        }
    } catch (const brun71::BasicError& error) {
        return pds_error(error.number(), statement_offset);
    }
    return true;
}

std::optional<bool> Interpreter::parse_pds_statement(
    const std::size_t statement_offset) {
    if (const auto input = parse_pds_input_statement(statement_offset)) {
        return input;
    }
    static constexpr std::string_view kWords[] = {"cls",   "locate", "color",
                                                  "width", "view",   "sleep",
                                                  "sound", "play",   "key"};
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
    if (word == "key" && !at_end() && current() == '(' &&
        !procedures_.contains("key") && find_variable("key").value == nullptr &&
        (instance == nullptr || instance->native == nullptr)) {
        return parse_key_trap_statement(start, statement_offset);
    }
    if (not_a_statement) {
        offset_ = start;
        return std::nullopt;
    }
    if (word == "sleep" || word == "sound" || word == "play" || word == "key") {
        return parse_pds_sound_statement(word, start, statement_offset);
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
            pds::redraw_keys();
        } else if (word == "locate") {
            screen.locate(argument(0), argument(1), argument(2), argument(3),
                          argument(4));
        } else if (word == "color") {
            screen.color(argument(0), argument(1), argument(2));
        } else {
            screen.width(argument(0), argument(1));
            pds::redraw_keys();
        }
    });
    if (error != 0) {
        return pds_error(error, statement_offset);
    }
    return true;
}

}  // namespace wfc::detail
