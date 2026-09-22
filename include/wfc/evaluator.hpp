#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace wfc {

struct Evaluation final {
    bool success{};
    std::string output;
    std::string diagnostic;
    std::size_t error_offset{};
};

// One named class module source, supplied alongside the standard module
// source, mirroring a real VB6 project's separate .cls files. `name` is the
// class's name (matched case-insensitively against `New <name>`/`As <name>`
// in the module source); `source` is that class's own source text.
struct ClassModuleSource final {
    std::string name;
    std::string_view source;
};

[[nodiscard]] Evaluation evaluate_program(std::string_view source);

[[nodiscard]] Evaluation evaluate_program(
    std::string_view source, const std::vector<ClassModuleSource>& classes);

[[nodiscard]] Evaluation evaluate_print_statement(std::string_view source);

}  // namespace wfc
