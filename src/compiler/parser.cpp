#include "parser.hpp"

#include <cstdlib>
#include <utility>

#include "lexer.hpp"

namespace wfc::compiler {

namespace {

// A syntax error or an unsupported construct unwinds to `parse_module`.
struct ParseFailure final {
    std::string message;
    Position at;
};

class Parser final {
public:
    explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    Module parse() {
        Module module;
        skip_ends();
        while (!at_end_of_file()) {
            if (accept_word("option")) {
                option_statement();
            } else if (is_word("dim") || is_word("private") ||
                       is_word("public") || is_word("friend") ||
                       is_word("static") || is_word("sub") ||
                       is_word("function")) {
                declaration_or_procedure(module);
            } else {
                fail("expected a declaration, Sub or Function");
            }
            skip_ends();
        }
        return module;
    }

private:
    // ----- Token helpers ---------------------------------------------------

    [[nodiscard]] const Token& current() const { return tokens_[index_]; }
    [[nodiscard]] const Token& ahead(const std::size_t count) const {
        const auto at = index_ + count;
        return at < tokens_.size() ? tokens_[at] : tokens_.back();
    }
    [[nodiscard]] Position here() const {
        return Position{current().line, current().column};
    }
    [[nodiscard]] bool at_end_of_file() const {
        return current().kind == TokenKind::end_of_file;
    }
    [[nodiscard]] bool at_end_of_statement() const {
        return current().kind == TokenKind::end_of_statement ||
               current().kind == TokenKind::end_of_file;
    }
    void advance() {
        if (!at_end_of_file()) {
            ++index_;
        }
    }
    void skip_ends() {
        while (current().kind == TokenKind::end_of_statement) {
            advance();
        }
    }
    [[noreturn]] void fail(const std::string& message) const {
        throw ParseFailure{message, here()};
    }
    [[nodiscard]] bool is_word(const std::string_view word) const {
        return current().kind == TokenKind::identifier &&
               current().text == word;
    }
    [[nodiscard]] bool is_symbol(const std::string_view symbol) const {
        return current().kind == TokenKind::symbol && current().text == symbol;
    }
    bool accept_word(const std::string_view word) {
        if (is_word(word)) {
            advance();
            return true;
        }
        return false;
    }
    bool accept_symbol(const std::string_view symbol) {
        if (is_symbol(symbol)) {
            advance();
            return true;
        }
        return false;
    }
    void expect_word(const std::string_view word) {
        if (!accept_word(word)) {
            fail("expected " + std::string(word));
        }
    }
    void expect_symbol(const std::string_view symbol) {
        if (!accept_symbol(symbol)) {
            fail("expected '" + std::string(symbol) + "'");
        }
    }
    void expect_end_of_statement() {
        if (!at_end_of_statement()) {
            fail("expected the end of the statement");
        }
        skip_ends();
    }

    // ----- Declarations ----------------------------------------------------

    void option_statement() {
        if (accept_word("explicit")) {
            expect_end_of_statement();
            return;
        }
        fail("Option " + current().spelling + " is not compiled yet");
    }

    [[nodiscard]] static VbType suffix_type(const std::string& name) {
        switch (name.back()) {
            case '%':
                return VbType::integer;
            case '&':
                return VbType::long_integer;
            case '!':
                return VbType::single_precision;
            case '#':
                return VbType::double_precision;
            case '$':
                return VbType::string;
            default:
                return VbType::none;
        }
    }

    [[nodiscard]] static std::string without_suffix(std::string name) {
        if (!name.empty() && suffix_type(name) != VbType::none) {
            name.pop_back();
        } else if (!name.empty() && name.back() == '@') {
            name.pop_back();
        }
        return name;
    }

    [[nodiscard]] VbType type_name() {
        if (current().kind != TokenKind::identifier) {
            fail("expected a type name");
        }
        const std::string& name = current().text;
        VbType type = VbType::none;
        if (name == "integer") {
            type = VbType::integer;
        } else if (name == "long") {
            type = VbType::long_integer;
        } else if (name == "boolean") {
            type = VbType::boolean;
        } else if (name == "single") {
            type = VbType::single_precision;
        } else if (name == "double") {
            type = VbType::double_precision;
        } else if (name == "string") {
            type = VbType::string;
        } else {
            fail("the type " + current().spelling + " is not compiled yet");
        }
        advance();
        return type;
    }

