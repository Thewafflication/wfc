#include "wfc/evaluator.hpp"

#include "interpreter/interpreter.hpp"
#include "large_stack.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <exception>
#include <filesystem>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace {

using namespace wfc::detail;

// REQ-0261: joins ` _` line continuations by blanking the underscore and the
// line break (byte offsets are unchanged).
// `[bracketed name]` identifiers: plain names lose the brackets; names with
// spaces or a leading underscore become `wfcb_` identifiers; `[_NewEnum]` maps
// to `NewEnum`.
void rewrite_bracketed_identifiers(std::string& text) {
    if (text.find('[') == std::string::npos) {
        return;
    }
    std::string out;
    out.reserve(text.size());
    bool in_string = false;
    bool in_comment = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        if (ch == '\n') {
            in_string = false;
            in_comment = false;
        } else if (!in_comment) {
            if (ch == '"') {
                in_string = !in_string;
            } else if (!in_string && ch == '\'') {
                in_comment = true;
            } else if (!in_string && ch == '[') {
                const auto close = text.find_first_of("]\n", i + 1);
                if (close != std::string::npos && text[close] == ']' &&
                    close > i + 1) {
                    std::string name = text.substr(i + 1, close - i - 1);
                    bool plain =
                        !std::isdigit(static_cast<unsigned char>(name[0])) &&
                        name[0] != '_';
                    for (const char c : name) {
                        if (!(std::isalnum(static_cast<unsigned char>(c)) ||
                              c == '_')) {
                            plain = false;
                        }
                    }
                    if (name == "_NewEnum") {
                        name = "NewEnum";
                    } else if (!plain) {
                        for (char& c : name) {
                            if (!(std::isalnum(static_cast<unsigned char>(c)) ||
                                  c == '_')) {
                                c = '_';
                            }
                        }
                        name = "wfcb_" + name;
                    }
                    out += name;
                    i = close;
                    continue;
                }
            }
        }
        out.push_back(ch);
    }
    text = std::move(out);
}

// `As IUnknown` / `As IDispatch` are plain object references to this
// interpreter.
void rewrite_object_aliases(std::string& text) {
    const auto lower_at = [&](const std::size_t at,
                              const std::string_view word) {
        if (at + word.size() > text.size()) {
            return false;
        }
        for (std::size_t i = 0; i < word.size(); ++i) {
            const auto ch = static_cast<char>(
                std::tolower(static_cast<unsigned char>(text[at + i])));
            if (ch != word[i]) {
                return false;
            }
        }
        const auto after = at + word.size();
        return after >= text.size() ||
               !(std::isalnum(static_cast<unsigned char>(text[after])) ||
                 text[after] == '_');
    };
    for (std::size_t at = 0; at + 7 < text.size(); ++at) {
        if (text[at] != 'I' && text[at] != 'i') {
            continue;
        }
        const bool unknown = lower_at(at, "iunknown");
        const bool dispatch = !unknown && lower_at(at, "idispatch");
        if (!unknown && !dispatch) {
            continue;
        }
        std::size_t before = at;
        while (before > 0 &&
               (text[before - 1] == ' ' || text[before - 1] == '\t')) {
            --before;
        }
        if (before < 3 || before == at) {
            continue;
        }
        if (!lower_at(before - 2, "as") ||
            (before > 2 &&
             (std::isalnum(static_cast<unsigned char>(text[before - 3])) ||
              text[before - 3] == '_'))) {
            continue;
        }
        text.replace(at, unknown ? 8U : 9U, "Object");
    }
}

void join_line_continuations(std::string& text,
                             std::vector<std::size_t>* joined = nullptr) {
    std::size_t line_start = 0;
    while (line_start < text.size()) {
        std::size_t line_end = text.find('\n', line_start);
        const bool has_newline = line_end != std::string::npos;
        if (!has_newline) {
            line_end = text.size();
        }
        std::size_t content_end = line_end;
        if (content_end > line_start && text[content_end - 1] == '\r') {
            --content_end;
        }
        bool in_string = false;
        bool in_comment = false;
        for (std::size_t i = line_start; i < content_end && !in_comment; ++i) {
            if (text[i] == '"') {
                in_string = !in_string;
            } else if (text[i] == '\'' && !in_string) {
                in_comment = true;
            }
        }
        std::size_t last = content_end;
        while (last > line_start &&
               (text[last - 1] == ' ' || text[last - 1] == '\t')) {
            --last;
        }
        if (!in_string && !in_comment && has_newline && last > line_start &&
            text[last - 1] == '_' &&
            (last - 1 == line_start || text[last - 2] == ' ' ||
             text[last - 2] == '\t')) {
            text[last - 1] = ' ';
            if (joined != nullptr) {
                joined->push_back(line_end);
            }
            text[line_end] = ' ';
            if (line_end > line_start && text[line_end - 1] == '\r') {
                text[line_end - 1] = ' ';
            }
        }
        line_start = has_newline ? line_end + 1 : text.size();
    }
}

