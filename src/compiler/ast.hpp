#pragma once

// The abstract syntax tree of the native compiler front end (ADR-0008). It
// covers the language subset the compiler supports so far; the parser reports
// every construct outside it with a diagnostic that names it.

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace wfc::compiler {

// The VB types the compiler knows.
enum class VbType {
    none,  // a Sub has no value
    boolean,
    integer,
    long_integer,
    string,
};

struct Position final {
    std::size_t line{};
    std::size_t column{};
};

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

enum class ExprKind {
    integer_literal,
    string_literal,
    boolean_literal,
    name,    // a variable, or a function called without arguments
    call,    // `name(args)`: a function call (arrays come later)
    unary,   // op: "-", "not"
    binary,  // op: "+", "-", "*", "\\", "mod", "=", "<>", "<", ">", "<=",
             // ">=", "and", "or", "xor", "&"
};

struct Expr final {
    ExprKind kind{ExprKind::integer_literal};
    Position at;
    std::int64_t number{};
    bool long_literal{};
    std::string text;  // a literal's contents, a name, or an operator
    std::vector<ExprPtr>
        operands;  // call arguments, or the operator's operands
};

struct Stmt;
using StmtPtr = std::unique_ptr<Stmt>;

enum class StmtKind {
    dim,       // names/types in `declarations`
    assign,    // `text = expr`
    print,     // `Print` of `items`; `newline` tells whether it ends the line
    if_block,  // conditions[i] -> blocks[i]; an extra last block is Else
    for_loop,  // text = variable; expr, limit, step; body
    do_loop,   // condition tested before or after; `until` flips it
    while_loop,
    call,  // a Sub or Function called as a statement
    exit,  // text: "sub", "function", "for", "do"
};

struct Declaration final {
    std::string name;           // lower-case
    VbType type{VbType::none};  // none: no type given (a Variant)
    bool type_given{};
    Position at;
};

struct Stmt final {
    StmtKind kind{StmtKind::assign};
    Position at;
    std::string text;
    std::vector<Declaration> declarations;
    ExprPtr expr;   // assigned value, loop start, condition, or call
    ExprPtr limit;  // For limit
    ExprPtr step;   // For step
    std::vector<ExprPtr> items;  // Print items, or call arguments
    bool newline{true};
    bool until{};                              // Do ... Until / Loop Until
    bool test_after{};                         // Do ... Loop While/Until
    std::vector<ExprPtr> conditions;           // If / ElseIf
    std::vector<std::vector<StmtPtr>> blocks;  // bodies (the last may be Else)
    bool has_else{};
    std::vector<StmtPtr> body;  // loop body
};

struct Parameter final {
    std::string name;
    VbType type{VbType::none};  // none: no type given (a Variant)
    bool by_value{};
};

struct Procedure final {
    std::string name;  // lower-case
    std::string spelling;
    Position at;
    bool is_function{};
    bool is_public{true};
    VbType return_type{VbType::none};
    std::vector<Parameter> parameters;
    std::vector<StmtPtr> body;
};

struct Module final {
    std::string name;
    std::vector<Declaration> globals;
    std::vector<Procedure> procedures;
};

struct ParseResult final {
    Module module;
    std::string error;
    Position error_at;
};

}  // namespace wfc::compiler