    [[nodiscard]] Declaration declarator() {
        if (current().kind != TokenKind::identifier) {
            fail("expected a variable name");
        }
        Declaration declaration;
        declaration.at = here();
        const std::string spelled = current().text;
        declaration.name = without_suffix(spelled);
        const VbType by_suffix = suffix_type(spelled);
        advance();
        if (is_symbol("(")) {
            fail("arrays are not compiled yet");
        }
        if (accept_word("as")) {
            if (is_word("new")) {
                fail("objects are not compiled yet");
            }
            declaration.type = type_name();
            declaration.type_given = true;
        } else if (by_suffix != VbType::none) {
            declaration.type = by_suffix;
            declaration.type_given = true;
        }
        return declaration;
    }

    void declaration_or_procedure(Module& module) {
        bool is_public = true;
        bool saw_scope = false;
        while (is_word("private") || is_word("public") || is_word("friend") ||
               is_word("static")) {
            if (is_word("private")) {
                is_public = false;
            }
            if (is_word("static")) {
                fail("Static is not compiled yet");
            }
            saw_scope = true;
            advance();
        }
        if (is_word("sub") || is_word("function")) {
            procedure(module, is_public);
            return;
        }
        if (accept_word("dim") || saw_scope) {
            do {
                module.globals.push_back(declarator());
            } while (accept_symbol(","));
            expect_end_of_statement();
            return;
        }
        fail("expected a declaration, Sub or Function");
    }

    void procedure(Module& module, const bool is_public) {
        Procedure result;
        result.is_public = is_public;
        result.is_function = is_word("function");
        result.at = here();
        advance();
        if (current().kind != TokenKind::identifier) {
            fail("expected the procedure name");
        }
        result.spelling = current().spelling;
        result.name = without_suffix(current().text);
        const VbType by_suffix = suffix_type(current().text);
        advance();
        if (accept_symbol("(")) {
            if (!accept_symbol(")")) {
                do {
                    result.parameters.push_back(parameter());
                } while (accept_symbol(","));
                expect_symbol(")");
            }
        }
        if (result.is_function) {
            // Without `As` a Function returns a Variant, which the compiler
            // does not support yet: the return type stays `none`.
            result.return_type = by_suffix;
            if (accept_word("as")) {
                result.return_type = type_name();
            }
        }
        expect_end_of_statement();
        result.body = block({"end"});
        expect_word("end");
        if (!accept_word(result.is_function ? "function" : "sub")) {
            fail(result.is_function ? "expected End Function"
                                    : "expected End Sub");
        }
        expect_end_of_statement();
        module.procedures.push_back(std::move(result));
    }

    [[nodiscard]] Parameter parameter() {
        Parameter result;
        if (is_word("optional") || is_word("paramarray")) {
            fail("Optional and ParamArray parameters are not compiled yet");
        }
        if (accept_word("byval")) {
            result.by_value = true;
        } else {
            accept_word("byref");
        }
        if (current().kind != TokenKind::identifier) {
            fail("expected a parameter name");
        }
        const std::string spelled = current().text;
        result.name = without_suffix(spelled);
        result.type = suffix_type(spelled);
        advance();
        if (is_symbol("(")) {
            fail("array parameters are not compiled yet");
        }
        if (accept_word("as")) {
            result.type = type_name();
        }
        return result;
    }

    // ----- Statements ------------------------------------------------------

    // Reads statements until one of `stoppers` starts a statement. `end`
    // stops only when it is followed by If/Sub/Function/Select/With.
    [[nodiscard]] std::vector<StmtPtr> block(
        const std::vector<std::string>& stoppers) {
        std::vector<StmtPtr> result;
        skip_ends();
        while (!at_end_of_file() && !at_stopper(stoppers)) {
            result.push_back(statement());
            skip_ends();
        }
        if (at_end_of_file()) {
            fail("the block is not closed");
        }
        return result;
    }