// REQ-0240: conditional compilation. Returns `source` with every
// `#If/#ElseIf/#Else/#End If/#Const` directive line, and every line inside
// an inactive branch, blanked to spaces (line breaks kept, so byte offsets
// are unchanged). `error`/`error_offset` are set on malformed directives.
class ConditionalPreprocessor final {
public:
    [[nodiscard]] std::optional<std::string> run(const std::string_view source,
                                                 std::string& error,
                                                 std::size_t& error_offset) {
        std::string out(source);
        struct Frame {
            bool parent_active{};
            bool taken{};
            bool active{};
        };
        std::vector<Frame> stack;
        const auto active_now = [&] {
            return stack.empty() || stack.back().active;
        };
        std::size_t position = 0;
        while (position < out.size()) {
            std::size_t line_end = out.find('\n', position);
            if (line_end == std::string::npos) {
                line_end = out.size();
            }
            std::size_t i = position;
            while (i < line_end && (out[i] == ' ' || out[i] == '\t')) {
                ++i;
            }
            const bool directive =
                i < line_end && out[i] == '#' && i + 1 < line_end &&
                std::isalpha(static_cast<unsigned char>(out[i + 1])) != 0;
            if (directive) {
                std::string text = out.substr(i + 1, line_end - i - 1);
                if (!text.empty() && text.back() == '\r') {
                    text.pop_back();
                }
                if (const auto c = text.find('\''); c != std::string::npos) {
                    text.resize(c);
                }
                std::string lower;
                for (const char ch : text) {
                    lower.push_back(ascii_lower(ch));
                }
                const auto starts = [&](const std::string_view w) {
                    return lower.compare(0, w.size(), w) == 0 &&
                           (lower.size() == w.size() ||
                            !is_identifier_part(lower[w.size()]));
                };
                const auto condition_of =
                    [&](std::size_t skip) -> std::optional<bool> {
                    std::string expr = text.substr(skip);
                    std::string lexpr = lower.substr(skip);
                    const auto then = lexpr.rfind("then");
                    if (then == std::string::npos) {
                        error = "expected Then";
                        error_offset = position;
                        return std::nullopt;
                    }
                    expr.resize(then);
                    const auto value = evaluate_expression(expr, error);
                    if (!value.has_value()) {
                        error_offset = position;
                        return std::nullopt;
                    }
                    return *value != 0;
                };
                if (starts("if")) {
                    const bool parent = active_now();
                    const auto condition =
                        parent ? condition_of(2) : std::optional<bool>{false};
                    if (!condition.has_value()) {
                        return std::nullopt;
                    }
                    stack.push_back({parent, *condition, parent && *condition});
                } else if (starts("elseif")) {
                    if (stack.empty()) {
                        error = "#ElseIf without #If";
                        error_offset = position;
                        return std::nullopt;
                    }
                    auto& frame = stack.back();
                    if (frame.parent_active && !frame.taken) {
                        const auto condition = condition_of(6);
                        if (!condition.has_value()) {
                            return std::nullopt;
                        }
                        frame.active = *condition;
                        frame.taken = *condition;
                    } else {
                        frame.active = false;
                    }
                } else if (starts("else")) {
                    if (stack.empty()) {
                        error = "#Else without #If";
                        error_offset = position;
                        return std::nullopt;
                    }
                    auto& frame = stack.back();
                    frame.active = frame.parent_active && !frame.taken;
                    frame.taken = true;
                } else if (starts("end")) {
                    if (stack.empty()) {
                        error = "#End If without #If";
                        error_offset = position;
                        return std::nullopt;
                    }
                    stack.pop_back();
                } else if (starts("const")) {
                    if (active_now()) {
                        const auto equals = text.find('=', 5);
                        if (equals == std::string::npos) {
                            error = "expected = in #Const";
                            error_offset = position;
                            return std::nullopt;
                        }
                        std::string name;
                        for (std::size_t k = 5; k < equals; ++k) {
                            if (text[k] != ' ' && text[k] != '\t') {
                                name.push_back(ascii_lower(text[k]));
                            }
                        }
                        const auto value =
                            evaluate_expression(text.substr(equals + 1), error);
                        if (!value.has_value() || name.empty()) {
                            if (name.empty()) {
                                error = "expected #Const name";
                            }
                            error_offset = position;
                            return std::nullopt;
                        }
                        constants_[name] = *value;
                    }
                } else {
                    error = "unknown conditional compilation directive";
                    error_offset = position;
                    return std::nullopt;
                }
                for (std::size_t k = position; k < line_end; ++k) {
                    if (out[k] != '\r') {
                        out[k] = ' ';
                    }
                }
            } else if (!active_now()) {
                for (std::size_t k = position; k < line_end; ++k) {
                    if (out[k] != '\r') {
                        out[k] = ' ';
                    }
                }
            }
            position = line_end + 1;
        }
        if (!stack.empty()) {
            error = "missing #End If";
            error_offset = out.size();
            return std::nullopt;
        }
        return out;
    }

private:
    std::unordered_map<std::string, long long> constants_{
        {"win32", -1}, {"win16", 0}, {"mac", 0},
        {"vba6", -1},  {"vba7", -1}, {"vbaver", 6}};

