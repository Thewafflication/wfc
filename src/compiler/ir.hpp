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
    move,     // dst = a
    add,      // dst = a + b            (checked by `check`)
    sub,      // dst = a - b
    mul,      // dst = a * b
    divide,   // dst = a \ b            (error 11 when b is zero)
    modulo,   // dst = a Mod b
    negate,   // dst = -a
    bit_not,  // dst = Not a
    bit_and,  // dst = a And b
    bit_or,   // dst = a Or b
    bit_xor,  // dst = a Xor b
    compare,  // dst = -1 or 0 by `condition`
    // Floating point: a value is the eight bytes of an IEEE double (a Single
    // is a double rounded to a float's precision); a literal is an immediate
    // holding the bits.
    fadd,            // dst = a + b            (error 6 on overflow)
    fsub,            // dst = a - b
    fmul,            // dst = a * b
    fdivide,         // dst = a / b            (error 11 when b is zero)
    fnegate,         // dst = -a
    fcompare,        // dst = -1 or 0 by `condition`
    int_to_double,   // dst = double(a), a 32-bit integer
    round_single,    // dst = a rounded to a Single
    narrow,          // dst = a, error 6 unless it fits an Integer
    address,         // dst = the address of a (a slot or a global)
    pointer_add,     // dst = a (a slot holding a pointer) + the immediate b
    check_error,     // to `target` when a run-time error is pending
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
    print_double = 28,         // (double, int digits)
    string_from_double = 29,   // (double, int digits, int sign space) -> handle
    double_to_long = 30,       // (double) -> int32, rounded; error 6 if too big
    math_function = 31,        // (int which, double) -> double: MathFunction
    double_sign = 32,          // (double) -> -1, 0 or 1
    // Arrays: a handle is a pointer to a reference-counted array object; the
    // null handle is an array that has no elements yet (error 9 on access).
    array_redim = 33,     // (handle* var, kind|fixed, dims, int64* bounds,
                          //  preserve, element size, string offsets);
                          //  bounds are lower, upper per dimension; kind 3 is
                          //  a record whose size and String offsets (a count
                          //  then the offsets, as int32) follow
    array_element1 = 34,  // (handle, i) -> address of the element
    array_element2 = 35,  // (handle, i, j)
    array_element3 = 36,  // (handle, i, j, k)
    array_bound = 37,     // (handle, dimension from 1, 0 lower or 1 upper)
    array_release = 38,   // (handle)
    array_erase = 39,     // (handle* var)
    // The error state. A raised error sets the pending flag (generated code
    // tests it after every operation that can fail); a handler clears it.
    error_number = 40,       // () -> int32, Err.Number
    error_description = 41,  // () -> new handle, Err.Description
    error_source = 42,       // () -> new handle, Err.Source
    error_clear = 43,        // (), Err.Clear
    error_raise = 44,        // (number, source handle, description handle)
    error_acknowledge = 45,  // (), a handler took over: the flag is cleared
    print_zone = 46,         // (), a comma in Print: to the next 14-column zone
    double_power = 47,       // (double, double) -> double: a ^ b
    // Variants (ADR-0009): a handle to a reference-counted box that holds an
    // interpreter Value; the null handle is Empty. A service that fails sets
    // the pending error and returns the null handle or 0.
    variant_from_long = 48,     // (int32) -> new handle
    variant_from_integer = 49,  // (int32) -> new handle, an Integer (Int16)
    variant_from_double = 50,   // (double) -> new handle
    variant_from_single = 51,   // (double) -> new handle, a Single
    variant_from_string = 52,   // (string handle) -> new handle
    variant_from_boolean = 53,  // (int32) -> new handle
    variant_null = 54,          // () -> new handle holding Null
    variant_assign = 55,        // (handle* variable, handle value)
    variant_release = 56,       // (handle)
    variant_addref = 57,        // (handle)
    variant_binary = 58,        // (VariantOp, a, b) -> new handle
    variant_negate = 59,        // (a) -> new handle
    variant_not = 60,           // (a) -> new handle
    variant_to_long = 61,       // (handle) -> int32
    variant_to_double = 62,     // (handle) -> double
    variant_to_string = 63,     // (handle) -> new string handle
    variant_to_boolean = 64,    // (handle) -> -1 or 0
    variant_print = 65,         // (handle)
    variant_type_name = 66,     // (handle) -> new string handle
    variant_var_type = 67,      // (handle) -> int32
    variant_is_numeric = 68,    // (handle) -> -1 or 0
    // Objects of compiled classes (ADR-0009). A handle points to a block with
    // a 16-byte header (reference count, class record) and the fields behind
    // it; the null handle is Nothing. A class record is a literal of the
    // program (`Class::record`), see `HandleKind`.
    object_new = 69,      // (class record) -> new handle, fields zeroed
    object_addref = 70,   // (handle)
    object_release = 71,  // (handle): Terminate runs when the count reaches 0
    object_assign = 72,   // (handle* variable, handle value)
    object_check = 73,    // (handle): error 91 when it is Nothing
    object_is = 74,       // (a, b) -> -1 when they are the same object
    object_is_null = 75,  // (handle) -> -1 for Nothing
    // The address of element `index` of a fixed array inside a record:
    // (base, index, lower bound, count, element size); error 9 outside.
    inline_element = 76,  // -> pointer
    // A function of the VBA library with Variant arguments: (name literal,
    // count, block of argument handles read upward in memory) -> new handle.
    variant_call = 77,
    variant_missing = 78,     // () -> new handle: an omitted Optional Variant
    variant_is_missing = 79,  // (handle) -> -1 for the omitted argument
    array_count = 80,         // (handle) -> number of elements
    array_flat_element =
        81,          // (handle, index from 0) -> address of the element
    randomize = 82,  // (has seed, seed double, is Timer)
    // Objects in Variants and Variant arrays.
    variant_from_object = 83,  // (object) -> new handle (Nothing allowed)
    variant_to_object = 84,    // (handle) -> new object reference
    variant_array_count = 85,  // (handle) -> number of elements, 0 for none
    variant_array_item = 86,   // (handle, index from 0) -> new handle
    count = 87,
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
    // Bit i set: argument i is a double (passed in an SSE register); the
    // result of a call or service is a double.
    std::uint32_t double_args{};
    bool double_result{};
    std::vector<Operand> args;
};

