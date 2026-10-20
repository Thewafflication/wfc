#include "native_host.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#endif

namespace wfc::compiler {

namespace {

#ifdef _WIN32

// The runtime services, in the order of `ir::Service`.

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
        case 6:
            return "Overflow";
        case 11:
            return "Division by zero";
        default:
            return "Application-defined or object-defined error";
    }
}

void raise_error(const std::int32_t number) {
    std::cout.flush();
    std::cerr << "Run-time error '" << number << "': " << error_text(number)
              << '\n';
    std::cerr.flush();
    std::exit(1);
}

void end_program() {
    std::cout.flush();
    std::exit(0);
}

using EntryFunction = void (*)();

#endif

}  // namespace

int run_native_image(const NativeImage& image, std::string& error) {
#ifdef _WIN32
    if (image.architecture != host_architecture()) {
        error = "the image was made for another architecture";
        return -1;
    }
    if (image.bytes.empty() ||
        image.table_count != static_cast<std::size_t>(ir::Service::count)) {
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
    void* table[static_cast<std::size_t>(ir::Service::count)] = {
        reinterpret_cast<void*>(&print_number),
        reinterpret_cast<void*>(&print_boolean),
        reinterpret_cast<void*>(&print_text),
        reinterpret_cast<void*>(&print_newline),
        reinterpret_cast<void*>(&raise_error),
        reinterpret_cast<void*>(&end_program),
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