    // Recursive-descent evaluation over: Or/Xor < And < Not < comparison <
    // + - < * / Mod < unary minus < primary.
    struct Parser {
        ConditionalPreprocessor& owner;
        std::string text;
        std::size_t i{};
        std::string& error;
        void ws() {
            while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) {
                ++i;
            }
        }
        bool word(const std::string_view w) {
            ws();
            if (text.size() - i < w.size()) {
                return false;
            }
            for (std::size_t k = 0; k < w.size(); ++k) {
                if (ascii_lower(text[i + k]) != w[k]) {
                    return false;
                }
            }
            if (i + w.size() < text.size() &&
                is_identifier_part(text[i + w.size()])) {
                return false;
            }
            i += w.size();
            return true;
        }
        std::optional<long long> or_expr() {
            auto left = and_expr();
            while (left) {
                if (word("or")) {
                    auto r = and_expr();
                    if (!r) {
                        return r;
                    }
                    left = *left | *r;
                } else if (word("xor")) {
                    auto r = and_expr();
                    if (!r) {
                        return r;
                    }
                    left = *left ^ *r;
                } else {
                    break;
                }
            }
            return left;
        }
        std::optional<long long> and_expr() {
            auto left = not_expr();
            while (left && word("and")) {
                auto r = not_expr();
                if (!r) {
                    return r;
                }
                left = *left & *r;
            }
            return left;
        }
        std::optional<long long> not_expr() {
            if (word("not")) {
                auto r = not_expr();
                if (!r) {
                    return r;
                }
                return ~*r;
            }
            return cmp_expr();
        }
        std::optional<long long> cmp_expr() {
            auto left = add_expr();
            if (!left) {
                return left;
            }
            ws();
            const char a = i < text.size() ? text[i] : '\0';
            const char b = i + 1 < text.size() ? text[i + 1] : '\0';
            int op = 0;  // 1 = 2 <> 3 < 4 > 5 <= 6 >=
            if (a == '<' && b == '>') {
                op = 2;
                i += 2;
            } else if (a == '<' && b == '=') {
                op = 5;
                i += 2;
            } else if (a == '>' && b == '=') {
                op = 6;
                i += 2;
            } else if (a == '=') {
                op = 1;
                ++i;
            } else if (a == '<') {
                op = 3;
                ++i;
            } else if (a == '>') {
                op = 4;
                ++i;
            }
            if (op == 0) {
                return left;
            }
            auto right = add_expr();
            if (!right) {
                return right;
            }
            const long long l = *left, r = *right;
            const bool result = op == 1   ? l == r
                                : op == 2 ? l != r
                                : op == 3 ? l < r
                                : op == 4 ? l > r
                                : op == 5 ? l <= r
                                          : l >= r;
            return result ? -1 : 0;
        }
        std::optional<long long> add_expr() {
            auto left = mul_expr();
            while (left) {
                ws();
                if (i < text.size() && (text[i] == '+' || text[i] == '-')) {
                    const char op = text[i++];
                    auto r = mul_expr();
                    if (!r) {
                        return r;
                    }
                    left = op == '+' ? *left + *r : *left - *r;
                } else {
                    break;
                }
            }
            return left;
        }
        std::optional<long long> mul_expr() {
            auto left = unary();
            while (left) {
                ws();
                if (i < text.size() && text[i] == '*') {
                    ++i;
                    auto r = unary();
                    if (!r) {
                        return r;
                    }
                    left = *left * *r;
                } else if (word("mod")) {
                    auto r = unary();
                    if (!r) {
                        return r;
                    }
                    if (*r == 0) {
                        error = "division by zero";
                        return std::nullopt;
                    }
                    left = *left % *r;
                } else {
                    break;
                }
            }
            return left;
        }
        std::optional<long long> unary() {
            ws();
            if (i < text.size() && text[i] == '-') {
                ++i;
                auto r = unary();
                if (!r) {
                    return r;
                }
                return -*r;
            }
            return primary();
        }
        std::optional<long long> primary() {
            ws();
            if (i >= text.size()) {
                error = "expected expression";
                return std::nullopt;
            }
            if (text[i] == '(') {
                ++i;
                auto r = or_expr();
                if (!r) {
                    return r;
                }
                ws();
                if (i >= text.size() || text[i] != ')') {
                    error = "expected closing parenthesis";
                    return std::nullopt;
                }
                ++i;
                return r;
            }
            if (std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
                long long v = 0;
                while (i < text.size() &&
                       std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
                    v = v * 10 + (text[i++] - '0');
                }
                return v;
            }
            if (is_identifier_start(text[i])) {
                std::string name;
                while (i < text.size() && is_identifier_part(text[i])) {
                    name.push_back(ascii_lower(text[i++]));
                }
                if (name == "true") {
                    return -1;
                }
                if (name == "false") {
                    return 0;
                }
                const auto it = owner.constants_.find(name);
                return it == owner.constants_.end() ? 0 : it->second;
            }
            error = "expected expression";
            return std::nullopt;
        }
    };

    std::optional<long long> evaluate_expression(const std::string& text,
                                                 std::string& error) {
        Parser parser{*this, text, 0, error};
        auto value = parser.or_expr();
        if (!value) {
            return value;
        }
        parser.ws();
        if (parser.i != text.size()) {
            error = "unexpected text in conditional expression";
            return std::nullopt;
        }
        return value;
    }
};