    [[nodiscard]] bool at_stopper(
        const std::vector<std::string>& stoppers) const {
        if (current().kind != TokenKind::identifier) {
            return false;
        }
        for (const auto& word : stoppers) {
            if (current().text != word) {
                continue;
            }
            if (word == "end") {
                const auto& next = ahead(1);
                return next.kind == TokenKind::identifier &&
                       (next.text == "if" || next.text == "sub" ||
                        next.text == "function" || next.text == "select" ||
                        next.text == "with");
            }
            return true;
        }
        return false;
    }

    [[nodiscard]] StmtPtr make(const StmtKind kind) const {
        auto result = std::make_unique<Stmt>();
        result->kind = kind;
        result->at = here();
        return result;
    }

    [[nodiscard]] StmtPtr statement() {
        if (current().kind != TokenKind::identifier) {
            fail("expected a statement");
        }
        const std::string word = current().text;
        if (word == "dim") {
            return dim_statement();
        }
        if (word == "print") {
            return print_statement();
        }
        if (word == "if") {
            return if_statement();
        }
        if (word == "for") {
            return for_statement();
        }
        if (word == "do") {
            return do_statement();
        }
        if (word == "while") {
            return while_statement();
        }
        if (word == "exit") {
            return exit_statement();
        }
        if (word == "end") {
            auto result = make(StmtKind::exit);
            advance();
            result->text = "end";
            return result;
        }
        if (word == "call") {
            advance();
            return call_statement(true);
        }
        if (word == "let") {
            advance();
            return assignment_or_call();
        }
        if (word == "select" || word == "with" || word == "goto" ||
            word == "gosub" || word == "on" || word == "set" ||
            word == "redim" || word == "const" || word == "open" ||
            word == "close" || word == "static" || word == "erase" ||
            word == "randomize" || word == "line" || word == "raiseevent") {
            fail("the statement " + current().spelling +
                 " is not compiled yet");
        }
        return assignment_or_call();
    }

    [[nodiscard]] StmtPtr dim_statement() {
        auto result = make(StmtKind::dim);
        advance();
        do {
            result->declarations.push_back(declarator());
        } while (accept_symbol(","));
        return result;
    }

    [[nodiscard]] StmtPtr print_statement() {
        auto result = make(StmtKind::print);
        advance();
        while (!at_end_of_statement() && !is_word("else")) {
            result->items.push_back(expression());
            if (accept_symbol(";")) {
                result->newline =
                    at_end_of_statement() || is_word("else") ? false : true;
                continue;
            }
            if (is_symbol(",")) {
                fail("a comma in Print (tab zones) is not compiled yet");
            }
            break;
        }
        return result;
    }

    [[nodiscard]] StmtPtr assignment_or_call() {
        if (current().kind != TokenKind::identifier) {
            fail("expected a statement");
        }
        if (ahead(1).kind == TokenKind::symbol && ahead(1).text == "=") {
            auto result = make(StmtKind::assign);
            result->text = without_suffix(current().text);
            advance();
            advance();
            result->expr = expression();
            return result;
        }
        return call_statement(false);
    }

    [[nodiscard]] StmtPtr call_statement(const bool with_call_keyword) {
        auto result = make(StmtKind::call);
        if (current().kind != TokenKind::identifier) {
            fail("expected the procedure name");
        }
        result->text = without_suffix(current().text);
        advance();
        if (is_symbol(".")) {
            fail("members of objects are not compiled yet");
        }
        if (accept_symbol("(")) {
            if (!accept_symbol(")")) {
                do {
                    result->items.push_back(expression());
                } while (accept_symbol(","));
                expect_symbol(")");
            }
        } else if (!with_call_keyword) {
            // `name arg, arg`: arguments without parentheses.
            while (!at_end_of_statement() && !is_word("else")) {
                result->items.push_back(expression());
                if (!accept_symbol(",")) {
                    break;
                }
            }
        }
        return result;
    }

