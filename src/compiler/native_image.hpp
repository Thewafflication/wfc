#pragma once

// The product of a back end: position-independent machine code with its data,
// and where the loader finds the entry table (ADR-0008).
//
//   offset 0                  entry table, `table_count` pointers filled by the
//                             loader (one per runtime service)
//   `globals`                 module-level variables, zero-initialized
//   `strings`                 string literals of Print
//   `code`                    the generated functions
//
// Every reference inside the image is relative to the instruction pointer, so
// the loader only fills the entry table.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "ir.hpp"

namespace wfc::compiler {

enum class Architecture { x86, x64, arm64 };

// A class of the program: its lower-case name and where its record lies.
struct ImageClass final {
    std::string name;
    std::size_t record{};
};

struct NativeImage final {
    Architecture architecture{Architecture::x64};
    std::vector<ImageClass> classes;
    std::vector<std::uint8_t> bytes;
    std::size_t table_count{};
    std::size_t entry{};  // offset of the function that starts the program
    // Offset of the 32-bit pending-error flag. The pointer-sized stack limit
    // follows it eight bytes later: the lowest stack address a call may
    // reach, set by the loader (error 28 below it).
    std::size_t error_flag{};
};

// Version of the entry table layout (the order of `ir::Service`). The loader
// refuses an image made for another version.
inline constexpr int kRuntimeApiVersion = 2;

struct BackEndResult final {
    NativeImage image;
    std::string error;  // empty on success
};

// The architecture this build of WFC runs on.
[[nodiscard]] constexpr Architecture host_architecture() {
#if defined(_M_ARM64) || defined(__aarch64__)
    return Architecture::arm64;
#elif defined(_M_X64) || defined(__x86_64__)
    return Architecture::x64;
#else
    return Architecture::x86;
#endif
}

// Writes the image offset of each class's Terminate function into its record
// (a literal of the image; `string_offsets` says where each literal lies).
inline std::vector<ImageClass> patch_class_records(
    std::vector<std::uint8_t>& image, const ir::Program& program,
    const std::vector<std::size_t>& string_offsets,
    const std::vector<std::size_t>& function_starts) {
    std::vector<ImageClass> table;
    for (const auto& item : program.classes) {
        const auto base = string_offsets[static_cast<std::size_t>(item.record)];
        table.push_back(ImageClass{item.name, base + 8U});
        if (item.terminate >= 0) {
            const auto offset = static_cast<std::int32_t>(
                function_starts[static_cast<std::size_t>(item.terminate)]);
            std::memcpy(image.data() + base + 8U + 4U, &offset, sizeof(offset));
        }
        for (const auto& member : item.members) {
            if (member.function < 0) {
                continue;  // a marker ("implements:name"): no code
            }
            const auto offset = static_cast<std::int32_t>(
                function_starts[static_cast<std::size_t>(member.function)]);
            std::memcpy(image.data() + base + 8U + member.patch_at, &offset,
                        sizeof(offset));
        }
    }
    return table;
}

// Generates machine code for the program. `error` says why a back end cannot.
[[nodiscard]] BackEndResult generate_code(const ir::Program& program,
                                          Architecture architecture);

// The AMD64 back end (x64.cpp).
[[nodiscard]] BackEndResult generate_x64(const ir::Program& program);

// The x86 back end (x86.cpp).
[[nodiscard]] BackEndResult generate_x86(const ir::Program& program);

// The ARM64 back end (arm64.cpp).
[[nodiscard]] BackEndResult generate_arm64(const ir::Program& program);

}  // namespace wfc::compiler