[[nodiscard]] wfc::Evaluation failure_for_directive(const std::string& message,
                                                    const std::size_t offset) {
    return failure("WFC0310", message, offset);
}

}  // namespace

namespace wfc {

namespace {

// Deep VB recursion needs far more native stack than a default main thread
// has (1 MB on Windows), so the interpreter runs on a thread with a large
// reserved (not committed) stack. `depth_limit` is the procedure nesting the
// stack is sized for; 0 means the thread could not be created and the work
// ran inline.
constexpr std::size_t kLargeStackBytes =
    sizeof(void*) >= 8U ? (std::size_t{512} << 20U) : (std::size_t{160} << 20U);
constexpr std::size_t kLargeStackDepth = sizeof(void*) >= 8U ? 5000U : 1500U;

struct LargeStackTask {
    std::function<void()> work;
    std::exception_ptr failure;
};

void run_large_stack_task(void* raw) {
    auto& task = *static_cast<LargeStackTask*>(raw);
    try {
        task.work();
    } catch (...) {
        task.failure = std::current_exception();
    }
}

// Runs `work` on a large-stack thread; returns false if no such thread could
// be started (the caller then runs `work` itself with the small limit).
[[nodiscard]] bool run_on_large_stack(std::function<void()> work) {
    LargeStackTask task{std::move(work), nullptr};
    if (!detail::run_on_thread_with_stack(&run_large_stack_task, &task,
                                          kLargeStackBytes)) {
        return false;
    }
    if (task.failure) {
        std::rethrow_exception(task.failure);
    }
    return true;
}

// Evaluates with a deep-recursion limit matching the stack it runs on.
[[nodiscard]] Evaluation evaluate_interpreter(
    const std::function<Evaluation(std::size_t, const char*, std::size_t)>&
        run) {
    Evaluation result;
    // An unexpected exception (a bug) is reported as a diagnostic instead of
    // terminating the process.
    try {
        const bool ran = run_on_large_stack([&] {
            char stack_top = 0;
            result = run(kLargeStackDepth, &stack_top,
                         kLargeStackBytes - (std::size_t{24} << 20U));
        });
        if (!ran) {
            result = run(64U, nullptr, 0U);
        }
    } catch (const std::exception& exception) {
        result = detail::failure(
            "WFC0900", std::string("internal error: ") + exception.what(), 0U);
    } catch (...) {
        result = detail::failure("WFC0900", "internal error", 0U);
    }
    return result;
}

}  // namespace

// Fills in `error_line`/`error_column` from the failing module's processed text
// (line breaks removed by continuation joining are counted back in).
void attach_position(Evaluation& result, const std::string_view text,
                     const std::vector<std::size_t>& joined,
                     std::string module) {
    if (result.success) {
        return;
    }
    const auto offset = std::min(result.error_offset, text.size());
    std::size_t line = 1;
    std::size_t line_start = 0;
    for (std::size_t i = 0; i < offset; ++i) {
        if (text[i] == '\n') {
            ++line;
            line_start = i + 1;
        }
    }
    for (const auto position : joined) {
        if (position < offset) {
            ++line;
            line_start = std::max(line_start, position + 1);
        }
    }
    result.error_line = line;
    result.error_column = offset - line_start + 1;
    result.error_module = std::move(module);
}

Evaluation evaluate_program(const std::string_view source) {
    std::string error;
    std::size_t error_offset{};
    auto processed = ConditionalPreprocessor{}.run(source, error, error_offset);
    if (!processed.has_value()) {
        return failure_for_directive(error, error_offset);
    }
    rewrite_object_aliases(*processed);
    rewrite_bracketed_identifiers(*processed);
    std::vector<std::size_t> joined;
    join_line_continuations(*processed, &joined);
    return evaluate_interpreter([&](const std::size_t depth, const char* base,
                                    const std::size_t budget) {
        Interpreter interpreter(*processed);
        interpreter.set_max_procedure_depth(depth);
        interpreter.set_stack_budget(base, budget);
        auto result = interpreter.evaluate();
        if (interpreter.failing_module_name().empty()) {
            attach_position(result, *processed, joined, {});
        }
        return result;
    });
}

Evaluation evaluate_program(const std::string_view source,
                            const std::vector<ClassModuleSource>& classes) {
    return evaluate_program(source, classes, EvaluationOptions{});
}

Evaluation evaluate_program(const std::string_view source,
                            const std::vector<ClassModuleSource>& classes,
                            const EvaluationOptions& options) {
    std::string error;
    std::size_t error_offset{};
    auto processed = ConditionalPreprocessor{}.run(source, error, error_offset);
    if (!processed.has_value()) {
        return failure_for_directive(error, error_offset);
    }
    rewrite_object_aliases(*processed);
    rewrite_bracketed_identifiers(*processed);
    std::vector<std::size_t> joined;
    join_line_continuations(*processed, &joined);
    std::vector<std::string> processed_classes;
    std::vector<std::vector<std::size_t>> class_joined(classes.size());
    processed_classes.reserve(classes.size());
    for (std::size_t index = 0; index < classes.size(); ++index) {
        auto text = ConditionalPreprocessor{}.run(classes[index].source, error,
                                                  error_offset);
        if (!text.has_value()) {
            return failure_for_directive(error, error_offset);
        }
        rewrite_object_aliases(*text);
        rewrite_bracketed_identifiers(*text);
        join_line_continuations(*text, &class_joined[index]);
        processed_classes.push_back(std::move(*text));
    }
    std::vector<ClassModuleSource> rewritten;
    rewritten.reserve(classes.size());
    for (std::size_t index = 0; index < classes.size(); ++index) {
        rewritten.push_back({classes[index].name, processed_classes[index]});
    }
    return evaluate_interpreter([&](const std::size_t depth, const char* base,
                                    const std::size_t budget) {
        Interpreter interpreter(*processed, rewritten);
        interpreter.set_vb_number_spacing(options.vb6_print_spacing);
        interpreter.set_app_properties(options.app_properties);
        interpreter.set_max_procedure_depth(depth);
        interpreter.set_stack_budget(base, budget);
        auto result = interpreter.evaluate();
        const auto module = interpreter.failing_module_name();
        if (module.empty()) {
            attach_position(result, *processed, joined, {});
        } else {
            for (std::size_t index = 0; index < rewritten.size(); ++index) {
                std::string lowered_a = module;
                std::string lowered_b = rewritten[index].name;
                for (auto& ch : lowered_a) {
                    ch = static_cast<char>(
                        std::tolower(static_cast<unsigned char>(ch)));
                }
                for (auto& ch : lowered_b) {
                    ch = static_cast<char>(
                        std::tolower(static_cast<unsigned char>(ch)));
                }
                if (lowered_a == lowered_b) {
                    attach_position(result, processed_classes[index],
                                    class_joined[index], rewritten[index].name);
                    break;
                }
            }
        }
        return result;
    });
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
        if (offset == source.size() ||
            ascii_lower(source[offset]) != expected) {
            return failure("WFC0001", "expected Print statement",
                           statement_offset);
        }
        ++offset;
    }
    if (offset < source.size() && is_identifier_part(source[offset])) {
        return failure("WFC0001", "expected Print statement", statement_offset);
    }
    return Interpreter(source, false).evaluate();
}

}  // namespace wfc