    [[nodiscard]] StmtPtr if_statement() {
        auto result = make(StmtKind::if_block);
        advance();
        result->conditions.push_back(expression());
        expect_word("then");
        if (!at_end_of_statement()) {
            // The single-line form: If c Then stmt [Else stmt]
            std::vector<StmtPtr> then_part;
            then_part.push_back(statement());
            result->blocks.push_back(std::move(then_part));
            if (accept_word("else")) {
                std::vector<StmtPtr> else_part;
                else_part.push_back(statement());
                result->blocks.push_back(std::move(else_part));
                result->has_else = true;
            }
            return result;
        }
        result->blocks.push_back(block({"elseif", "else", "end"}));
        while (true) {
            if (accept_word("elseif")) {
                result->conditions.push_back(expression());
                expect_word("then");
                result->blocks.push_back(block({"elseif", "else", "end"}));
            } else if (accept_word("else")) {
                result->blocks.push_back(block({"end"}));
                result->has_else = true;
            } else {
                break;
            }
        }
        expect_word("end");
        expect_word("if");
        return result;
    }

    [[nodiscard]] StmtPtr for_statement() {
        auto result = make(StmtKind::for_loop);
        advance();
        if (is_word("each")) {
            fail("For Each is not compiled yet");
        }
        if (current().kind != TokenKind::identifier) {
            fail("expected the loop variable");
        }
        result->text = without_suffix(current().text);
        advance();
        expect_symbol("=");
        result->expr = expression();
        expect_word("to");
        result->limit = expression();
        if (accept_word("step")) {
            result->step = expression();
        }
        result->body = block({"next"});
        expect_word("next");
        if (current().kind == TokenKind::identifier) {
            advance();  // the optional variable after Next
        }
        return result;
    }

    [[nodiscard]] StmtPtr do_statement() {
        auto result = make(StmtKind::do_loop);
        advance();
        bool tested_first = false;
        if (accept_word("while")) {
            result->expr = expression();
            tested_first = true;
        } else if (accept_word("until")) {
            result->expr = expression();
            result->until = true;
            tested_first = true;
        }
        result->body = block({"loop"});
        expect_word("loop");
        if (!tested_first) {
            if (accept_word("while")) {
                result->expr = expression();
                result->test_after = true;
            } else if (accept_word("until")) {
                result->expr = expression();
                result->until = true;
                result->test_after = true;
            }
        }
        return result;
    }

    [[nodiscard]] StmtPtr while_statement() {
        auto result = make(StmtKind::while_loop);
        advance();
        result->expr = expression();
        result->body = block({"wend"});
        expect_word("wend");
        return result;
    }

    [[nodiscard]] StmtPtr exit_statement() {
        auto result = make(StmtKind::exit);
        advance();
        if (is_word("sub") || is_word("function") || is_word("for") ||
            is_word("do")) {
            result->text = current().text;
            advance();
            return result;
        }
        fail("expected Sub, Function, For or Do after Exit");
    }

    // ----- Expressions -----------------------------------------------------

    [[nodiscard]] ExprPtr make_expr(const ExprKind kind,
                                    const Position at) const {
        auto result = std::make_unique<Expr>();
        result->kind = kind;
        result->at = at;
        return result;
    }

    [[nodiscard]] ExprPtr binary(std::string op, ExprPtr left, ExprPtr right,
                                 const Position at) const {
        auto result = make_expr(ExprKind::binary, at);
        result->text = std::move(op);
        result->operands.push_back(std::move(left));
        result->operands.push_back(std::move(right));
        return result;
    }

    [[nodiscard]] ExprPtr expression() { return xor_expression(); }

    [[nodiscard]] ExprPtr xor_expression() {
        auto left = or_expression();
        while (is_word("xor")) {
            const auto at = here();
            advance();
            left = binary("xor", std::move(left), or_expression(), at);
        }
        if (is_word("eqv") || is_word("imp")) {
            fail(current().spelling + " is not compiled yet");
        }
        return left;
    }

