#include "native_host.hpp"

#include "library.hpp"

#include "../interpreter/native_engine.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "../interpreter/interpreter.hpp"

namespace wfc::compiler {

namespace {

// ----- Strings ----------------------------------------------------------
//
// A string handle points to a `RtString`; the null handle is the empty
// string. A literal of the generated image has the same layout with a
// reference count of -1 and is never freed or counted.

struct RtString final {
    std::int32_t refs;
    std::int32_t length;
    char data[1];
};

constexpr std::int32_t kOmitted = INT32_MIN;  // an omitted Mid length

[[nodiscard]] std::int32_t length_of(const RtString* text) {
    return text == nullptr ? 0 : text->length;
}

[[nodiscard]] std::string_view view_of(const RtString* text) {
    return text == nullptr
               ? std::string_view{}
               : std::string_view(text->data,
                                  static_cast<std::size_t>(text->length));
}

// A new string with a reference count of one; the empty string is null.
[[nodiscard]] RtString* make_string(const char* data, const std::size_t size) {
    if (size == 0U) {
        return nullptr;
    }
    auto* text = static_cast<RtString*>(
        std::malloc(offsetof(RtString, data) + size + 1U));
    if (text == nullptr) {
        std::cerr << "out of memory\n";
        std::exit(1);
    }
    text->refs = 1;
    text->length = static_cast<std::int32_t>(size);
    std::memcpy(text->data, data, size);
    text->data[size] = '\0';
    return text;
}

[[nodiscard]] RtString* make_string(const std::string_view text) {
    return make_string(text.data(), text.size());
}

RtString* retain(RtString* text) {
    if (text != nullptr && text->refs > 0) {
        ++text->refs;
    }
    return text;
}

void release(RtString* text) {
    if (text != nullptr && text->refs > 0 && --text->refs == 0) {
        std::free(text);
    }
}

// ----- The services, in the order of `ir::Service` ----------------------

// The column of the output line the next character lands in (a comma in a
// Print moves to the next zone of 14 columns).
int g_column = 0;

// The output of the interpreter (forms) comes in order with the program's own.
void flush_engine_output();
bool g_engine_line_open = false;
void end_engine_line();

void put(const char* text, const std::size_t length) {
    flush_engine_output();
    end_engine_line();
    std::cout.write(text, static_cast<std::streamsize>(length));
    for (std::size_t i = 0; i < length; ++i) {
        g_column = text[i] == '\n' ? 0 : g_column + 1;
    }
}

void put(const std::string& text) {
    put(text.data(), text.size());
}

void print_number(const std::int32_t value) {
    // VB6 prints a number with a sign position and a trailing space.
    put(std::string(value >= 0 ? " " : "") + std::to_string(value) + ' ');
}

void print_boolean(const std::int32_t value) {
    put(std::string(value != 0 ? "True" : "False"));
}

void print_text(const char* text, const std::int32_t length) {
    put(text, static_cast<std::size_t>(length));
}

void print_newline() {
    put("\n", 1U);
}

// Tab(n): to column n (a new line first when the column has passed).
void print_tab(const std::int32_t column) {
    const auto at = static_cast<std::int32_t>(g_column) + 1;
    if (column < at) {
        put("\n", 1U);
        put(std::string(static_cast<std::size_t>(column > 1 ? column - 1 : 0),
                        ' '));
    } else {
        put(std::string(static_cast<std::size_t>(column - at), ' '));
    }
}

void print_spc(const std::int32_t count) {
    if (count > 0) {
        put(std::string(static_cast<std::size_t>(count), ' '));
    }
}

void print_zone() {
    put(std::string(static_cast<std::size_t>(14 - g_column % 14), ' '));
}

[[nodiscard]] std::string standard_error_text(const std::int32_t number) {
    switch (number) {
        case 5:
            return "Invalid procedure call or argument";
        case 6:
            return "Overflow";
        case 7:
            return "Out of memory";
        case 9:
            return "Subscript out of range";
        case 11:
            return "Division by zero";
        case 13:
            return "Type mismatch";
        case 28:
            return "Out of stack space";
        case 20:
            return "Resume without error";
        case 35:
            return "Sub, Function, or Property not defined";
        case 52:
            return "Bad file name or number";
        case 53:
            return "File not found";
        case 54:
            return "Bad file mode";
        case 55:
            return "File already open";
        case 58:
            return "File already exists";
        case 62:
            return "Input past end of file";
        case 70:
            return "Permission denied";
        case 75:
            return "Path/File access error";
        case 76:
            return "Path not found";
        case 91:
            return "Object variable or With block variable not set";
        case 92:
            return "For loop not initialized";
        case 94:
            return "Invalid use of Null";
        case 429:
            return "ActiveX component can't create object";
        case 438:
            return "Object doesn't support this property or method";
        case 445:
            return "Object doesn't support this action";
        case 448:
            return "Named argument not found";
        case 449:
            return "Argument not optional";
        case 450:
            return "Wrong number of arguments or invalid property assignment";
        case 451:
            return "Property let procedure not defined and property get "
                   "procedure did not return an object";
        case 457:
            return "This key is already associated with an element of this "
                   "collection";
        default:
            return "Application-defined or object-defined error";
    }
}

// The error state: Err and the flag that generated code tests after every
// operation that can fail. `pending` points into the loaded image.
struct ErrorState final {
    std::int32_t* pending{};
    std::int32_t number{};
    std::string description;
    std::string source;
    std::string help_file;
    std::int32_t help_context{};
};

ErrorState g_error;

// Records a run-time error and marks it pending. The caller returns at once;
// generated code sees the flag and goes to the procedure's handler.
void raise_error(const std::int32_t number) {
    g_error.number = number;
    g_error.description = standard_error_text(number);
    g_error.source = "Project1";
    g_error.help_file.clear();
    g_error.help_context = 0;
    if (g_error.pending != nullptr) {
        *g_error.pending = 1;
    }
}

void finish_output();

[[noreturn]] void end_program() {
    finish_output();
    std::exit(0);
}

void print_string(const RtString* text) {
    const auto content = view_of(text);
    put(content.data(), content.size());
}

RtString* string_concat(const RtString* a, const RtString* b) {
    if (length_of(a) == 0) {
        return retain(const_cast<RtString*>(b));
    }
    if (length_of(b) == 0) {
        return retain(const_cast<RtString*>(a));
    }
    std::string joined(view_of(a));
    joined += view_of(b);
    return make_string(joined);
}

// A fixed-length String: the text cut or padded with blanks to its length.
RtString* string_fixed(const RtString* text, const std::int32_t length) {
    std::string result(view_of(text));
    result.resize(static_cast<std::size_t>(length), ' ');
    return make_string(result);
}

void trace_call(const RtString* name) {
    std::fputs(std::string(view_of(name)).c_str(), stderr);
    std::fputc(10, stderr);
}

void string_assign(RtString** variable, RtString* value) {
    retain(value);
    release(*variable);
    *variable = value;
}

void string_release(RtString* text) {
    release(text);
}

void string_addref(RtString* text) {
    static_cast<void>(retain(text));
}

// Strings are UTF-8; Visual Basic counts and cuts them in UTF-16 units. ASCII
// text takes the quick paths here; anything else goes to the interpreter's own
// functions (defined with the Variants).
[[nodiscard]] bool all_ascii(const std::string_view text) {
    return std::all_of(text.begin(), text.end(), [](const char c) {
        return static_cast<unsigned char>(c) < 0x80U;
    });
}

RtString* via_engine_text(std::string_view function,
                          std::vector<wfc::detail::Value> arguments);
std::int32_t via_engine_long(std::string_view function,
                             std::vector<wfc::detail::Value> arguments);

[[nodiscard]] wfc::detail::Value text_value(const RtString* text) {
    return wfc::detail::Value{std::string(view_of(text))};
}

[[nodiscard]] wfc::detail::Value long_value(const std::int32_t number) {
    return wfc::detail::Value{static_cast<wfc::detail::Integer>(number)};
}

std::int32_t string_length(const RtString* text) {
    if (!all_ascii(view_of(text))) {
        return via_engine_long("len", {text_value(text)});
    }
    return length_of(text);
}

std::int32_t string_compare(const RtString* a, const RtString* b) {
    const auto left = view_of(a);
    const auto right = view_of(b);
    const int order = left.compare(right);
    return order < 0 ? -1 : (order > 0 ? 1 : 0);
}

std::int32_t string_like(const RtString* text, const RtString* pattern) {
    return wfc::detail::NativeEngine::like(std::string(view_of(text)),
                                           std::string(view_of(pattern)))
               ? -1
               : 0;
}

// Mid(variable, start[, length]) = text
void string_mid_assign(RtString** variable, const std::int32_t start,
                       const std::int32_t length, const RtString* text) {
    if (start < 1 || length < -1) {
        raise_error(5);
        return;
    }
    std::string target(view_of(*variable));
    const auto begin = static_cast<std::size_t>(start - 1);
    if (begin >= target.size()) {
        return;
    }
    std::size_t count = std::min<std::size_t>(
        static_cast<std::size_t>(length_of(text)), target.size() - begin);
    if (length >= 0) {
        count = std::min(count, static_cast<std::size_t>(length));
    }
    target.replace(begin, count, view_of(text).substr(0, count));
    RtString* replaced = make_string(target);
    release(*variable);
    *variable = replaced;
}

RtString* string_left(RtString* text, const std::int32_t count) {
    if (!all_ascii(view_of(text))) {
        return via_engine_text("left", {text_value(text), long_value(count)});
    }
    if (count < 0) {
        raise_error(5);
        return nullptr;
    }
    if (count >= length_of(text)) {
        return retain(text);
    }
    return make_string(text->data, static_cast<std::size_t>(count));
}

RtString* string_right(RtString* text, const std::int32_t count) {
    if (!all_ascii(view_of(text))) {
        return via_engine_text("right", {text_value(text), long_value(count)});
    }
    if (count < 0) {
        raise_error(5);
        return nullptr;
    }
    if (count >= length_of(text)) {
        return retain(text);
    }
    return make_string(text->data + (text->length - count),
                       static_cast<std::size_t>(count));
}

RtString* string_mid(RtString* text, const std::int32_t start,
                     const std::int32_t count) {
    if (!all_ascii(view_of(text))) {
        std::vector<wfc::detail::Value> arguments{text_value(text),
                                                  long_value(start)};
        if (count != kOmitted) {
            arguments.push_back(long_value(count));
        }
        return via_engine_text("mid", std::move(arguments));
    }
    if (start < 1 || (count < 0 && count != kOmitted)) {
        raise_error(5);
        return nullptr;
    }
    if (start > length_of(text)) {
        return nullptr;
    }
    const auto rest = static_cast<std::size_t>(text->length - (start - 1));
    const auto wanted = count == kOmitted
                            ? rest
                            : std::min(rest, static_cast<std::size_t>(count));
    if (start == 1 && wanted == static_cast<std::size_t>(text->length)) {
        return retain(text);
    }
    return make_string(text->data + (start - 1), wanted);
}

[[nodiscard]] char to_upper(const char c) {
    return c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c;
}

[[nodiscard]] char to_lower(const char c) {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

RtString* string_upper(const RtString* text) {
    if (!all_ascii(view_of(text))) {
        return via_engine_text("ucase", {text_value(text)});
    }
    std::string result(view_of(text));
    std::transform(result.begin(), result.end(), result.begin(), to_upper);
    return make_string(result);
}

RtString* string_lower(const RtString* text) {
    if (!all_ascii(view_of(text))) {
        return via_engine_text("lcase", {text_value(text)});
    }
    std::string result(view_of(text));
    std::transform(result.begin(), result.end(), result.begin(), to_lower);
    return make_string(result);
}

RtString* string_trim(const RtString* text) {
    auto content = view_of(text);
    while (!content.empty() && content.front() == ' ') {
        content.remove_prefix(1);
    }
    while (!content.empty() && content.back() == ' ') {
        content.remove_suffix(1);
    }
    return make_string(content);
}

RtString* string_ltrim(const RtString* text) {
    auto content = view_of(text);
    while (!content.empty() && content.front() == ' ') {
        content.remove_prefix(1);
    }
    return make_string(content);
}

RtString* string_rtrim(const RtString* text) {
    auto content = view_of(text);
    while (!content.empty() && content.back() == ' ') {
        content.remove_suffix(1);
    }
    return make_string(content);
}

RtString* string_chr(const std::int32_t code) {
    if (code > 127 && code <= 255) {
        return via_engine_text("chr", {long_value(code)});
    }
    if (code < 0 || code > 255) {
        raise_error(5);
        return nullptr;
    }
    const char c = static_cast<char>(code);
    return make_string(&c, 1U);
}

std::int32_t string_asc(const RtString* text) {
    if (length_of(text) == 0) {
        raise_error(5);
        return 0;
    }
    if (static_cast<unsigned char>(text->data[0]) >= 0x80U) {
        return via_engine_long("asc", {text_value(text)});
    }
    return static_cast<unsigned char>(text->data[0]);
}

RtString* string_from_number(const std::int32_t value) {
    const auto digits = std::to_string(value);
    return make_string(value >= 0 ? " " + digits : digits);
}

RtString* string_from_integer(const std::int32_t value) {
    return make_string(std::to_string(value));
}

RtString* string_from_boolean(const std::int32_t value) {
    return make_string(value != 0 ? "True" : "False");
}

std::int32_t string_instr(const std::int32_t start, const RtString* text,
                          const RtString* pattern) {
    if (start < 1) {
        raise_error(5);
        return 0;
    }
    if (!all_ascii(view_of(text)) || !all_ascii(view_of(pattern))) {
        return via_engine_long("instr", {long_value(start), text_value(text),
                                         text_value(pattern)});
    }
    const auto content = view_of(text);
    if (static_cast<std::size_t>(start) > content.size()) {
        return 0;
    }
    if (length_of(pattern) == 0) {
        return start;
    }
    const auto at =
        content.find(view_of(pattern), static_cast<std::size_t>(start - 1));
    return at == std::string_view::npos ? 0 : static_cast<std::int32_t>(at) + 1;
}

RtString* string_space(const std::int32_t count) {
    if (count < 0) {
        raise_error(5);
        return nullptr;
    }
    return make_string(std::string(static_cast<std::size_t>(count), ' '));
}

// VB6 writes a Double with 15 significant digits and a Single with 7 (C's %G
// rules: exponent form below 1E-4 or from 1E15 up); negative zero is "0".
[[nodiscard]] std::string format_float(const double value, const int digits) {
    if (std::isnan(value) || std::isinf(value)) {
        return std::to_string(value);
    }
    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), "%.*G", digits, value);
    std::string text = buffer;
    return text == "-0" ? "0" : text;
}

void print_double(const double value, const std::int32_t digits) {
    // A sign position before the number and a trailing space.
    put(std::string(value >= 0 ? " " : "") + format_float(value, digits) + ' ');
}

RtString* string_from_double(const double value, const std::int32_t digits,
                             const std::int32_t sign_space) {
    const auto text = format_float(value, digits);
    return make_string(sign_space != 0 && value >= 0 ? " " + text : text);
}

std::int32_t double_to_long(const double value) {
    const double rounded = std::nearbyint(value);  // to even, as CInt and CLng
    if (!(rounded >= -2147483648.0 && rounded <= 2147483647.0)) {
        raise_error(6);
        return 0;
    }
    return static_cast<std::int32_t>(rounded);
}

double math_function(const std::int32_t which, const double value) {
    switch (static_cast<ir::MathFunction>(which)) {
        case ir::MathFunction::square_root:
            if (value < 0) {
                raise_error(5);
                return 0;
            }
            return std::sqrt(value);
        case ir::MathFunction::sine:
            return std::sin(value);
        case ir::MathFunction::cosine:
            return std::cos(value);
        case ir::MathFunction::tangent:
            return std::tan(value);
        case ir::MathFunction::arctangent:
            return std::atan(value);
        case ir::MathFunction::logarithm:
            if (value <= 0) {
                raise_error(5);
                return 0;
            }
            return std::log(value);
        case ir::MathFunction::exponential: {
            const double result = std::exp(value);
            if (std::isinf(result)) {
                raise_error(6);
                return 0;
            }
            return result;
        }
        case ir::MathFunction::floor:
            return std::floor(value);
        case ir::MathFunction::truncate:
            return std::trunc(value);
        case ir::MathFunction::absolute:
            return std::fabs(value);
    }
    return value;
}

double double_power(const double base, const double exponent) {
    if (base == 0.0 && exponent < 0.0) {
        raise_error(11);
        return 0.0;
    }
    const double result = std::pow(base, exponent);
    if (!std::isfinite(result)) {
        raise_error(6);
        return 0.0;
    }
    return result;
}

std::int32_t double_sign(const double value) {
    return value > 0 ? 1 : (value < 0 ? -1 : 0);
}

// ----- Arrays -------------------------------------------------------------
//
// An array handle points to a `RtArray`; elements follow the header. Kind 0
// holds 32-bit integers (Boolean, Integer, Long), kind 1 doubles and kind 2
// string handles (released with the array). The null handle is an array that
// has not been dimensioned.

struct RtBound final {
    std::int32_t lower;
    std::int32_t upper;
};

struct RtArray final {
    std::int32_t kind;
    std::int32_t dims;
    std::int32_t fixed;
    std::int32_t count;
    RtBound bounds[3];
    std::int32_t size;  // bytes per element
    std::int32_t pad;
    const std::int32_t* strings;  // kind 3: a count, then the String offsets
};

[[nodiscard]] std::size_t element_size(const RtArray* array) {
    return static_cast<std::size_t>(array->size);
}

[[nodiscard]] char* data_of(RtArray* array) {
    return reinterpret_cast<char*>(array) + sizeof(RtArray);
}

// A string element occupies eight bytes on every architecture (the generated
// code uses 8-byte cells), whatever the size of a pointer.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324)  // padded to eight bytes on purpose
#endif
struct alignas(8) HandleCell final {
    RtString* handle;
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

struct RtVariant;
struct RtObject;
void variant_release(RtVariant* handle);
void object_release(RtObject* object);

RtVariant* variant_retain(RtVariant* handle);
void object_addref(RtObject* object);

// After a copy of the bytes of an array: its handles are shared.
void array_retain_elements(RtArray* array) {
    if (array->kind == 3 && array->strings != nullptr) {
        for (std::int32_t i = 0; i < array->count; ++i) {
            char* base =
                data_of(array) + static_cast<std::size_t>(i) *
                                     static_cast<std::size_t>(array->size);
            for (std::int32_t k = 0; k < array->strings[0]; ++k) {
                auto** handle =
                    reinterpret_cast<RtString**>(base + array->strings[1 + k]);
                retain(*handle);
            }
        }
    } else if (array->kind == 2) {
        auto* cells = reinterpret_cast<HandleCell*>(data_of(array));
        for (std::int32_t i = 0; i < array->count; ++i) {
            retain(cells[i].handle);
        }
    } else if (array->kind == 4 || array->kind == 5) {
        auto* cells = reinterpret_cast<void**>(data_of(array));
        for (std::int32_t i = 0; i < array->count; ++i) {
            if (array->kind == 4) {
                variant_retain(static_cast<RtVariant*>(cells[i]));
            } else {
                object_addref(static_cast<RtObject*>(cells[i]));
            }
        }
    }
}

void release_elements(RtArray* array) {
    if (array->kind == 3 && array->strings != nullptr) {
        for (std::int32_t i = 0; i < array->count; ++i) {
            char* base =
                data_of(array) + static_cast<std::size_t>(i) *
                                     static_cast<std::size_t>(array->size);
            for (std::int32_t k = 0; k < array->strings[0]; ++k) {
                auto** handle =
                    reinterpret_cast<RtString**>(base + array->strings[1 + k]);
                release(*handle);
                *handle = nullptr;
            }
        }
    } else if (array->kind == 2) {
        auto* cells = reinterpret_cast<HandleCell*>(data_of(array));
        for (std::int32_t i = 0; i < array->count; ++i) {
            release(cells[i].handle);
            cells[i].handle = nullptr;
        }
    } else if (array->kind == 4 || array->kind == 5) {
        auto* cells = reinterpret_cast<void**>(data_of(array));
        for (std::int32_t i = 0; i < array->count; ++i) {
            if (array->kind == 4) {
                variant_release(static_cast<RtVariant*>(cells[i]));
            } else {
                object_release(static_cast<RtObject*>(cells[i]));
            }
            cells[i] = nullptr;
        }
    }
}

void array_retain_elements(RtArray* array);
void array_release(RtArray* array);

// `target = source` for arrays: the target becomes a copy.
void array_assign(RtArray** variable, RtArray* source) {
    if (source == nullptr) {
        array_release(*variable);
        *variable = nullptr;
        return;
    }
    if (*variable == source) {
        return;
    }
    const std::size_t bytes =
        sizeof(RtArray) + static_cast<std::size_t>(source->count) *
                              static_cast<std::size_t>(source->size);
    auto* copy = static_cast<RtArray*>(std::malloc(bytes));
    if (copy == nullptr) {
        raise_error(7);
        return;
    }
    std::memcpy(copy, source, bytes);
    array_retain_elements(copy);
    array_release(*variable);
    *variable = copy;
}

void array_release(RtArray* array) {
    if (array != nullptr) {
        release_elements(array);
        std::free(array);
    }
}

void array_redim(RtArray** variable, const std::int32_t flags,
                 const std::int32_t dims, const std::int64_t* bounds,
                 const std::int32_t preserve, const std::int32_t record_size,
                 const std::int32_t* record_strings) {
    const std::int32_t kind = flags & 0xFF;
    if (dims < 1 || dims > 3) {
        raise_error(9);
        return;
    }
    RtArray header{};
    header.kind = kind;
    header.dims = dims;
    header.fixed = (flags >> 8) & 1;
    header.size = kind == 0 ? 4 : (kind == 3 ? record_size : 8);
    const bool empty_allowed = (flags & 0x200) != 0;
    header.strings = kind == 3 ? record_strings : nullptr;
    std::int64_t count = 1;
    for (std::int32_t k = 0; k < dims; ++k) {
        const auto lower = static_cast<std::int32_t>(bounds[2 * k]);
        const auto upper = static_cast<std::int32_t>(bounds[2 * k + 1]);
        if (upper < lower && !(empty_allowed && upper == lower - 1)) {
            raise_error(9);
            return;
        }
        header.bounds[k] = RtBound{lower, upper};
        count *= static_cast<std::int64_t>(upper) - lower + 1;
        if (count > (1LL << 28)) {
            raise_error(7);
            return;
        }
    }
    header.count = static_cast<std::int32_t>(count);
    auto* array = static_cast<RtArray*>(std::calloc(
        1, sizeof(RtArray) + static_cast<std::size_t>(count) *
                                 static_cast<std::size_t>(header.size)));
    if (array == nullptr) {
        raise_error(7);
        return;
    }
    *array = header;
    RtArray* old = *variable;
    if (preserve != 0 && old != nullptr && old->dims == dims &&
        old->kind == kind) {
        // Elements that exist in both keep their values.
        const std::size_t size = static_cast<std::size_t>(header.size);
        for (std::int32_t at = 0; at < old->count; ++at) {
            std::int32_t index[3] = {0, 0, 0};
            std::int32_t rest = at;
            for (std::int32_t k = dims - 1; k >= 0; --k) {
                const std::int32_t extent =
                    old->bounds[k].upper - old->bounds[k].lower + 1;
                index[k] = old->bounds[k].lower + rest % extent;
                rest /= extent;
            }
            std::int64_t target = 0;
            bool inside = true;
            for (std::int32_t k = 0; k < dims; ++k) {
                if (index[k] < header.bounds[k].lower ||
                    index[k] > header.bounds[k].upper) {
                    inside = false;
                    break;
                }
                target =
                    target *
                        (static_cast<std::int64_t>(header.bounds[k].upper) -
                         header.bounds[k].lower + 1) +
                    (index[k] - header.bounds[k].lower);
            }
            if (inside) {
                char* from = data_of(old) + static_cast<std::size_t>(at) * size;
                std::memcpy(
                    data_of(array) + static_cast<std::size_t>(target) * size,
                    from, size);
                if (kind == 2 || kind == 4 || kind == 5) {
                    std::memset(from, 0, size);  // the handle moved
                } else if (kind == 3 && old->strings != nullptr) {
                    for (std::int32_t k = 0; k < old->strings[0]; ++k) {
                        std::memset(from + old->strings[1 + k], 0, 8U);
                    }
                }
            }
        }
    }
    array_release(old);
    *variable = array;
}

// The offset of an index inside a dimension, or -1 after raising error 9.
[[nodiscard]] std::int64_t offset_in(const RtArray* array, const std::int32_t k,
                                     const std::int32_t index) {
    const auto& bound = array->bounds[k];
    if (index < bound.lower || index > bound.upper) {
        raise_error(9);
        return -1;
    }
    return static_cast<std::int64_t>(index) - bound.lower;
}

[[nodiscard]] std::int64_t extent_of(const RtArray* array,
                                     const std::int32_t k) {
    return static_cast<std::int64_t>(array->bounds[k].upper) -
           array->bounds[k].lower + 1;
}

[[nodiscard]] bool has_dims(const RtArray* array, const std::int32_t dims) {
    if (array == nullptr || array->dims != dims) {
        raise_error(9);
        return false;
    }
    return true;
}

[[nodiscard]] void* element_at(RtArray* array, const std::int64_t at) {
    return at < 0 ? nullptr
                  : data_of(array) +
                        static_cast<std::size_t>(at) * element_size(array);
}

void* array_element1(RtArray* array, const std::int32_t i) {
    if (!has_dims(array, 1)) {
        return nullptr;
    }
    return element_at(array, offset_in(array, 0, i));
}

void* array_element2(RtArray* array, const std::int32_t i,
                     const std::int32_t j) {
    if (!has_dims(array, 2)) {
        return nullptr;
    }
    const auto a = offset_in(array, 0, i);
    const auto b = offset_in(array, 1, j);
    return element_at(array, a < 0 || b < 0 ? -1 : a * extent_of(array, 1) + b);
}

void* array_element3(RtArray* array, const std::int32_t i, const std::int32_t j,
                     const std::int32_t k) {
    if (!has_dims(array, 3)) {
        return nullptr;
    }
    const auto a = offset_in(array, 0, i);
    const auto b = offset_in(array, 1, j);
    const auto c = offset_in(array, 2, k);
    return element_at(
        array, a < 0 || b < 0 || c < 0
                   ? -1
                   : (a * extent_of(array, 1) + b) * extent_of(array, 2) + c);
}

std::int32_t array_bound(const RtArray* array, const std::int32_t dimension,
                         const std::int32_t which) {
    if (array == nullptr || dimension < 1 || dimension > array->dims) {
        raise_error(9);
        return 0;
    }
    const auto& bound = array->bounds[dimension - 1];
    return which == 0 ? bound.lower : bound.upper;
}

std::int32_t array_count(const RtArray* array) {
    return array == nullptr ? 0 : array->count;
}

void* array_flat_element(RtArray* array, const std::int32_t index) {
    if (array == nullptr || index < 0 || index >= array->count) {
        raise_error(9);
        return nullptr;
    }
    return element_at(array, index);
}

void array_erase(RtArray** variable) {
    RtArray* array = *variable;
    if (array == nullptr) {
        return;
    }
    if (array->fixed != 0) {
        release_elements(array);
        std::memset(
            data_of(array), 0,
            static_cast<std::size_t>(array->count) * element_size(array));
        return;
    }
    array_release(array);
    *variable = nullptr;
}

std::int32_t error_number() {
    return g_error.number;
}

RtString* error_description() {
    return make_string(g_error.description);
}

RtString* error_source() {
    return make_string(g_error.source);
}

void error_clear() {
    g_error.number = 0;
    g_error.description.clear();
    g_error.source.clear();
    g_error.help_file.clear();
    g_error.help_context = 0;
}

RtString* error_help_file() {
    return make_string(g_error.help_file);
}

std::int32_t error_help_context() {
    return g_error.help_context;
}

// Err.Raise number[, source[, description]]
void error_raise(const std::int32_t number, const RtString* source,
                 const RtString* description) {
    raise_error(number);
    if (length_of(description) != 0) {
        g_error.description = std::string(view_of(description));
    }
    if (length_of(source) != 0) {
        g_error.source = std::string(view_of(source));
    }
}

void error_raise_help(const std::int32_t number, const RtString* source,
                      const RtString* description, const RtString* help_file,
                      const std::int32_t help_context) {
    error_raise(number, source, description);
    g_error.help_file = std::string(view_of(help_file));
    g_error.help_context = help_context;
}

// A handler took over: the error is no longer pending.
void error_acknowledge() {
    if (g_error.pending != nullptr) {
        *g_error.pending = 0;
    }
}

// ----- Variants (ADR-0009) ------------------------------------------------
//
// A Variant handle points to a `RtVariant`: a reference count and an
// interpreter `Value` that never changes after it is boxed. The null handle is
// Empty. The engine implements every operator and conversion.

struct RtVariant final {
    std::int32_t refs;
    wfc::detail::Value value;
};

std::unique_ptr<wfc::detail::NativeEngine> g_engine;

[[nodiscard]] wfc::detail::NativeEngine& engine() {
    if (!g_engine) {
        g_engine = std::make_unique<wfc::detail::NativeEngine>();
    }
    return *g_engine;
}

// The end of the program: what the interpreter printed, and its open line.
void finish_output() {
    flush_engine_output();
    if (g_engine_line_open) {
        std::cout.put(char{10});
        g_engine_line_open = false;
    }
    std::cout.flush();
}

void flush_engine_output() {
    if (!g_engine) {
        return;
    }
    bool open = false;
    const std::string text = g_engine->take_output(open);
    if (!text.empty()) {
        std::cout.write(text.data(), static_cast<std::streamsize>(text.size()));
        g_column = 0;
    }
    g_engine_line_open = open;
}

// The program prints after the interpreter left a line open: end it.
void end_engine_line() {
    if (g_engine_line_open && g_engine) {
        std::cout.put('\n');
        g_engine->close_output_line();
        g_engine_line_open = false;
        g_column = 0;
    }
}

[[nodiscard]] const wfc::detail::Value& value_of(const RtVariant* handle) {
    static const wfc::detail::Value empty{wfc::detail::Empty{}};
    return handle == nullptr ? empty : handle->value;
}

[[nodiscard]] RtVariant* box(wfc::detail::Value value) {
    if (std::holds_alternative<wfc::detail::Empty>(value)) {
        return nullptr;  // Empty is the null handle
    }
    return new RtVariant{1, std::move(value)};
}

RtVariant* variant_retain(RtVariant* handle) {
    if (handle != nullptr) {
        ++handle->refs;
    }
    return handle;
}

void variant_release(RtVariant* handle) {
    if (handle != nullptr && --handle->refs == 0) {
        delete handle;
    }
}

// Makes an error of the engine the pending error.
void raise_engine_error(const wfc::detail::NativeError& problem) {
    raise_error(problem.number);
    g_error.description = problem.description;
    if (!problem.source.empty()) {
        g_error.source = problem.source;
    }
}

RtVariant* variant_from_long(const std::int32_t value) {
    return box(wfc::detail::Value{static_cast<wfc::detail::Integer>(value)});
}

RtVariant* variant_from_byte(const std::int32_t value) {
    return box(wfc::detail::Value{static_cast<wfc::detail::Byte>(value)});
}

RtVariant* variant_from_integer(const std::int32_t value) {
    return box(wfc::detail::Value{static_cast<wfc::detail::Int16>(value)});
}

RtVariant* variant_from_double(const double value) {
    return box(wfc::detail::Value{value});
}

RtVariant* variant_from_single(const double value) {
    return box(wfc::detail::Value{static_cast<float>(value)});
}

RtVariant* variant_from_string(const RtString* text) {
    return box(wfc::detail::Value{std::string(view_of(text))});
}

RtVariant* variant_from_boolean(const std::int32_t value) {
    return box(wfc::detail::Value{value != 0});
}

RtVariant* variant_null() {
    return box(wfc::detail::Value{wfc::detail::Null{}});
}

void variant_assign(RtVariant** variable, RtVariant* value) {
    variant_retain(value);
    variant_release(*variable);
    *variable = value;
}

void variant_addref(RtVariant* handle) {
    static_cast<void>(variant_retain(handle));
}

RtVariant* variant_binary(const std::int32_t op, const RtVariant* a,
                          const RtVariant* b) {
    static const char* const names[] = {
        "+", "-", "*",  "/",  "\\",  "mod", "&",   "^",   "=",  "<>",
        "<", ">", "<=", ">=", "and", "or",  "xor", "eqv", "imp"};
    wfc::detail::NativeError problem;
    // Bits 8 and 9 say that an operand is a Variant of the program.
    auto result =
        engine().binary(names[op & 0xFF], value_of(a), value_of(b),
                        (op & 0x100) != 0, (op & 0x200) != 0, problem);
    if (!result.has_value()) {
        raise_engine_error(problem);
        return nullptr;
    }
    return box(std::move(*result));
}

RtVariant* variant_negate(const RtVariant* a) {
    wfc::detail::NativeError problem;
    auto result = engine().negate(value_of(a), problem);
    if (!result.has_value()) {
        raise_engine_error(problem);
        return nullptr;
    }
    return box(std::move(*result));
}

RtVariant* variant_not(const RtVariant* a) {
    wfc::detail::NativeError problem;
    auto result = engine().logical_not(value_of(a), problem);
    if (!result.has_value()) {
        raise_engine_error(problem);
        return nullptr;
    }
    return box(std::move(*result));
}

std::int32_t variant_to_long(const RtVariant* a) {
    wfc::detail::NativeError problem;
    const auto result = engine().to_long(value_of(a), problem);
    if (!result.has_value()) {
        raise_engine_error(problem);
        return 0;
    }
    return *result;
}

double variant_to_double(const RtVariant* a) {
    wfc::detail::NativeError problem;
    const auto result = engine().to_double(value_of(a), problem);
    if (!result.has_value()) {
        raise_engine_error(problem);
        return 0.0;
    }
    return *result;
}

RtString* variant_to_string(const RtVariant* a) {
    wfc::detail::NativeError problem;
    const auto result = engine().to_text(value_of(a), problem);
    if (!result.has_value()) {
        raise_engine_error(problem);
        return nullptr;
    }
    return make_string(*result);
}

std::int32_t variant_to_boolean(const RtVariant* a) {
    wfc::detail::NativeError problem;
    const auto result = engine().to_boolean(value_of(a), problem);
    if (!result.has_value()) {
        raise_engine_error(problem);
        return 0;
    }
    return *result ? -1 : 0;
}

void variant_print(const RtVariant* a) {
    if (std::holds_alternative<wfc::detail::ObjectInstance>(value_of(a))) {
        // An object prints its default member.
        wfc::detail::NativeError problem;
        const auto text = engine().to_text(value_of(a), problem);
        if (!text.has_value()) {
            raise_engine_error(problem);
            return;
        }
        put(*text);
        return;
    }
    put(wfc::detail::NativeEngine::print_text(value_of(a)));
}

RtString* variant_type_name(const RtVariant* a) {
    return make_string(engine().type_name(value_of(a)));
}

std::int32_t variant_var_type(const RtVariant* a) {
    return wfc::detail::NativeEngine::var_type(value_of(a));
}

std::int32_t variant_is_numeric(const RtVariant* a) {
    wfc::detail::NativeError problem;
    const auto& value = value_of(a);
    if (std::holds_alternative<std::string>(value)) {
        return engine().to_double(value, problem).has_value() ? -1 : 0;
    }
    const int type = wfc::detail::NativeEngine::var_type(value);
    return (type >= 2 && type <= 6) || type == 11 || type == 14 || type == 17
               ? -1
               : 0;
}

// ----- Objects of compiled classes (ADR-0009) ------------------------------
//
// An object is a block: a 16-byte header (the reference count and the class
// record), then the fields. The class record is a literal of the image: the
// size of the fields, the image offset of Class_Terminate (-1 for none), and
// the offsets and kinds of the fields that hold counted handles.

struct ClassRecord final {
    std::int32_t size;
    std::int32_t terminate;
    std::int32_t count;
    std::int32_t name_length;
    std::int32_t handles[1];  // offset, kind (ir::HandleKind) pairs, the name
};

struct RtObject final {
    std::int32_t refs;
    std::int32_t pad;
    const ClassRecord* record;
    wfc::detail::NativeStore* store;  // the store of a Collection (WfcStore)
};

[[nodiscard]] std::string class_name_of(const ClassRecord* record) {
    return std::string(
        reinterpret_cast<const char*>(&record->handles[2 * record->count]),
        static_cast<std::size_t>(record->name_length));
}

// The Variant wrapper of each object that has crossed into the Variant world.
std::unordered_map<struct RtObject*, std::weak_ptr<wfc::detail::InstanceData>>
    g_wrappers;

constexpr std::size_t kObjectHeaderSize = 24U;
static_assert(sizeof(RtObject) <= kObjectHeaderSize);

std::uint8_t* g_image_base = nullptr;

void object_release(RtObject* object);
void events_forget(RtObject* object);

// Releases the counted handle at `cell`, of the given kind.
void release_cell(void* cell, const std::int32_t kind) {
    switch (static_cast<ir::HandleKind>(kind)) {
        case ir::HandleKind::string:
            release(*static_cast<RtString**>(cell));
            *static_cast<RtString**>(cell) = nullptr;
            break;
        case ir::HandleKind::variant:
            variant_release(*static_cast<RtVariant**>(cell));
            *static_cast<RtVariant**>(cell) = nullptr;
            break;
        case ir::HandleKind::object:
            object_release(*static_cast<RtObject**>(cell));
            *static_cast<RtObject**>(cell) = nullptr;
            break;
        case ir::HandleKind::array:
            array_release(*static_cast<RtArray**>(cell));
            *static_cast<RtArray**>(cell) = nullptr;
            break;
    }
}

RtObject* object_new(const RtString* record_literal) {
    const auto* record =
        reinterpret_cast<const ClassRecord*>(record_literal->data);
    auto* object = static_cast<RtObject*>(std::calloc(
        1, kObjectHeaderSize + static_cast<std::size_t>(record->size)));
    if (object == nullptr) {
        raise_error(7);
        return nullptr;
    }
    object->refs = 1;
    object->record = record;
    return object;
}

void object_addref(RtObject* object) {
    if (object != nullptr) {
        ++object->refs;
    }
}

void object_release(RtObject* object) {
    if (object == nullptr || --object->refs > 0) {
        return;
    }
    object->refs = 1000000;  // a release inside Terminate must not recurse
    if (object->record->terminate >= 0) {
        // Class_Terminate runs even while an error is pending: it sees none.
        const std::int32_t saved =
            g_error.pending != nullptr ? *g_error.pending : 0;
        if (g_error.pending != nullptr) {
            *g_error.pending = 0;
        }
        reinterpret_cast<void (*)(void*)>(g_image_base +
                                          object->record->terminate)(object);
        if (g_error.pending != nullptr && *g_error.pending == 0) {
            *g_error.pending = saved;
        }
    }
    auto* fields = reinterpret_cast<std::uint8_t*>(object) + kObjectHeaderSize;
    for (std::int32_t i = 0; i < object->record->count; ++i) {
        release_cell(fields + object->record->handles[2 * i],
                     object->record->handles[2 * i + 1]);
    }
    g_wrappers.erase(object);
    events_forget(object);
    delete object->store;
    std::free(object);
}

// The objects that listen to the events of an object (WithEvents): the
// handler of event E is the method prefix + E of the sink.
struct EventSink final {
    RtObject* sink;
    std::string prefix;
};

std::unordered_map<RtObject*, std::vector<EventSink>> g_event_sinks;

void events_attach(RtObject* source, RtObject* sink, const RtString* prefix) {
    if (source == nullptr || sink == nullptr) {
        return;
    }
    auto& list = g_event_sinks[source];
    const std::string text(view_of(prefix));
    for (const auto& item : list) {
        if (item.sink == sink && item.prefix == text) {
            return;
        }
    }
    list.push_back(EventSink{sink, text});
}

void events_detach(RtObject* source, RtObject* sink, const RtString* prefix) {
    const auto found = g_event_sinks.find(source);
    if (source == nullptr || found == g_event_sinks.end()) {
        return;
    }
    const std::string text(view_of(prefix));
    auto& list = found->second;
    list.erase(std::remove_if(list.begin(), list.end(),
                              [&](const EventSink& item) {
                                  return item.sink == sink &&
                                         item.prefix == text;
                              }),
               list.end());
}

// An object that goes away: it has no sinks and is nobody's sink.
void events_forget(RtObject* object) {
    g_event_sinks.erase(object);
    for (auto& [source, list] : g_event_sinks) {
        list.erase(std::remove_if(list.begin(), list.end(),
                                  [&](const EventSink& item) {
                                      return item.sink == object;
                                  }),
                   list.end());
    }
}

void object_assign(RtObject** variable, RtObject* value) {
    object_addref(value);
    object_release(*variable);
    *variable = value;
}

void object_check(const RtObject* object) {
    if (object == nullptr) {
        raise_error(91);
    }
}

std::int32_t object_is(const RtObject* a, const RtObject* b) {
    return a == b ? -1 : 0;
}

// Whether the class of the record implements the interface named `name`
// (lower case): its member table has the marker "implements:name".
[[nodiscard]] bool record_implements(const ClassRecord* record,
                                     const std::string& name) {
    const std::string key = "implements:" + name;
    const char* at =
        reinterpret_cast<const char*>(&record->handles[2 * record->count]) +
        record->name_length;
    std::int32_t count = 0;
    std::memcpy(&count, at, sizeof(count));
    at += sizeof(count);
    for (std::int32_t i = 0; i < count; ++i) {
        std::int32_t length = 0;
        std::memcpy(&length, at, sizeof(length));
        at += 8;
        if (static_cast<std::size_t>(length) == key.size() &&
            std::memcmp(at, key.data(), key.size()) == 0) {
            return true;
        }
        at += length;
    }
    return false;
}

[[nodiscard]] bool object_is_a(const RtObject* object,
                               const RtString* record_literal) {
    if (object == nullptr) {
        return false;
    }
    if (reinterpret_cast<const char*>(object->record) == record_literal->data) {
        return true;
    }
    const auto* target =
        reinterpret_cast<const ClassRecord*>(record_literal->data);
    std::string name(
        reinterpret_cast<const char*>(&target->handles[2 * target->count]),
        static_cast<std::size_t>(target->name_length));
    for (auto& c : name) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return record_implements(object->record, name);
}

std::int32_t object_is_class(const RtObject* object,
                             const RtString* record_literal) {
    return object_is_a(object, record_literal) ? -1 : 0;
}

std::int32_t object_is_null(const RtObject* object) {
    return object == nullptr ? -1 : 0;
}

// An object inside the engine's world: it keeps the object alive and says
// what it is; its members are not reached late bound yet.
// The arguments of the call into a compiled member that is running (the
// thunk services read them; calls nest, so each frame remembers the last).
struct ThunkFrame final {
    std::vector<RtVariant*> arguments;
    std::vector<bool> stored;
};

ThunkFrame* g_thunk = nullptr;

std::int32_t thunk_count() {
    return g_thunk == nullptr
               ? 0
               : static_cast<std::int32_t>(g_thunk->arguments.size());
}

RtVariant* thunk_arg(const std::int32_t i) {
    if (g_thunk == nullptr || i < 0 ||
        static_cast<std::size_t>(i) >= g_thunk->arguments.size()) {
        return nullptr;
    }
    return variant_retain(g_thunk->arguments[static_cast<std::size_t>(i)]);
}

void thunk_store(const std::int32_t i, RtVariant* value) {
    if (g_thunk == nullptr || i < 0 ||
        static_cast<std::size_t>(i) >= g_thunk->arguments.size()) {
        return;
    }
    const auto at = static_cast<std::size_t>(i);
    variant_release(g_thunk->arguments[at]);
    g_thunk->arguments[at] = variant_retain(value);
    g_thunk->stored[at] = true;
}

// The image offset of the thunk with this key ("call:name", "get:name", ...)
// in a class record, or -1.
[[nodiscard]] std::int32_t find_thunk(const ClassRecord* record,
                                      const std::string& key) {
    const char* at =
        reinterpret_cast<const char*>(&record->handles[2 * record->count]) +
        record->name_length;
    std::int32_t count = 0;
    std::memcpy(&count, at, sizeof(count));
    at += sizeof(count);
    for (std::int32_t i = 0; i < count; ++i) {
        std::int32_t length = 0;
        std::int32_t offset = 0;
        std::memcpy(&length, at, sizeof(length));
        std::memcpy(&offset, at + 4, sizeof(offset));
        at += 8;
        if (static_cast<std::size_t>(length) == key.size() &&
            std::memcmp(at, key.data(), key.size()) == 0) {
            return offset;
        }
        at += length;
    }
    return -1;
}

[[nodiscard]] std::string lower_text(std::string text) {
    for (auto& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

// Calls a thunk of the object with the arguments the shared runtime has.
void call_thunk(RtObject* object, const std::int32_t offset,
                std::vector<wfc::detail::CallArgument>& arguments,
                const wfc::detail::Value* assigned, wfc::detail::Value& result,
                wfc::detail::NativeFailure& failure) {
    ThunkFrame frame;
    for (const auto& argument : arguments) {
        frame.arguments.push_back(box(argument.value));
    }
    if (assigned != nullptr) {
        frame.arguments.push_back(box(*assigned));
    }
    frame.stored.assign(frame.arguments.size(), false);
    ThunkFrame* saved = g_thunk;
    g_thunk = &frame;
    object_addref(object);
    RtVariant* returned = reinterpret_cast<RtVariant* (*)(RtObject*)>(
        g_image_base + offset)(object);
    g_thunk = saved;
    if (g_error.pending != nullptr && *g_error.pending != 0) {
        failure.number = g_error.number;
        failure.description = g_error.description;
        *g_error.pending = 0;
    } else {
        result = value_of(returned);
        for (std::size_t i = 0; i < arguments.size(); ++i) {
            if (frame.stored[i] && arguments[i].byref_target != nullptr) {
                *arguments[i].byref_target = value_of(frame.arguments[i]);
            }
        }
    }
    variant_release(returned);
    for (auto* handle : frame.arguments) {
        variant_release(handle);
    }
    object_release(object);
}

// Calls a member of a compiled object by name: `names` are alternatives
// separated by '|' (an interface member, then the member of the class
// itself). False when the object has none of them.
[[nodiscard]] bool call_member_by_name(
    RtObject* object, const std::string& names, const bool put,
    const wfc::detail::Value* assigned,
    std::vector<wfc::detail::CallArgument>& arguments,
    wfc::detail::Value& result, wfc::detail::NativeFailure& failure) {
    std::size_t begin = 0;
    while (begin <= names.size()) {
        auto end = names.find('|', begin);
        if (end == std::string::npos) {
            end = names.size();
        }
        const std::string name = lower_text(names.substr(begin, end - begin));
        begin = end + 1U;
        if (!put) {
            for (const char* kind : {"call:", "get:"}) {
                const auto offset = find_thunk(object->record, kind + name);
                if (offset >= 0) {
                    result = wfc::detail::Value{wfc::detail::Empty{}};
                    call_thunk(object, offset, arguments, nullptr, result,
                               failure);
                    return true;
                }
            }
        } else {
            const bool is_object =
                std::holds_alternative<wfc::detail::ObjectInstance>(
                    *assigned) ||
                std::holds_alternative<wfc::detail::Nothing>(*assigned);
            for (const char* kind : is_object ? std::array{"set:", "let:"}
                                              : std::array{"let:", "set:"}) {
                const auto offset = find_thunk(object->record, kind + name);
                if (offset >= 0) {
                    wfc::detail::Value ignored;
                    call_thunk(object, offset, arguments, assigned, ignored,
                               failure);
                    return true;
                }
            }
        }
    }
    return false;
}

class CompiledObject final : public wfc::detail::NativeObject {
public:
    explicit CompiledObject(RtObject* object) : object_(object) {
        object_addref(object_);
    }
    ~CompiledObject() override { object_release(object_); }
    [[nodiscard]] std::string type_name() const override {
        return class_name_of(object_->record);
    }
    [[nodiscard]] bool has_member(const std::string& name) const override {
        for (const char* kind : {"call:", "get:", "let:", "set:"}) {
            if (find_thunk(object_->record, kind + lower_text(name)) >= 0) {
                return true;
            }
        }
        return false;
    }
    [[nodiscard]] bool get(wfc::detail::Interpreter&, const std::string& name,
                           std::vector<wfc::detail::CallArgument>& arguments,
                           wfc::detail::Value& result,
                           wfc::detail::NativeFailure& failure) override {
        for (const char* kind : {"call:", "get:"}) {
            const auto offset =
                find_thunk(object_->record, kind + lower_text(name));
            if (offset >= 0) {
                result = wfc::detail::Value{wfc::detail::Empty{}};
                call_thunk(object_, offset, arguments, nullptr, result,
                           failure);
                return true;
            }
        }
        wfc::detail::Value host;
        if (!form_host(host, failure)) {
            return false;
        }
        if (failure.number != 0) {
            return true;
        }
        std::vector<wfc::detail::Value> values;
        for (const auto& argument : arguments) {
            values.push_back(argument.value);
        }
        wfc::detail::NativeError problem;
        auto found =
            engine().member_get(host, lower_text(name), values, problem);
        if (!found.has_value()) {
            failure.number = problem.number;
            failure.description = problem.description;
        } else {
            result = std::move(*found);
        }
        return true;
    }
    [[nodiscard]] bool put(wfc::detail::Interpreter&, const std::string& name,
                           std::vector<wfc::detail::CallArgument>& arguments,
                           const wfc::detail::Value& value,
                           wfc::detail::NativeFailure& failure) override {
        const bool is_object =
            std::holds_alternative<wfc::detail::ObjectInstance>(value) ||
            std::holds_alternative<wfc::detail::Nothing>(value);
        for (const char* kind : is_object ? std::array{"set:", "let:"}
                                          : std::array{"let:", "set:"}) {
            const auto offset =
                find_thunk(object_->record, kind + lower_text(name));
            if (offset >= 0) {
                wfc::detail::Value ignored;
                call_thunk(object_, offset, arguments, &value, ignored,
                           failure);
                return true;
            }
        }
        wfc::detail::Value host;
        if (!form_host(host, failure)) {
            return false;
        }
        if (failure.number != 0) {
            return true;
        }
        std::vector<wfc::detail::Value> values;
        for (const auto& argument : arguments) {
            values.push_back(argument.value);
        }
        wfc::detail::NativeError problem;
        if (!engine().member_put(host, lower_text(name), values, value,
                                 is_object, problem)) {
            failure.number = problem.number;
            failure.description = problem.description;
        }
        return true;
    }
    [[nodiscard]] RtObject* object() const { return object_; }

private:
    // The form of the engine this object stands for (false: not a form).
    [[nodiscard]] bool form_host(wfc::detail::Value& host,
                                 wfc::detail::NativeFailure& failure) const {
        const auto offset = find_thunk(object_->record, "call:wfcgethost");
        if (offset < 0) {
            return false;
        }
        std::vector<wfc::detail::CallArgument> none;
        host = wfc::detail::Value{wfc::detail::Empty{}};
        call_thunk(object_, offset, none, nullptr, host, failure);
        return true;
    }

    RtObject* object_;
};

// The forms of the engine get their code from compiled classes.
std::unordered_map<std::string, const ClassRecord*> g_class_records;

class FormHooks final : public wfc::detail::NativeHooks {
public:
    void form_created(
        const std::shared_ptr<wfc::detail::InstanceData>& form) override {
        const auto found = g_class_records.find(lower_text(form->class_name));
        if (found == g_class_records.end()) {
            return;
        }
        const ClassRecord* record = found->second;
        const auto made = find_thunk(record, "new:");
        if (made < 0) {
            return;
        }
        auto* object = reinterpret_cast<RtObject* (*)()>(g_image_base + made)();
        if (object == nullptr ||
            (g_error.pending != nullptr && *g_error.pending != 0)) {
            if (g_error.pending != nullptr) {
                *g_error.pending = 0;
            }
            return;
        }
        // The compiled code reaches the window through this link.
        if (const auto link = find_thunk(record, "call:wfcsethost");
            link >= 0) {
            std::vector<wfc::detail::CallArgument> arguments;
            arguments.push_back(wfc::detail::CallArgument{
                wfc::detail::Value{wfc::detail::ObjectInstance{form}},
                nullptr,
                false,
                {}});
            wfc::detail::Value ignored;
            wfc::detail::NativeFailure failure;
            call_thunk(object, link, arguments, nullptr, ignored, failure);
        }
        form->compiled = std::shared_ptr<void>(object, [](void* held) {
            object_release(static_cast<RtObject*>(held));
        });
    }

    bool has(wfc::detail::InstanceData& form,
             const std::string& name) override {
        auto* object = static_cast<RtObject*>(form.compiled.get());
        if (object == nullptr) {
            return false;
        }
        const std::string key = lower_text(name);
        for (const char* kind : {"call:", "get:", "let:", "set:"}) {
            if (find_thunk(object->record, kind + key) >= 0) {
                return true;
            }
        }
        return false;
    }

    bool get(wfc::detail::InstanceData& form, const std::string& name,
             std::vector<wfc::detail::CallArgument>& arguments,
             wfc::detail::Value& result,
             wfc::detail::NativeFailure& failure) override {
        auto* object = static_cast<RtObject*>(form.compiled.get());
        if (object == nullptr) {
            return false;
        }
        for (const char* kind : {"call:", "get:"}) {
            const auto offset =
                find_thunk(object->record, kind + lower_text(name));
            if (offset >= 0) {
                result = wfc::detail::Value{wfc::detail::Empty{}};
                call_thunk(object, offset, arguments, nullptr, result, failure);
                return true;
            }
        }
        return false;
    }

    bool put(wfc::detail::InstanceData& form, const std::string& name,
             std::vector<wfc::detail::CallArgument>& arguments,
             const wfc::detail::Value& value,
             wfc::detail::NativeFailure& failure) override {
        auto* object = static_cast<RtObject*>(form.compiled.get());
        if (object == nullptr) {
            return false;
        }
        for (const char* kind : {"let:", "set:"}) {
            const auto offset =
                find_thunk(object->record, kind + lower_text(name));
            if (offset >= 0) {
                wfc::detail::Value ignored;
                call_thunk(object, offset, arguments, &value, ignored, failure);
                return true;
            }
        }
        return false;
    }
};

FormHooks g_form_hooks;

// The Value that stands for an object (one wrapper per object while any copy
// of the Value lives, so that `Is` still works).
[[nodiscard]] wfc::detail::Value wrap_object(RtObject* object) {
    using namespace wfc::detail;
    if (object == nullptr) {
        return Value{Nothing{}};
    }
    if (const auto found = g_wrappers.find(object); found != g_wrappers.end()) {
        if (auto alive = found->second.lock()) {
            return Value{ObjectInstance{std::move(alive)}};
        }
    }
    auto data = std::make_shared<InstanceData>();
    data->class_name = "wfcnative";
    data->native = std::make_shared<CompiledObject>(object);
    g_wrappers[object] = data;
    return Value{ObjectInstance{std::move(data)}};
}

// The compiled object a Value stands for (null for Nothing and Empty).
[[nodiscard]] RtObject* object_of(const wfc::detail::Value& value,
                                  bool& valid) {
    using namespace wfc::detail;
    valid = true;
    if (const auto* instance = std::get_if<ObjectInstance>(&value)) {
        if (const auto* compiled =
                dynamic_cast<CompiledObject*>(instance->data->native.get())) {
            return compiled->object();
        }
        valid = false;  // an object of the engine: not reachable yet
        return nullptr;
    }
    if (std::holds_alternative<Nothing>(value) ||
        std::holds_alternative<Empty>(value)) {
        return nullptr;
    }
    valid = false;
    return nullptr;
}

void* inline_element(void* base, const std::int32_t index,
                     const std::int32_t lower, const std::int32_t count,
                     const std::int32_t size) {
    if (index < lower || index - lower >= count) {
        raise_error(9);
        return nullptr;
    }
    return static_cast<char*>(base) +
           static_cast<std::int64_t>(index - lower) * size;
}

}  // namespace

bool is_library_function(const std::string_view name) {
    return wfc::detail::NativeEngine::is_library_function(name);
}

namespace {

// The argument of an omitted Optional Variant: an Empty that IsMissing knows.
RtVariant g_missing{1000000000, wfc::detail::Value{wfc::detail::Empty{}}};

RtVariant* variant_missing() {
    ++g_missing.refs;
    return &g_missing;
}

std::int32_t variant_is_missing(const RtVariant* handle) {
    return handle == &g_missing ? -1 : 0;
}

void randomize(const std::int32_t has_seed, const double seed,
               const std::int32_t is_timer) {
    engine().randomize(has_seed != 0, seed, is_timer != 0);
}

RtVariant* variant_from_object(RtObject* object);

RtString* via_engine_text(std::string_view function,
                          std::vector<wfc::detail::Value> arguments) {
    wfc::detail::NativeError problem;
    auto result = engine().call_function(function, arguments, problem);
    if (!result.has_value()) {
        raise_engine_error(problem);
        return nullptr;
    }
    return make_string(wfc::detail::NativeEngine::render(*result));
}

std::int32_t via_engine_long(std::string_view function,
                             std::vector<wfc::detail::Value> arguments) {
    wfc::detail::NativeError problem;
    auto result = engine().call_function(function, arguments, problem);
    if (!result.has_value()) {
        raise_engine_error(problem);
        return 0;
    }
    const auto number = engine().to_long(*result, problem);
    return number.value_or(0);
}

// The arguments of a late-bound call: handles read upward from the block.
[[nodiscard]] std::vector<wfc::detail::Value> block_values(
    const std::int32_t count, const char* block) {
    std::vector<wfc::detail::Value> values;
    for (std::int32_t i = 0; i < count; ++i) {
        RtVariant* handle;
        std::memcpy(&handle, block + 8 * i, sizeof(handle));
        values.push_back(value_of(handle));
    }
    return values;
}

// Which of the arguments in the block were left out.
[[nodiscard]] std::uint32_t omitted_in(const std::int32_t count,
                                       const char* block) {
    std::uint32_t mask = 0;
    for (std::int32_t i = 0; i < count && i < 32; ++i) {
        RtVariant* handle;
        std::memcpy(&handle, block + 8 * i, sizeof(handle));
        if (handle == &g_missing) {
            mask |= std::uint32_t{1} << i;
        }
    }
    return mask;
}

RtVariant* host_global(const RtString* name) {
    wfc::detail::NativeError problem;
    auto result = engine().global(std::string(view_of(name)), problem);
    if (!result.has_value()) {
        raise_engine_error(problem);
        return nullptr;
    }
    return box(std::move(*result));
}

RtVariant* host_new(const RtString* name) {
    wfc::detail::NativeError problem;
    auto result = engine().new_form(std::string(view_of(name)), problem);
    if (!result.has_value()) {
        raise_engine_error(problem);
        return nullptr;
    }
    return box(std::move(*result));
}

// `words = Split(text)`: the array a Variant holds becomes the array of the
// variable (element kind 0 whole numbers, 1 fractions, 2 Strings, 4 Variants).
void array_from_variant(RtArray** variable, const RtVariant* source,
                        const std::int32_t kind) {
    const auto* held = std::get_if<wfc::detail::ArrayValue>(&value_of(source));
    if (held == nullptr) {
        raise_error(13);
        return;
    }
    const auto count = static_cast<std::int32_t>(held->elements.size());
    RtArray header{};
    header.kind = kind;
    header.dims = 1;
    header.size = kind == 0 ? 4 : 8;
    header.count = count;
    header.bounds[0] =
        RtBound{static_cast<std::int32_t>(held->lower_bound),
                static_cast<std::int32_t>(held->lower_bound) + count - 1};
    auto* array = static_cast<RtArray*>(std::calloc(
        1, sizeof(RtArray) + static_cast<std::size_t>(count) *
                                 static_cast<std::size_t>(header.size)));
    if (array == nullptr) {
        raise_error(7);
        return;
    }
    *array = header;
    wfc::detail::NativeError problem;
    for (std::int32_t i = 0; i < count; ++i) {
        const auto& item = held->elements[static_cast<std::size_t>(i)];
        char* where =
            data_of(array) + static_cast<std::size_t>(i) * header.size;
        if (kind == 2) {
            const auto text = engine().to_text(item, problem);
            if (!text.has_value()) {
                array_release(array);
                raise_engine_error(problem);
                return;
            }
            *reinterpret_cast<RtString**>(where) = make_string(*text);
        } else if (kind == 4) {
            *reinterpret_cast<RtVariant**>(where) = box(item);
        } else if (kind == 1) {
            const auto number = engine().to_double(item, problem);
            if (!number.has_value()) {
                array_release(array);
                raise_engine_error(problem);
                return;
            }
            std::memcpy(where, &*number, sizeof(double));
        } else {
            const auto number = engine().to_long(item, problem);
            if (!number.has_value()) {
                array_release(array);
                raise_engine_error(problem);
                return;
            }
            std::memcpy(where, &*number, sizeof(std::int32_t));
        }
    }
    array_release(*variable);
    *variable = array;
}

// `Join(words, " ")`, `v = a`: a one-dimensional array as the array a Variant
// holds (element kinds as for array_from_variant).
RtVariant* array_to_variant(const RtArray* array) {
    if (array == nullptr || array->dims != 1 || array->kind == 3) {
        raise_error(13);
        return nullptr;
    }
    wfc::detail::ArrayValue held{};
    held.lower_bound = array->bounds[0].lower;
    held.is_variant_element = true;
    held.element_type_index = wfc::detail::Value{wfc::detail::Empty{}}.index();
    const char* base = reinterpret_cast<const char*>(array) + sizeof(RtArray);
    for (std::int32_t i = 0; i < array->count; ++i) {
        const char* where = base + static_cast<std::size_t>(i) * array->size;
        if (array->kind == 2) {
            const RtString* text;
            std::memcpy(&text, where, sizeof(text));
            held.elements.emplace_back(std::string(view_of(text)));
        } else if (array->kind == 4) {
            const RtVariant* item;
            std::memcpy(&item, where, sizeof(item));
            held.elements.push_back(value_of(item));
        } else if (array->kind == 1) {
            double number;
            std::memcpy(&number, where, sizeof(number));
            held.elements.emplace_back(number);
        } else {
            std::int32_t number;
            std::memcpy(&number, where, sizeof(number));
            held.elements.emplace_back(
                static_cast<wfc::detail::Integer>(number));
        }
    }
    return box(wfc::detail::Value{std::move(held)});
}

// ----- Records handed to routines of DLLs ----------------------------------
//
// The descriptor: the lower-case name of the type, a count and for each field
// its name, its kind (1 Integer, 2 Long, 3 Byte, 4 Boolean, 5 Single,
// 6 Double, 7 String) and its offset in the record.

struct RecordField final {
    std::string name;
    std::int32_t kind;
    std::int32_t offset;
};

struct RecordShape final {
    std::string type;
    std::vector<RecordField> fields;
};

[[nodiscard]] RecordShape shape_of(const RtString* descriptor) {
    RecordShape shape;
    const char* at = descriptor->data;
    const auto read_int = [&at]() {
        std::int32_t value = 0;
        std::memcpy(&value, at, sizeof(value));
        at += sizeof(value);
        return value;
    };
    const auto read_text = [&]() {
        const auto length = read_int();
        std::string text(at, static_cast<std::size_t>(length));
        at += length;
        return text;
    };
    shape.type = read_text();
    const auto count = read_int();
    for (std::int32_t i = 0; i < count; ++i) {
        RecordField field;
        field.name = read_text();
        field.kind = read_int();
        field.offset = read_int();
        shape.fields.push_back(std::move(field));
    }
    return shape;
}

RtVariant* udt_to_engine(const RtString* descriptor, const char* record) {
    const auto shape = shape_of(descriptor);
    wfc::detail::NativeError problem;
    auto made = engine().new_record(shape.type, problem);
    if (!made.has_value()) {
        raise_engine_error(problem);
        return nullptr;
    }
    for (const auto& field : shape.fields) {
        const char* where = record + field.offset;
        wfc::detail::Value value{wfc::detail::Empty{}};
        switch (field.kind) {
            case 1: {
                std::int32_t number = 0;
                std::memcpy(&number, where, sizeof(number));
                value =
                    wfc::detail::Value{static_cast<wfc::detail::Int16>(number)};
                break;
            }
            case 2: {
                std::int32_t number = 0;
                std::memcpy(&number, where, sizeof(number));
                value = wfc::detail::Value{
                    static_cast<wfc::detail::Integer>(number)};
                break;
            }
            case 3: {
                std::int32_t number = 0;
                std::memcpy(&number, where, sizeof(number));
                value =
                    wfc::detail::Value{static_cast<wfc::detail::Byte>(number)};
                break;
            }
            case 4: {
                std::int32_t number = 0;
                std::memcpy(&number, where, sizeof(number));
                value = wfc::detail::Value{number != 0};
                break;
            }
            case 5:
            case 6: {
                double number = 0;
                std::memcpy(&number, where, sizeof(number));
                value = field.kind == 5
                            ? wfc::detail::Value{static_cast<float>(number)}
                            : wfc::detail::Value{number};
                break;
            }
            default: {
                const RtString* text = nullptr;
                std::memcpy(&text, where, sizeof(text));
                value = wfc::detail::Value{std::string(view_of(text))};
                break;
            }
        }
        wfc::detail::NativeEngine::set_field(*made, field.name,
                                             std::move(value));
    }
    return box(std::move(*made));
}

void udt_from_engine(const RtString* descriptor, char* record,
                     const RtVariant* held) {
    const auto shape = shape_of(descriptor);
    for (const auto& field : shape.fields) {
        char* where = record + field.offset;
        const auto value =
            wfc::detail::NativeEngine::field(value_of(held), field.name);
        wfc::detail::NativeError problem;
        if (field.kind == 7) {
            RtString** cell = reinterpret_cast<RtString**>(where);
            const auto text = engine().to_text(value, problem);
            RtString* made = make_string(text.value_or(std::string{}));
            release(*cell);
            *cell = made;
        } else if (field.kind == 5 || field.kind == 6) {
            const double number =
                engine().to_double(value, problem).value_or(0.0);
            std::memcpy(where, &number, sizeof(number));
        } else {
            const std::int32_t number =
                engine().to_long(value, problem).value_or(0);
            std::memcpy(where, &number, sizeof(number));
        }
    }
}

// `RaiseEvent Name(args)`: every sink of the object runs its handler.
void raise_event(RtObject* source, const RtString* name,
                 const std::int32_t count, const char* block) {
    const auto found = g_event_sinks.find(source);
    if (found == g_event_sinks.end()) {
        return;
    }
    const auto sinks = found->second;  // handlers may change the list
    auto arguments = block_values(count, block);
    // By-reference arguments (Cancel) are handed back through the block.
    const auto hand_back = [&]() {
        auto* slots = const_cast<char*>(block);
        for (std::int32_t i = 0; i < count; ++i) {
            RtVariant* old;
            std::memcpy(&old, slots + 8 * i, sizeof(old));
            RtVariant* now = box(arguments[static_cast<std::size_t>(i)]);
            variant_release(old);
            std::memcpy(slots + 8 * i, &now, sizeof(now));
        }
    };
    for (const auto& item : sinks) {
        std::vector<wfc::detail::CallArgument> call;
        for (auto& argument : arguments) {
            call.push_back(
                wfc::detail::CallArgument{argument, &argument, false, {}});
        }
        wfc::detail::Value ignored{wfc::detail::Empty{}};
        wfc::detail::NativeFailure failure;
        object_addref(item.sink);
        static_cast<void>(call_member_by_name(
            item.sink, item.prefix + std::string(view_of(name)), false, nullptr,
            call, ignored, failure));
        object_release(item.sink);
        if (failure.number != 0) {
            raise_error(static_cast<std::int32_t>(failure.number));
            g_error.description = failure.description;
            return;
        }
    }
    hand_back();
}

RtVariant* variant_member_get(const RtVariant* object, const RtString* name,
                              const std::int32_t count, const char* block) {
    wfc::detail::NativeError problem;
    auto arguments = block_values(count, block);
    std::optional<wfc::detail::Value> result;
    bool valid = false;
    if (RtObject* compiled = object_of(value_of(object), valid);
        valid && compiled != nullptr) {
        // An object of compiled code: its members are called directly.
        std::vector<wfc::detail::CallArgument> call;
        for (auto& argument : arguments) {
            call.push_back(
                wfc::detail::CallArgument{argument, &argument, false, {}});
        }
        wfc::detail::Value value{wfc::detail::Empty{}};
        wfc::detail::NativeFailure failure;
        const std::string names(view_of(name));
        const bool found = call_member_by_name(compiled, names, false, nullptr,
                                               call, value, failure);
        if (!found) {
            // A form of compiled code: the rest is the form of the engine.
            wfc::detail::NativeEngine::omit_arguments(omitted_in(count, block));
            result = engine().member_get(value_of(object), lower_text(names),
                                         arguments, problem);
        } else if (failure.number != 0) {
            problem.number = static_cast<int>(failure.number);
            problem.description = failure.description;
        } else {
            result = std::move(value);
        }
    } else {
        wfc::detail::NativeEngine::omit_arguments(omitted_in(count, block));
        result = engine().member_get(
            value_of(object), std::string(view_of(name)), arguments, problem);
    }
    if (!result.has_value()) {
        raise_engine_error(problem);
        return nullptr;
    }
    // By-reference arguments: the slots now hold the values the call left.
    auto* slots = const_cast<char*>(block);
    for (std::int32_t i = 0; i < count; ++i) {
        RtVariant* old;
        std::memcpy(&old, slots + 8 * i, sizeof(old));
        RtVariant* now = box(arguments[static_cast<std::size_t>(i)]);
        variant_release(old);
        std::memcpy(slots + 8 * i, &now, sizeof(now));
    }
    return box(std::move(*result));
}

void variant_member_put(const RtVariant* object, const RtString* name,
                        const std::int32_t count, const char* block,
                        const RtVariant* value, const std::int32_t is_set) {
    wfc::detail::NativeError problem;
    auto arguments = block_values(count, block);
    bool valid = false;
    if (RtObject* compiled = object_of(value_of(object), valid);
        valid && compiled != nullptr) {
        std::vector<wfc::detail::CallArgument> call;
        for (auto& argument : arguments) {
            call.push_back(
                wfc::detail::CallArgument{argument, &argument, false, {}});
        }
        wfc::detail::Value ignored;
        wfc::detail::NativeFailure failure;
        const std::string names(view_of(name));
        const wfc::detail::Value assigned = value_of(value);
        const bool found = call_member_by_name(compiled, names, true, &assigned,
                                               call, ignored, failure);
        if (!found) {
            wfc::detail::NativeEngine::omit_arguments(omitted_in(count, block));
            if (!engine().member_put(value_of(object), lower_text(names),
                                     arguments, assigned, is_set != 0,
                                     problem)) {
                raise_engine_error(problem);
            }
        } else if (failure.number != 0) {
            raise_error(static_cast<std::int32_t>(failure.number));
            g_error.description = failure.description;
        }
        return;
    }
    wfc::detail::NativeEngine::omit_arguments(omitted_in(count, block));
    if (!engine().member_put(value_of(object), std::string(view_of(name)),
                             arguments, value_of(value), is_set != 0,
                             problem)) {
        raise_engine_error(problem);
    }
}

std::int32_t variant_is(const RtVariant* a, const RtVariant* b) {
    return wfc::detail::NativeEngine::same_object(value_of(a), value_of(b)) ? -1
                                                                            : 0;
}

std::int32_t variant_type_of(const RtVariant* object,
                             const RtString* record_literal) {
    bool valid = false;
    const RtObject* compiled = object_of(value_of(object), valid);
    return valid && compiled != nullptr && object_is_a(compiled, record_literal)
               ? -1
               : 0;
}

RtVariant* variant_items(const RtVariant* object) {
    wfc::detail::NativeError problem;
    auto items = engine().items(value_of(object), problem);
    if (!items.has_value()) {
        raise_engine_error(problem);
        return nullptr;
    }
    wfc::detail::ArrayValue array{};
    array.elements = std::move(*items);
    array.is_variant_element = true;
    array.element_type_index = wfc::detail::Value{wfc::detail::Empty{}}.index();
    return box(wfc::detail::Value{std::move(array)});
}

void run_forms() {
    wfc::detail::NativeError problem;
    if (!engine().run_forms(problem)) {
        raise_engine_error(problem);
    }
}

// What the engine sees of a value: a compiled form is its window.
[[nodiscard]] wfc::detail::Value engine_value(const wfc::detail::Value& value) {
    if (const auto* instance =
            std::get_if<wfc::detail::ObjectInstance>(&value)) {
        if (const auto* compiled =
                dynamic_cast<CompiledObject*>(instance->data->native.get())) {
            const auto offset =
                find_thunk(compiled->object()->record, "call:wfcgethost");
            if (offset >= 0) {
                std::vector<wfc::detail::CallArgument> none;
                wfc::detail::Value host{wfc::detail::Empty{}};
                wfc::detail::NativeFailure failure;
                call_thunk(compiled->object(), offset, none, nullptr, host,
                           failure);
                return host;
            }
        }
    }
    return value;
}

void engine_statement(const RtString* text, const std::int32_t count,
                      const char* in_block, char* out_block,
                      const std::int32_t variant_mask) {
    std::vector<wfc::detail::Value> values;
    for (std::int32_t i = 0; i < count; ++i) {
        RtVariant* handle;
        std::memcpy(&handle, in_block + 8 * i, sizeof(handle));
        values.push_back(engine_value(value_of(handle)));
    }
    wfc::detail::NativeError problem;
    if (!engine().run_statement(std::string(view_of(text)), values,
                                static_cast<std::uint32_t>(variant_mask),
                                problem)) {
        raise_engine_error(problem);
        return;
    }
    for (std::int32_t i = 0; i < count; ++i) {
        RtVariant* handle = box(std::move(values[static_cast<std::size_t>(i)]));
        std::memcpy(out_block + 8 * i, &handle, sizeof(handle));
    }
}

RtVariant* variant_call(const RtString* name, const std::int32_t count,
                        const char* block) {
    std::vector<wfc::detail::Value> arguments;
    for (std::int32_t i = 0; i < count; ++i) {
        RtVariant* handle;
        std::memcpy(&handle, block + 8 * i, sizeof(handle));
        arguments.push_back(value_of(handle));
    }
    // WfcStore works on the store of the object it is given: lend it.
    RtObject* lender = nullptr;
    std::shared_ptr<wfc::detail::InstanceData> lent;
    if (view_of(name) == "wfcstore" && arguments.size() >= 2U) {
        bool valid = false;
        lender = object_of(arguments[1], valid);
        if (lender != nullptr) {
            lent = std::get<wfc::detail::ObjectInstance>(arguments[1]).data;
            lent->store.reset(lender->store);
            lender->store = nullptr;
        }
    }
    wfc::detail::NativeError problem;
    auto result = engine().call_function(view_of(name), arguments, problem);
    if (lender != nullptr) {
        lender->store = lent->store.release();
    }
    if (!result.has_value()) {
        raise_engine_error(problem);
        return nullptr;
    }
    return box(std::move(*result));
}

RtVariant* variant_from_object(RtObject* object) {
    return box(wrap_object(object));
}

RtObject* variant_to_object(const RtVariant* handle) {
    bool valid = false;
    RtObject* object = object_of(value_of(handle), valid);
    if (!valid) {
        raise_error(13);
        return nullptr;
    }
    object_addref(object);
    return object;
}

std::int32_t variant_array_count(const RtVariant* handle) {
    if (const auto* array =
            std::get_if<wfc::detail::ArrayValue>(&value_of(handle))) {
        return static_cast<std::int32_t>(array->elements.size());
    }
    return 0;
}

RtVariant* variant_array_item(const RtVariant* handle, const std::int32_t i) {
    if (const auto* array =
            std::get_if<wfc::detail::ArrayValue>(&value_of(handle));
        array != nullptr && i >= 0 &&
        static_cast<std::size_t>(i) < array->elements.size()) {
        return box(array->elements[static_cast<std::size_t>(i)]);
    }
    raise_error(9);
    return nullptr;
}

using EntryFunction = void (*)();

template <typename Function>
[[nodiscard]] void* address_of(Function* function) {
    return reinterpret_cast<void*>(function);
}

}  // namespace

int run_native_image(const NativeImage& image, std::string& error,
                     const wfc::detail::EngineSetup* setup) {
#ifdef _WIN32
    if (image.architecture != host_architecture()) {
        error = "the image was made for another architecture";
        return -1;
    }
    constexpr auto count = static_cast<std::size_t>(ir::Service::count);
    if (image.bytes.empty() || image.table_count != count) {
        error = "the image does not match the runtime entry table";
        return -1;
    }
    void* memory = VirtualAlloc(nullptr, image.bytes.size(),
                                MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (memory == nullptr) {
        error = "cannot allocate memory for the program";
        return -1;
    }
    std::memcpy(memory, image.bytes.data(), image.bytes.size());
    // The entry table: one pointer per service.
    void* table[count] = {
        address_of(&print_number),
        address_of(&print_boolean),
        address_of(&print_text),
        address_of(&print_newline),
        address_of(&raise_error),
        address_of(&end_program),
        address_of(&print_string),
        address_of(&string_concat),
        address_of(&string_assign),
        address_of(&string_release),
        address_of(&string_addref),
        address_of(&string_length),
        address_of(&string_compare),
        address_of(&string_left),
        address_of(&string_right),
        address_of(&string_mid),
        address_of(&string_upper),
        address_of(&string_lower),
        address_of(&string_trim),
        address_of(&string_ltrim),
        address_of(&string_rtrim),
        address_of(&string_chr),
        address_of(&string_asc),
        address_of(&string_from_number),
        address_of(&string_from_integer),
        address_of(&string_from_boolean),
        address_of(&string_instr),
        address_of(&string_space),
        address_of(&print_double),
        address_of(&string_from_double),
        address_of(&double_to_long),
        address_of(&math_function),
        address_of(&double_sign),
        address_of(&array_redim),
        address_of(&array_element1),
        address_of(&array_element2),
        address_of(&array_element3),
        address_of(&array_bound),
        address_of(&array_release),
        address_of(&array_erase),
        address_of(&error_number),
        address_of(&error_description),
        address_of(&error_source),
        address_of(&error_clear),
        address_of(&error_raise),
        address_of(&error_acknowledge),
        address_of(&print_zone),
        address_of(&double_power),
        address_of(&variant_from_long),
        address_of(&variant_from_integer),
        address_of(&variant_from_double),
        address_of(&variant_from_single),
        address_of(&variant_from_string),
        address_of(&variant_from_boolean),
        address_of(&variant_null),
        address_of(&variant_assign),
        address_of(&variant_release),
        address_of(&variant_addref),
        address_of(&variant_binary),
        address_of(&variant_negate),
        address_of(&variant_not),
        address_of(&variant_to_long),
        address_of(&variant_to_double),
        address_of(&variant_to_string),
        address_of(&variant_to_boolean),
        address_of(&variant_print),
        address_of(&variant_type_name),
        address_of(&variant_var_type),
        address_of(&variant_is_numeric),
        address_of(&object_new),
        address_of(&object_addref),
        address_of(&object_release),
        address_of(&object_assign),
        address_of(&object_check),
        address_of(&object_is),
        address_of(&object_is_null),
        address_of(&inline_element),
        address_of(&variant_call),
        address_of(&variant_missing),
        address_of(&variant_is_missing),
        address_of(&array_count),
        address_of(&array_flat_element),
        address_of(&randomize),
        address_of(&variant_from_object),
        address_of(&variant_to_object),
        address_of(&variant_array_count),
        address_of(&variant_array_item),
        address_of(&string_like),
        address_of(&object_is_class),
        address_of(&string_mid_assign),
        address_of(&variant_from_byte),
        address_of(&engine_statement),
        address_of(&host_global),
        address_of(&variant_member_get),
        address_of(&variant_member_put),
        address_of(&variant_is),
        address_of(&variant_type_of),
        address_of(&variant_items),
        address_of(&run_forms),
        address_of(&thunk_count),
        address_of(&thunk_arg),
        address_of(&thunk_store),
        address_of(&host_new),
        address_of(&array_assign),
        address_of(&events_attach),
        address_of(&events_detach),
        address_of(&raise_event),
        address_of(&string_fixed),
        address_of(&trace_call),
        address_of(&array_from_variant),
        address_of(&udt_to_engine),
        address_of(&udt_from_engine),
        address_of(&print_tab),
        address_of(&print_spc),
        address_of(&array_to_variant),
        address_of(&error_raise_help),
        address_of(&error_help_file),
        address_of(&error_help_context),
    };
    std::memcpy(memory, table, sizeof(table));
    DWORD previous = 0;
    if (VirtualProtect(memory, image.bytes.size(), PAGE_EXECUTE_READWRITE,
                       &previous) == FALSE) {
        VirtualFree(memory, 0, MEM_RELEASE);
        error = "cannot make the program executable";
        return -1;
    }
    FlushInstructionCache(GetCurrentProcess(), memory, image.bytes.size());
    g_error = ErrorState{};
    g_column = 0;
    g_image_base = static_cast<std::uint8_t*>(memory);
    g_engine.reset();
    g_class_records.clear();
    for (const auto& item : image.classes) {
        g_class_records[item.name] =
            reinterpret_cast<const ClassRecord*>(g_image_base + item.record);
    }
    if (setup != nullptr) {
        wfc::detail::NativeError problem;
        if (!engine().start(*setup, problem)) {
            error = problem.description;
            VirtualFree(memory, 0, MEM_RELEASE);
            return -1;
        }
        wfc::detail::g_native_hooks = &g_form_hooks;
    }
    // The stack limit: half of the stack that is left (the services need some
    // room too).
    {
        ULONG_PTR low = 0;
        ULONG_PTR high = 0;
        GetCurrentThreadStackLimits(&low, &high);
        const auto here = reinterpret_cast<std::uintptr_t>(&previous);
        const std::uintptr_t remaining = here > low ? here - low : 0;
        const std::uintptr_t limit = here - remaining / 2;
        std::memcpy(static_cast<std::uint8_t*>(memory) + image.error_flag + 8U,
                    &limit, sizeof(limit));
    }
    g_error.pending = reinterpret_cast<std::int32_t*>(
        static_cast<std::uint8_t*>(memory) + image.error_flag);
    const auto entry = reinterpret_cast<EntryFunction>(
        static_cast<std::uint8_t*>(memory) + image.entry);
    entry();
    finish_output();
    int code = 0;
    if (*g_error.pending != 0) {
        // An error that no handler took ends the program.
        std::cerr << "Run-time error '" << g_error.number
                  << "': " << g_error.description << '\n';
        std::cerr.flush();
        code = 1;
    }
    wfc::detail::g_native_hooks = nullptr;
    // Forms hold compiled objects: they end while the code still exists.
    g_engine.reset();
    g_engine.reset();
    g_error.pending = nullptr;
    VirtualFree(memory, 0, MEM_RELEASE);
    return code;
#else
    static_cast<void>(image);
    static_cast<void>(setup);
    error = "native programs run on Windows only";
    return -1;
#endif
}

}  // namespace wfc::compiler