// A field or element that holds a counted handle, as the class records and
// the descriptors of record arrays name it.
enum class HandleKind : int { string = 0, variant = 1, object = 2, array = 3 };

// The offset of the fields behind the header of an object.
inline constexpr int kObjectHeader = 24;

// A class: its record is a literal of the program holding int32 values: the
// size of the fields, the image offset of Class_Terminate (-1 for none;
// filled in by the back end), a count, the length of the class name,
// (offset, HandleKind) pairs and the name.
struct Class final {
    std::string name;
    std::string display_name;
    int record{-1};     // the literal that holds the record
    int terminate{-1};  // the function of Class_Terminate
};

// The operator of the `variant_binary` service.
enum class VariantOp : int {
    add = 0,
    subtract = 1,
    multiply = 2,
    divide = 3,
    integer_divide = 4,
    modulo = 5,
    concatenate = 6,
    power = 7,
    equal = 8,
    not_equal = 9,
    less = 10,
    greater = 11,
    less_equal = 12,
    greater_equal = 13,
    logical_and = 14,
    logical_or = 15,
    logical_xor = 16,
    equivalence = 17,
    implication = 18,
};

// The functions of the `math_function` service.
enum class MathFunction : int {
    square_root = 0,
    sine = 1,
    cosine = 2,
    tangent = 3,
    arctangent = 4,
    logarithm = 5,
    exponential = 6,
    floor = 7,
    truncate = 8,
    absolute = 9,
};

struct Parameter final {
    bool by_value{};
    bool wide{};       // eight bytes: a pointer (ByRef) or a string handle
    bool is_double{};  // a ByVal Single or Double
};

struct Function final {
    std::string name;
    std::vector<Parameter> parameters;
    int slot_count{};
    int result_slot{-1};   // -1 for a Sub
    int error_label{-1};   // where a pending error goes (the end or a handler)
    bool result_wide{};    // the result is a string handle
    bool result_double{};  // the result is a Single or a Double
    std::vector<Instruction> code;
    int label_count{};
};

struct Global final {
    std::string name;
};

struct Program final {
    std::vector<Class> classes;
    std::vector<Function> functions;
    std::vector<Global> globals;
    std::vector<std::string> strings;
    int entry{-1};  // the function that starts the program
};

}  // namespace wfc::compiler::ir