    [[nodiscard]] ExprPtr or_expression() {
        auto left = and_expression();
        while (is_word("or")) {
            const auto at = here();
            advance();
            left = binary("or", std::move(left), and_expression(), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr and_expression() {
        auto left = not_expression();
        while (is_word("and")) {
            const auto at = here();
            advance();
            left = binary("and", std::move(left), not_expression(), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr not_expression() {
        if (is_word("not")) {
            const auto at = here();
            advance();
            auto result = make_expr(ExprKind::unary, at);
            result->text = "not";
            result->operands.push_back(not_expression());
            return result;
        }
        return comparison();
    }

    [[nodiscard]] ExprPtr comparison() {
        auto left = concatenation();
        while (current().kind == TokenKind::symbol &&
               (current().text == "=" || current().text == "<>" ||
                current().text == "<" || current().text == ">" ||
                current().text == "<=" || current().text == ">=")) {
            const auto at = here();
            const std::string op = current().text;
            advance();
            left = binary(op, std::move(left), concatenation(), at);
        }
        if (is_word("like") || is_word("is")) {
            fail(current().spelling + " is not compiled yet");
        }
        return left;
    }

    [[nodiscard]] ExprPtr concatenation() {
        auto left = additive();
        while (is_symbol("&")) {
            const auto at = here();
            advance();
            left = binary("&", std::move(left), additive(), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr additive() {
        auto left = modulus();
        while (is_symbol("+") || is_symbol("-")) {
            const auto at = here();
            const std::string op = current().text;
            advance();
            left = binary(op, std::move(left), modulus(), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr modulus() {
        auto left = integer_division();
        while (is_word("mod")) {
            const auto at = here();
            advance();
            left = binary("mod", std::move(left), integer_division(), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr integer_division() {
        auto left = multiplicative();
        while (is_symbol("\\")) {
            const auto at = here();
            advance();
            left = binary("\\", std::move(left), multiplicative(), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr multiplicative() {
        auto left = unary();
        while (is_symbol("*") || is_symbol("/")) {
            const auto at = here();
            const std::string op = current().text;
            advance();
            left = binary(op, std::move(left), unary(), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr unary() {
        if (is_symbol("-") || is_symbol("+")) {
            const auto at = here();
            const bool negate = is_symbol("-");
            advance();
            auto operand = unary();
            if (!negate) {
                return operand;
            }
            auto result = make_expr(ExprKind::unary, at);
            result->text = "-";
            result->operands.push_back(std::move(operand));
            return result;
        }
        return power();
    }

    [[nodiscard]] ExprPtr power() {
        auto left = primary();
        if (is_symbol("^")) {
            fail("the operator ^ is not compiled yet");
        }
        return left;
    }

    [[nodiscard]] ExprPtr primary() {
        const auto at = here();
        const Token token = current();
        if (token.kind == TokenKind::integer) {
            advance();
            auto result = make_expr(ExprKind::integer_literal, at);
            result->number = token.number;
            result->long_literal = token.long_literal;
            return result;
        }
        if (token.kind == TokenKind::real) {
            advance();
            auto result = make_expr(ExprKind::real_literal, at);
            result->real = std::strtod(token.text.c_str(), nullptr);
            result->single_literal = token.single_literal;
            return result;
        }
        if (token.kind == TokenKind::string) {
            advance();
            auto result = make_expr(ExprKind::string_literal, at);
            result->text = token.text;
            return result;
        }
        if (token.kind == TokenKind::symbol && token.text == "(") {
            advance();
            auto inner = expression();
            expect_symbol(")");
            return inner;
        }
        if (token.kind == TokenKind::identifier) {
            if (token.text == "true" || token.text == "false") {
                advance();
                auto result = make_expr(ExprKind::boolean_literal, at);
                result->number = token.text == "true" ? -1 : 0;
                return result;
            }
            advance();
            if (is_symbol(".")) {
                fail("members of objects are not compiled yet");
            }
            if (is_symbol("(")) {
                advance();
                auto result = make_expr(ExprKind::call, at);
                result->text = without_suffix(token.text);
                if (!accept_symbol(")")) {
                    do {
                        result->operands.push_back(expression());
                    } while (accept_symbol(","));
                    expect_symbol(")");
                }
                return result;
            }
            auto result = make_expr(ExprKind::name, at);
            result->text = without_suffix(token.text);
            return result;
        }
        fail("expected an expression");
    }

    std::vector<Token> tokens_;
    std::size_t index_{};
};

}  // namespace

ParseResult parse_module(const std::string_view source) {
    ParseResult result;
    auto lexed = tokenize(source);
    if (!lexed.error.empty()) {
        result.error = lexed.error;
        result.error_at = Position{lexed.error_line, lexed.error_column};
        return result;
    }
    try {
        Parser parser(std::move(lexed.tokens));
        result.module = parser.parse();
    } catch (const ParseFailure& failure) {
        result.error = failure.message;
        result.error_at = failure.at;
    }
    return result;
}

}  // namespace wfc::compiler
