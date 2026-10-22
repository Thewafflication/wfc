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
    Parser(std::vector<Token> tokens, const ParseOptions& options)
        : tokens_(std::move(tokens)), options_(options) {}

    Module parse() {
        Module module;
        module.is_class = options_.is_class;
        module.name = options_.name;
        skip_ends();
        while (!at_end_of_file()) {
            if (accept_word("option")) {
                option_statement(module);
            } else if (is_word("dim") || is_word("private") ||
                       is_word("public") || is_word("friend") ||
                       is_word("static") || is_word("sub") ||
                       is_word("function") || is_word("property") ||
                       is_word("const") || is_word("type") ||
                       is_word("event") || is_word("implements") ||
                       is_word("enum") || is_word("declare")) {
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

    void option_statement(Module& module) {
        if (accept_word("explicit")) {
            expect_end_of_statement();
            return;
        }
        if (accept_word("base")) {
            if (current().kind != TokenKind::integer ||
                (current().number != 0 && current().number != 1)) {
                fail("Option Base must be 0 or 1");
            }
            module.option_base = static_cast<int>(current().number);
            advance();
            expect_end_of_statement();
            return;
        }
        if (accept_word("compare")) {
            if (accept_word("binary")) {
                expect_end_of_statement();
                return;
            }
            fail("Option Compare " + current().spelling +
                 " is not compiled yet");
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

    [[nodiscard]] VbType type_name(std::string* record_name = nullptr) {
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
        } else if (name == "variant") {
            type = VbType::variant;
        } else if (name == "object") {
            type = VbType::object;
            if (record_name != nullptr) {
                record_name->clear();
            }
        } else if (options_.class_names.contains(name)) {
            type = VbType::object;
            if (record_name != nullptr) {
                *record_name = name;
            }
        } else if (name == "string") {
            type = VbType::string;
            if (ahead(1).kind == TokenKind::symbol && ahead(1).text == "*") {
                fail("fixed-length strings are not compiled yet");
            }
        } else if (record_name != nullptr) {
            // Any other name is a user-defined type; the compiler checks it.
            type = VbType::record;
            *record_name = name;
        } else {
            fail("the type " + current().spelling + " is not compiled yet");
        }
        advance();
        return type;
    }

    // `[low To] high {, [low To] high} )`; the opening parenthesis is read.
    void array_bounds(std::vector<ArrayBound>& into) {
        do {
            ArrayBound bound;
            auto first = expression();
            if (accept_word("to")) {
                bound.low = std::move(first);
                bound.high = expression();
            } else {
                bound.high = std::move(first);
            }
            into.push_back(std::move(bound));
        } while (accept_symbol(","));
        expect_symbol(")");
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
            advance();
            declaration.is_array = true;
            if (!accept_symbol(")")) {
                array_bounds(declaration.bounds);
            }
        }
        if (accept_word("as")) {
            declaration.is_new = accept_word("new");
            declaration.type = type_name(&declaration.type_name);
            if (declaration.is_new && (declaration.type != VbType::object ||
                                       declaration.type_name.empty())) {
                fail("New needs the name of a class");
            }
            declaration.type_given = true;
        } else if (by_suffix != VbType::none) {
            declaration.type = by_suffix;
            declaration.type_given = true;
        } else {
            declaration.type = VbType::variant;
            declaration.type_given = true;
        }
        return declaration;
    }

    void declaration_or_procedure(Module& module) {
        bool is_public = true;
        bool saw_scope = false;
        bool saw_public = false;
        while (is_word("private") || is_word("public") || is_word("friend") ||
               is_word("static")) {
            if (is_word("private")) {
                is_public = false;
            }
            if (is_word("public")) {
                saw_public = true;
            }
            if (is_word("static")) {
                fail("Static is not compiled yet");
            }
            saw_scope = true;
            advance();
        }
        if (is_word("sub") || is_word("function") || is_word("property")) {
            procedure(module, is_public);
            return;
        }
        if (is_word("event") || is_word("implements") || is_word("enum") ||
            is_word("declare") || is_word("withevents")) {
            fail(current().spelling + " is not compiled yet");
        }
        if (is_word("type")) {
            type_declaration(module);
            return;
        }
        if (accept_word("const")) {
            constants(module.constants);
            expect_end_of_statement();
            return;
        }
        if (accept_word("dim") || saw_scope) {
            do {
                auto declaration = declarator();
                declaration.is_public = saw_public;
                module.globals.push_back(std::move(declaration));
            } while (accept_symbol(","));
            expect_end_of_statement();
            return;
        }
        fail("expected a declaration, Sub or Function");
    }

    // Type Name / fields / End Type
    void type_declaration(Module& module) {
        TypeDecl type;
        type.at = here();
        advance();
        if (current().kind != TokenKind::identifier) {
            fail("expected the name of the type");
        }
        type.name = current().text;
        advance();
        expect_end_of_statement();
        while (!(is_word("end") && ahead(1).kind == TokenKind::identifier &&
                 ahead(1).text == "type")) {
            if (at_end_of_file()) {
                fail("expected End Type");
            }
            FieldDecl field;
            field.at = here();
            if (current().kind != TokenKind::identifier) {
                fail("expected a field name");
            }
            const std::string spelled = current().text;
            field.name = without_suffix(spelled);
            const VbType by_suffix = suffix_type(spelled);
            advance();
            if (is_symbol("(")) {
                fail("arrays in types are not compiled yet");
            }
            if (accept_word("as")) {
                field.type = type_name(&field.type_name);
            } else if (by_suffix != VbType::none) {
                field.type = by_suffix;
            } else {
                fail("the field '" + field.name +
                     "' needs a type (Variant is not compiled yet)");
            }
            expect_end_of_statement();
            type.fields.push_back(std::move(field));
        }
        advance();
        advance();
        expect_end_of_statement();
        module.types.push_back(std::move(type));
    }

    // `name [As Type] = expression {, name [As Type] = expression}`
    void constants(std::vector<ConstantDecl>& into) {
        do {
            if (current().kind != TokenKind::identifier) {
                fail("expected the constant name");
            }
            ConstantDecl constant;
            constant.at = here();
            const std::string spelled = current().text;
            constant.name = without_suffix(spelled);
            const VbType by_suffix = suffix_type(spelled);
            advance();
            if (by_suffix != VbType::none) {
                constant.type = by_suffix;
                constant.type_given = true;
            }
            if (accept_word("as")) {
                constant.type = type_name();
                constant.type_given = true;
            }
            expect_symbol("=");
            constant.value = expression();
            into.push_back(std::move(constant));
        } while (accept_symbol(","));
    }

    void procedure(Module& module, const bool is_public) {
        Procedure result;
        result.is_public = is_public;
        result.is_function = is_word("function");
        const bool is_property = is_word("property");
        result.at = here();
        advance();
        if (is_property) {
            if (accept_word("get")) {
                result.property = PropertyKind::get;
                result.is_function = true;
            } else if (accept_word("let")) {
                result.property = PropertyKind::let;
            } else if (accept_word("set")) {
                result.property = PropertyKind::set;
            } else {
                fail("expected Get, Let or Set after Property");
            }
        }
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
            // Without `As` a Function returns a Variant.
            result.return_type = by_suffix;
            if (accept_word("as")) {
                result.return_type = type_name(&result.return_type_name);
            }
            if (result.return_type == VbType::none) {
                result.return_type = VbType::variant;
            }
        }
        expect_end_of_statement();
        result.body = block({"end"});
        expect_word("end");
        const char* closing = is_property          ? "property"
                              : result.is_function ? "function"
                                                   : "sub";
        if (!accept_word(closing)) {
            fail(std::string("expected End ") + (is_property ? "Property"
                                                 : result.is_function
                                                     ? "Function"
                                                     : "Sub"));
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
            advance();
            expect_symbol(")");
            result.is_array = true;
            result.by_value = false;
        }
        if (accept_word("as")) {
            result.type = type_name(&result.type_name);
        }
        if (result.type == VbType::none) {
            result.type = VbType::variant;
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
                        next.text == "with" || next.text == "property");
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
        if (current().line_start &&
            ahead(1).kind == TokenKind::end_of_statement &&
            ahead(1).text == ":" && word != "else") {
            auto result = make(StmtKind::label);
            result->text = without_suffix(word);
            result->maybe_call = result->at.column != 1;
            advance();
            advance();
            return result;
        }
        if (word == "dim" || word == "static") {
            return dim_statement();
        }
        if (word == "const") {
            auto result = make(StmtKind::const_decl);
            advance();
            constants(result->constants);
            return result;
        }
        if (word == "goto") {
            auto result = make(StmtKind::go_to);
            advance();
            if (current().kind != TokenKind::identifier) {
                fail("expected a label after GoTo");
            }
            result->text = without_suffix(current().text);
            advance();
            return result;
        }
        if (word == "select") {
            return select_statement();
        }
        if (word == "on" && ahead(1).kind == TokenKind::identifier &&
            ahead(1).text == "error") {
            return on_error_statement();
        }
        if (word == "resume") {
            auto result = make(StmtKind::resume);
            advance();
            if (accept_word("next")) {
                result->text = "next";
            } else if (current().kind == TokenKind::identifier &&
                       !is_word("else")) {
                result->text = without_suffix(current().text);
                advance();
            }
            return result;
        }
        if (word == "redim") {
            return redim_statement();
        }
        if (word == "erase") {
            auto result = make(StmtKind::erase);
            advance();
            do {
                auto name = make_expr(ExprKind::name, here());
                if (current().kind != TokenKind::identifier) {
                    fail("expected an array name");
                }
                name->text = without_suffix(current().text);
                advance();
                result->items.push_back(std::move(name));
            } while (accept_symbol(","));
            return result;
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
        if (word == "set") {
            auto result = make(StmtKind::set_object);
            advance();
            result->target = primary();
            expect_symbol("=");
            result->expr = expression();
            return result;
        }
        if (word == "with" || word == "gosub" || word == "on" ||
            word == "open" || word == "close" || word == "static" ||
            word == "randomize" || word == "line" || word == "raiseevent") {
            fail("the statement " + current().spelling +
                 " is not compiled yet");
        }
        return assignment_or_call();
    }

    // On Error GoTo label | On Error GoTo 0 | On Error Resume Next
    [[nodiscard]] StmtPtr on_error_statement() {
        auto result = make(StmtKind::on_error);
        advance();
        advance();
        if (accept_word("resume")) {
            expect_word("next");
            result->text = "resume next";
            return result;
        }
        expect_word("goto");
        if (current().kind == TokenKind::integer) {
            if (current().number != 0) {
                fail("On Error GoTo " + std::to_string(current().number) +
                     " is not compiled yet");
            }
            result->text = "0";
            advance();
            return result;
        }
        if (current().kind != TokenKind::identifier) {
            fail("expected a label after On Error GoTo");
        }
        result->text = without_suffix(current().text);
        advance();
        return result;
    }

    [[nodiscard]] StmtPtr redim_statement() {
        auto result = make(StmtKind::redim);
        advance();
        if (accept_word("preserve")) {
            result->preserve = true;
        }
        do {
            if (current().kind != TokenKind::identifier) {
                fail("expected an array name");
            }
            Declaration declaration;
            declaration.at = here();
            declaration.name = without_suffix(current().text);
            declaration.is_array = true;
            advance();
            expect_symbol("(");
            array_bounds(declaration.bounds);
            if (accept_word("as")) {
                declaration.type = type_name(&declaration.type_name);
                declaration.type_given = true;
            }
            result->declarations.push_back(std::move(declaration));
        } while (accept_symbol(","));
        return result;
    }

    [[nodiscard]] StmtPtr select_statement() {
        auto result = make(StmtKind::select_case);
        advance();
        expect_word("case");
        result->expr = expression();
        expect_end_of_statement();
        while (is_word("case")) {
            advance();
            if (accept_word("else")) {
                expect_end_of_statement();
                result->body = block({"case", "end"});
                result->has_else = true;
                break;
            }
            CaseBlock block_of_case;
            do {
                CaseClause clause;
                if (accept_word("is")) {
                    if (current().kind != TokenKind::symbol ||
                        !(current().text == "=" || current().text == "<>" ||
                          current().text == "<" || current().text == ">" ||
                          current().text == "<=" || current().text == ">=")) {
                        fail("expected a comparison operator after Is");
                    }
                    clause.kind = CaseClause::Kind::relation;
                    clause.op = current().text;
                    advance();
                    clause.low = expression();
                } else {
                    clause.low = expression();
                    if (accept_word("to")) {
                        clause.kind = CaseClause::Kind::range;
                        clause.high = expression();
                    }
                }
                block_of_case.clauses.push_back(std::move(clause));
            } while (accept_symbol(","));
            expect_end_of_statement();
            block_of_case.body = block({"case", "end"});
            result->cases.push_back(std::move(block_of_case));
        }
        expect_word("end");
        expect_word("select");
        return result;
    }

    [[nodiscard]] StmtPtr dim_statement() {
        auto result = make(StmtKind::dim);
        const bool is_static = is_word("static");
        advance();
        do {
            result->declarations.push_back(declarator());
            result->declarations.back().is_static = is_static;
        } while (accept_symbol(","));
        return result;
    }

    [[nodiscard]] StmtPtr print_statement() {
        auto result = make(StmtKind::print);
        advance();
        while (!at_end_of_statement() && !is_word("else")) {
            // A separator with no item before it (`Print , x`) is an empty
            // item.
            result->items.push_back(
                is_symbol(",") || is_symbol(";") ? nullptr : expression());
            char separator = ' ';
            if (accept_symbol(";")) {
                separator = ';';
            } else if (accept_symbol(",")) {
                separator = ',';
            }
            result->separators.push_back(separator);
            if (separator == ' ') {
                break;
            }
            result->newline =
                at_end_of_statement() || is_word("else") ? false : true;
        }
        return result;
    }

    // Whether `name ( ... ) =` starts here: an assignment to an element.
    [[nodiscard]] bool is_element_assignment() const {
        if (!(ahead(1).kind == TokenKind::symbol && ahead(1).text == "(")) {
            return false;
        }
        int depth = 0;
        for (std::size_t i = 1;; ++i) {
            const Token& token = ahead(i);
            if (token.kind == TokenKind::end_of_statement ||
                token.kind == TokenKind::end_of_file) {
                return false;
            }
            if (token.kind == TokenKind::symbol && token.text == "(") {
                ++depth;
            } else if (token.kind == TokenKind::symbol && token.text == ")") {
                if (--depth == 0) {
                    const Token& next = ahead(i + 1);
                    return next.kind == TokenKind::symbol && next.text == "=";
                }
            }
        }
    }

    // Whether `name [(...)] . field ... =` starts here: an assignment to a
    // field of a record.
    [[nodiscard]] bool is_field_assignment() const {
        std::size_t i = 1;
        bool saw_field = false;
        while (true) {
            const Token& token = ahead(i);
            if (token.kind == TokenKind::symbol && token.text == "(") {
                int depth = 0;
                while (true) {
                    const Token& inner = ahead(i);
                    if (inner.kind == TokenKind::end_of_statement ||
                        inner.kind == TokenKind::end_of_file) {
                        return false;
                    }
                    if (inner.kind == TokenKind::symbol && inner.text == "(") {
                        ++depth;
                    } else if (inner.kind == TokenKind::symbol &&
                               inner.text == ")") {
                        if (--depth == 0) {
                            break;
                        }
                    }
                    ++i;
                }
                ++i;
            } else if (token.kind == TokenKind::symbol && token.text == "." &&
                       ahead(i + 1).kind == TokenKind::identifier) {
                saw_field = true;
                i += 2;
            } else {
                return saw_field && token.kind == TokenKind::symbol &&
                       token.text == "=";
            }
        }
    }

    [[nodiscard]] StmtPtr assignment_or_call() {
        if (current().kind != TokenKind::identifier) {
            fail("expected a statement");
        }
        if (is_field_assignment()) {
            auto result = make(StmtKind::assign);
            result->target = primary();
            expect_symbol("=");
            result->expr = expression();
            return result;
        }
        if (is_element_assignment()) {
            auto result = make(StmtKind::assign);
            result->text = without_suffix(current().text);
            advance();
            expect_symbol("(");
            do {
                result->items.push_back(expression());
            } while (accept_symbol(","));
            expect_symbol(")");
            expect_symbol("=");
            result->expr = expression();
            return result;
        }
        if (ahead(1).kind == TokenKind::symbol && ahead(1).text == "=") {
            auto result = make(StmtKind::assign);
            result->text = without_suffix(current().text);
            advance();
            advance();
            result->expr = expression();
            return result;
        }
        if (starts_member_chain()) {
            return member_statement(false);
        }
        return call_statement(false);
    }

    // Whether `name[(...)] . member ...` starts here (not the Err object).
    [[nodiscard]] bool starts_member_chain() const {
        if (current().kind != TokenKind::identifier ||
            current().text == "err") {
            return false;
        }
        std::size_t i = 1;
        if (ahead(i).kind == TokenKind::symbol && ahead(i).text == "(") {
            int depth = 0;
            while (true) {
                const Token& token = ahead(i);
                if (token.kind == TokenKind::end_of_statement ||
                    token.kind == TokenKind::end_of_file) {
                    return false;
                }
                if (token.kind == TokenKind::symbol && token.text == "(") {
                    ++depth;
                } else if (token.kind == TokenKind::symbol &&
                           token.text == ")") {
                    if (--depth == 0) {
                        break;
                    }
                }
                ++i;
            }
            ++i;
        }
        return ahead(i).kind == TokenKind::symbol && ahead(i).text == ".";
    }

    // `obj.Member args` or `Call obj.Member(args)`: the member is called.
    [[nodiscard]] StmtPtr member_statement(const bool with_call_keyword) {
        auto result = make(StmtKind::call_member);
        auto chain = primary();
        if (chain->kind == ExprKind::member && !with_call_keyword) {
            // Arguments without parentheses: `obj.Member a, b`.
            auto call = make_expr(ExprKind::member_call, chain->at);
            call->text = chain->text;
            call->operands.push_back(std::move(chain->operands[0]));
            while (!at_end_of_statement() && !is_word("else")) {
                call->operands.push_back(expression());
                if (!accept_symbol(",")) {
                    break;
                }
            }
            chain = std::move(call);
        }
        result->expr = std::move(chain);
        return result;
    }

    [[nodiscard]] StmtPtr call_statement(const bool with_call_keyword) {
        if (with_call_keyword && starts_member_chain()) {
            return member_statement(true);
        }
        auto result = make(StmtKind::call);
        if (current().kind != TokenKind::identifier) {
            fail("expected the procedure name");
        }
        result->text = without_suffix(current().text);
        const bool is_err = current().text == "err";
        advance();
        if (is_symbol(".")) {
            if (!is_err) {
                fail("members of objects are not compiled yet");
            }
            advance();
            if (current().kind != TokenKind::identifier) {
                fail("expected a member name after Err.");
            }
            result->text = "err." + current().text;
            advance();
        }
        if (accept_symbol("(")) {
            if (!accept_symbol(")")) {
                do {
                    // An omitted argument (`Err.Raise 5, , "text"`) is null.
                    result->items.push_back(is_symbol(",") ? nullptr
                                                           : expression());
                } while (accept_symbol(","));
                expect_symbol(")");
            }
            // `Name (x)` without Call passes the parenthesized value.
            if (!with_call_keyword && result->items.size() == 1U) {
                result->items.front()->parenthesized = true;
            }
        } else if (!with_call_keyword) {
            // `name arg, arg`: arguments without parentheses.
            while (!at_end_of_statement() && !is_word("else")) {
                result->items.push_back(is_symbol(",") ? nullptr
                                                       : expression());
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
        if (is_word("is")) {
            const auto at = here();
            advance();
            left = binary("is", std::move(left), concatenation(), at);
        }
        if (is_word("like")) {
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

    // `^` is left to right and binds tighter than a unary minus; the exponent
    // may carry a sign of its own.
    [[nodiscard]] ExprPtr power() {
        auto left = primary();
        while (is_symbol("^")) {
            const auto at = here();
            advance();
            bool negate = false;
            if (is_symbol("-") || is_symbol("+")) {
                negate = is_symbol("-");
                advance();
            }
            auto exponent = primary();
            if (negate) {
                auto minus = make_expr(ExprKind::unary, at);
                minus->text = "-";
                minus->operands.push_back(std::move(exponent));
                exponent = std::move(minus);
            }
            left = binary("^", std::move(left), std::move(exponent), at);
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
            inner->parenthesized = true;
            return inner;
        }
        if (token.kind == TokenKind::identifier && token.text == "new") {
            advance();
            if (current().kind != TokenKind::identifier ||
                !options_.class_names.contains(current().text)) {
                fail("New needs the name of a class");
            }
            auto result = make_expr(ExprKind::new_object, at);
            result->text = current().text;
            advance();
            return result;
        }
        if (token.kind == TokenKind::identifier) {
            if (token.text == "true" || token.text == "false") {
                advance();
                auto result = make_expr(ExprKind::boolean_literal, at);
                result->number = token.text == "true" ? -1 : 0;
                return result;
            }
            advance();
            if (is_symbol(".") && token.text == "err") {
                // The Err object.
                advance();
                if (current().kind != TokenKind::identifier) {
                    fail("expected a member name after Err.");
                }
                auto member = make_expr(ExprKind::name, at);
                member->text = "err." + current().text;
                advance();
                return member;
            }
            ExprPtr node;
            if (is_symbol("(")) {
                advance();
                node = make_expr(ExprKind::call, at);
                node->text = without_suffix(token.text);
                if (!accept_symbol(")")) {
                    do {
                        node->operands.push_back(expression());
                    } while (accept_symbol(","));
                    expect_symbol(")");
                }
            } else {
                node = make_expr(ExprKind::name, at);
                node->text = without_suffix(token.text);
            }
            // `.member` chains: a.b.c, array(i).field, obj.Method(args)
            while (is_symbol(".")) {
                advance();
                if (current().kind != TokenKind::identifier) {
                    fail("expected a member name after '.'");
                }
                auto field = make_expr(ExprKind::member, at);
                field->text = without_suffix(current().text);
                advance();
                field->operands.push_back(std::move(node));
                if (is_symbol("(")) {
                    advance();
                    field->kind = ExprKind::member_call;
                    if (!accept_symbol(")")) {
                        do {
                            field->operands.push_back(expression());
                        } while (accept_symbol(","));
                        expect_symbol(")");
                    }
                }
                node = std::move(field);
            }
            return node;
        }
        fail("expected an expression");
    }

    std::vector<Token> tokens_;
    const ParseOptions& options_;
    std::size_t index_{};
};

}  // namespace

ParseResult parse_module(const std::string_view source,
                         const ParseOptions& options) {
    ParseResult result;
    auto lexed = tokenize(source);
    if (!lexed.error.empty()) {
        result.error = lexed.error;
        result.error_at = Position{lexed.error_line, lexed.error_column};
        return result;
    }
    try {
        Parser parser(std::move(lexed.tokens), options);
        result.module = parser.parse();
    } catch (const ParseFailure& failure) {
        result.error = failure.message;
        result.error_at = failure.at;
    }
    return result;
}

}  // namespace wfc::compiler
