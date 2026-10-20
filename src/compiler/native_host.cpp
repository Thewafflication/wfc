#include "native_host.hpp"

#include <algorithm>
#include <cstdint>
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

void print_number(const std::int32_t value) {
    // VB6 prints a number with a sign position and a trailing space.
    std::cout << (value >= 0 ? " " : "") << value << ' ';
}

void print_boolean(const std::int32_t value) {
    std::cout << (value != 0 ? "True" : "False");
}

void print_text(const char* text, const std::int32_t length) {
    std::cout.write(text, length);
}

void print_newline() {
    std::cout << '\n';
}

[[nodiscard]] std::string_view error_text(const std::int32_t number) {
    switch (number) {
        case 5:
            return "Invalid procedure call or argument";
        case 6:
            return "Overflow";
        case 11:
            return "Division by zero";
        default:
            return "Application-defined or object-defined error";
    }
}

[[noreturn]] void raise_error(const std::int32_t number) {
    std::cout.flush();
    std::cerr << "Run-time error '" << number << "': " << error_text(number)
              << '\n';
    std::cerr.flush();
    std::exit(1);
}

[[noreturn]] void end_program() {
    std::cout.flush();
    std::exit(0);
}

void print_string(const RtString* text) {
    const auto content = view_of(text);
    std::cout.write(content.data(),
                    static_cast<std::streamsize>(content.size()));
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
    }
    if (count >= length_of(text)) {
        return retain(text);
    }
    return make_string(text->data, static_cast<std::size_t>(count));
}

RtString* string_right(RtString* text, const std::int32_t count) {
    if (count < 0) {
        raise_error(5);
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
    }
    const char c = static_cast<char>(code);
    return make_string(&c, 1U);
}

std::int32_t string_asc(const RtString* text) {
    if (length_of(text) == 0) {
        raise_error(5);
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
    }
    return make_string(std::string(static_cast<std::size_t>(count), ' '));
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
    const auto entry = reinterpret_cast<EntryFunction>(
        static_cast<std::uint8_t*>(memory) + image.entry);
    entry();
    std::cout.flush();
    VirtualFree(memory, 0, MEM_RELEASE);
    return 0;
#else
    static_cast<void>(image);
    error = "native programs run on Windows only";
    return -1;
#endif
}

}  // namespace wfc::compiler
