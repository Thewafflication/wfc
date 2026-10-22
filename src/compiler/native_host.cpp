#include "native_host.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

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

void put(const char* text, const std::size_t length) {
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
};

ErrorState g_error;

// Records a run-time error and marks it pending. The caller returns at once;
// generated code sees the flag and goes to the procedure's handler.
void raise_error(const std::int32_t number) {
    g_error.number = number;
    g_error.description = standard_error_text(number);
    g_error.source = "Project1";
    if (g_error.pending != nullptr) {
        *g_error.pending = 1;
    }
}

[[noreturn]] void end_program() {
    std::cout.flush();
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

std::int32_t string_length(const RtString* text) {
    return length_of(text);
}

std::int32_t string_compare(const RtString* a, const RtString* b) {
    const auto left = view_of(a);
    const auto right = view_of(b);
    const int order = left.compare(right);
    return order < 0 ? -1 : (order > 0 ? 1 : 0);
}

RtString* string_left(RtString* text, const std::int32_t count) {
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
    std::string result(view_of(text));
    std::transform(result.begin(), result.end(), result.begin(), to_upper);
    return make_string(result);
}

RtString* string_lower(const RtString* text) {
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
    }
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
    header.strings = kind == 3 ? record_strings : nullptr;
    std::int64_t count = 1;
    for (std::int32_t k = 0; k < dims; ++k) {
        const auto lower = static_cast<std::int32_t>(bounds[2 * k]);
        const auto upper = static_cast<std::int32_t>(bounds[2 * k + 1]);
        if (upper < lower) {
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
                if (kind == 2) {
                    std::memset(from, 0, size);  // the string moved
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

// A handler took over: the error is no longer pending.
void error_acknowledge() {
    if (g_error.pending != nullptr) {
        *g_error.pending = 0;
    }
}

using EntryFunction = void (*)();

template <typename Function>
[[nodiscard]] void* address_of(Function* function) {
    return reinterpret_cast<void*>(function);
}

}  // namespace

int run_native_image(const NativeImage& image, std::string& error) {
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
        address_of(&print_number),        address_of(&print_boolean),
        address_of(&print_text),          address_of(&print_newline),
        address_of(&raise_error),         address_of(&end_program),
        address_of(&print_string),        address_of(&string_concat),
        address_of(&string_assign),       address_of(&string_release),
        address_of(&string_addref),       address_of(&string_length),
        address_of(&string_compare),      address_of(&string_left),
        address_of(&string_right),        address_of(&string_mid),
        address_of(&string_upper),        address_of(&string_lower),
        address_of(&string_trim),         address_of(&string_ltrim),
        address_of(&string_rtrim),        address_of(&string_chr),
        address_of(&string_asc),          address_of(&string_from_number),
        address_of(&string_from_integer), address_of(&string_from_boolean),
        address_of(&string_instr),        address_of(&string_space),
        address_of(&print_double),        address_of(&string_from_double),
        address_of(&double_to_long),      address_of(&math_function),
        address_of(&double_sign),         address_of(&array_redim),
        address_of(&array_element1),      address_of(&array_element2),
        address_of(&array_element3),      address_of(&array_bound),
        address_of(&array_release),       address_of(&array_erase),
        address_of(&error_number),        address_of(&error_description),
        address_of(&error_source),        address_of(&error_clear),
        address_of(&error_raise),         address_of(&error_acknowledge),
        address_of(&print_zone),          address_of(&double_power),
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
    g_error.pending = reinterpret_cast<std::int32_t*>(
        static_cast<std::uint8_t*>(memory) + image.error_flag);
    const auto entry = reinterpret_cast<EntryFunction>(
        static_cast<std::uint8_t*>(memory) + image.entry);
    entry();
    std::cout.flush();
    int code = 0;
    if (*g_error.pending != 0) {
        // An error that no handler took ends the program.
        std::cerr << "Run-time error '" << g_error.number
                  << "': " << g_error.description << '\n';
        std::cerr.flush();
        code = 1;
    }
    g_error.pending = nullptr;
    VirtualFree(memory, 0, MEM_RELEASE);
    return code;
#else
    static_cast<void>(image);
    error = "native programs run on Windows only";
    return -1;
#endif
}

}  // namespace wfc::compiler
