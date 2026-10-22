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
    single_precision,
    double_precision,
    string,
    variant,  // a handle to a boxed interpreter value (ADR-0009)
    object,   // a class instance (its class name is kept beside the type) or
              // a late-bound object (empty class name)
    record,   // a user-defined type: its name is kept beside the type
};

struct Position final {
    std::size_t line{};
    std::size_t column{};
};

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

enum class ExprKind {
    integer_literal,
    real_literal,
    string_literal,
    boolean_literal,
    name,         // a variable, or a function called without arguments
    call,         // `name(args)`: a function call (arrays come later)
    unary,        // op: "-", "not"
    member,       // text = the field; operands[0] = the record or object
    member_call,  // text = the member; operands[0] = the object; the rest
                  // are the arguments
    new_object,   // `New Class`: text = the class name
    with_target,  // the object of the enclosing With: `.member` starts here
    binary,       // op: "+", "-", "*", "\\", "mod", "=", "<>", "<", ">", "<=",
                  // ">=", "and", "or", "xor", "&"
};

struct Expr final {
    ExprKind kind{ExprKind::integer_literal};
    Position at;
    std::int64_t number{};
    double real{};
    bool long_literal{};
    bool single_literal{};
    bool parenthesized{};  // `(x)`: a value, never a variable passed ByRef
    std::string text;      // a literal's contents, a name, or an operator
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
    call,         // a Sub or Function called as a statement
    exit,         // text: "sub", "function", "for", "do"
    select_case,  // expr = subject; cases; body = Case Else when has_else
    const_decl,   // constants
    label,        // text = the label
    go_to,        // text = the label
    redim,        // declarations (with bounds); preserve
    on_error,     // text: a label, "0" or "resume next"
    resume,       // text: "", "next" or a label
    erase,        // items: the names (as name expressions)
    set_object,   // `Set target = expr`
    call_member,  // an object's member called as a statement: expr is a
                  // member or member_call
    with_block,   // `With expr`: expr; body
};

// One test of a Case: a value, a range (`low To high`) or `Is <op> low`.
struct CaseClause final {
    enum class Kind { value, range, relation };
    Kind kind{Kind::value};
    ExprPtr low;
    ExprPtr high;
    std::string op;
};

struct CaseBlock final {
    std::vector<CaseClause> clauses;
    std::vector<StmtPtr> body;
};

struct ConstantDecl final {
    std::string name;  // lower-case
    VbType type{VbType::none};
    bool type_given{};
    ExprPtr value;
    Position at;
};

// One dimension of an array: `low To high`, or just `high` (low is null).
struct ArrayBound final {
    ExprPtr low;
    ExprPtr high;
};

struct Declaration final {
    std::string name;           // lower-case
    VbType type{VbType::none};  // none: no type given (a Variant)
    bool type_given{};
    std::string type_name;  // the user-defined type, when type is record
    bool is_array{};
    bool is_public{};                // `Public` (a field of a class)
    bool is_new{};                   // `As New Class`
    bool is_static{};                // `Static` inside a procedure
    std::vector<ArrayBound> bounds;  // empty for a dynamic array: `Dim a()`
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
    // A label that is not in the first column may be a call (`Bump : Bump`).
    bool maybe_call{};
    // Print: the separator after each item: ' ' (none), ';' or ','.
    std::string separators;
    bool until{};                              // Do ... Until / Loop Until
    bool test_after{};                         // Do ... Loop While/Until
    std::vector<ExprPtr> conditions;           // If / ElseIf
    std::vector<std::vector<StmtPtr>> blocks;  // bodies (the last may be Else)
    bool has_else{};
    std::vector<StmtPtr> body;  // loop body, or Case Else
    ExprPtr target;             // an assignment to a field: `a.b.c = expr`
    std::vector<CaseBlock> cases;
    bool preserve{};  // ReDim Preserve
    std::vector<ConstantDecl> constants;
};

struct Parameter final {
    std::string name;
    std::string type_name;      // the user-defined type, when type is record
    bool is_array{};            // `name() As Type`: always passed by reference
    VbType type{VbType::none};  // none: no type given (a Variant)
    bool by_value{};
};

// Property Get, Let or Set; none for a Sub or a Function.
enum class PropertyKind { none, get, let, set };

struct Procedure final {
    std::string name;  // lower-case
    std::string spelling;
    Position at;
    PropertyKind property{PropertyKind::none};
    bool is_function{};
    bool is_public{true};
    VbType return_type{VbType::none};
    std::string return_type_name;  // the class, when the type is an object
    std::vector<Parameter> parameters;
    std::vector<StmtPtr> body;
};

struct FieldDecl final {
    std::string name;  // lower-case
    VbType type{VbType::none};
    std::string type_name;
    bool is_array{};                 // a fixed array inside the type
    std::vector<ArrayBound> bounds;  // its one dimension
    Position at;
};

struct TypeDecl final {
    std::string name;  // lower-case
    std::vector<FieldDecl> fields;
    Position at;
};

struct Module final {
    std::string name;
    bool is_class{};  // a class module: `globals` are its fields
    std::vector<TypeDecl> types;
    std::vector<ConstantDecl> constants;
    std::vector<Declaration> globals;
    std::vector<Procedure> procedures;
    int option_base{0};  // the lower bound of an array that gives none
};

struct ParseResult final {
    Module module;
    std::string error;
    Position error_at;
};

}  // namespace wfc::compiler
