#pragma once

// The intermediate representation of the native compiler (ADR-0008): a
// three-address code over frame slots. Every variable, parameter and temporary
// is a slot of eight bytes; a value is a 32-bit integer in the low half of its
// slot (Integer values are kept sign-extended, Boolean is -1 or 0). Back ends
// translate it without further analysis, so a back end stays small.

#include <cstdint>
#include <string>
#include <vector>

namespace wfc::compiler::ir {

enum class OperandKind {
    none,
    slot,       // value slot `index` of the current function's frame
    global,     // module-level variable `index`
    immediate,  // the constant `value`
    indirect,   // the value that slot `index` points to (a ByRef parameter)
    literal,    // the string literal `index` of the program (an immortal
                // string handle)
};

struct Operand final {
    OperandKind kind{OperandKind::none};
    std::int64_t value{};  // slot or global index, or the immediate
};

[[nodiscard]] inline Operand slot(const std::int64_t index) {
    return Operand{OperandKind::slot, index};
}
[[nodiscard]] inline Operand global(const std::int64_t index) {
    return Operand{OperandKind::global, index};
}
[[nodiscard]] inline Operand immediate(const std::int64_t value) {
    return Operand{OperandKind::immediate, value};
}
[[nodiscard]] inline Operand indirect(const std::int64_t index) {
    return Operand{OperandKind::indirect, index};
}
[[nodiscard]] inline Operand literal(const std::int64_t index) {
    return Operand{OperandKind::literal, index};
}

enum class Opcode {
    move,            // dst = a
    add,             // dst = a + b            (checked by `check`)
    sub,             // dst = a - b
    mul,             // dst = a * b
    divide,          // dst = a \ b            (error 11 when b is zero)
    modulo,          // dst = a Mod b
    negate,          // dst = -a
    bit_not,         // dst = Not a
    bit_and,         // dst = a And b
    bit_or,          // dst = a Or b
    bit_xor,         // dst = a Xor b
    compare,         // dst = -1 or 0 by `condition`
    narrow,          // dst = a, error 6 unless it fits an Integer
    address,         // dst = the address of a (a slot or a global)
    jump,            // to `target`
    branch_zero,     // to `target` when a is 0
    branch_nonzero,  // to `target` when a is not 0
    label,           // the label `target` is here
    call,            // dst = callee(args); `target` is the function index
    runtime,         // runtime service `target` with args
    print_text,      // writes the string `target` of the program
    ret,             // returns the function's result slot
};

enum class Check {
    none,
    int16,  // the result must fit an Integer
    int32,  // the result must fit a Long
};

enum class Condition {
    equal,
    not_equal,
    less,
    less_equal,
    greater,
    greater_equal
};

// Runtime services reachable from generated code. The numbering is the
// layout of the entry table of the loader (`NativeRuntimeApi`), so changing
// it changes the table version.
enum class Service : int {
    print_number = 0,   // (int32)
    print_boolean = 1,  // (int32)
    print_text = 2,     // (const char*, int32 length)
    print_newline = 3,  // ()
    raise_error = 4,    // (int32 VB error number); does not return
    end_program = 5,    // (); the End statement; does not return
    // Strings: a handle is a pointer to a reference-counted byte string; the
    // null handle is the empty string. Literals are immortal.
    print_string = 6,          // (handle)
    string_concat = 7,         // (a, b) -> new handle
    string_assign = 8,         // (handle* variable, handle value)
    string_release = 9,        // (handle)
    string_addref = 10,        // (handle)
    string_length = 11,        // (handle) -> int32
    string_compare = 12,       // (a, b) -> -1, 0 or 1
    string_left = 13,          // (s, n) -> new handle
    string_right = 14,         // (s, n)
    string_mid = 15,           // (s, start, length or -1)
    string_upper = 16,         // (s)
    string_lower = 17,         // (s)
    string_trim = 18,          // (s)
    string_ltrim = 19,         // (s)
    string_rtrim = 20,         // (s)
    string_chr = 21,           // (int32) -> new handle
    string_asc = 22,           // (s) -> int32
    string_from_number = 23,   // Str: (int32) -> " 5" or "-5"
    string_from_integer = 24,  // CStr: (int32) -> "5"
    string_from_boolean = 25,  // (int32) -> "True" or "False"
    string_instr = 26,         // (start, s, t) -> int32
    string_space = 27,         // (n) -> new handle
    count = 28,
};

struct Instruction final {
    Opcode op{Opcode::move};
    Operand dst;
    Operand a;
    Operand b;
    Check check{Check::none};
    Condition condition{Condition::equal};
    int target{};
    // `move` copies eight bytes; `call` and `runtime` store an eight-byte
    // result (a string handle) instead of four.
    bool wide{};
    std::vector<Operand> args;
};

struct Parameter final {
    bool by_value{};
    bool wide{};  // eight bytes: a pointer (ByRef) or a string handle
};

struct Function final {
    std::string name;
    std::vector<Parameter> parameters;
    int slot_count{};
    int result_slot{-1};  // -1 for a Sub
    bool result_wide{};   // the result is a string handle
    std::vector<Instruction> code;
    int label_count{};
};

struct Global final {
    std::string name;
};

struct Program final {
    std::vector<Function> functions;
    std::vector<Global> globals;
    std::vector<std::string> strings;
    int entry{-1};  // the function that starts the program
};

}  // namespace wfc::compiler::ir
