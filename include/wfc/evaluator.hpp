#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace wfc {

struct Evaluation final {
    bool success{};
    std::string output;
    std::string diagnostic;
    std::size_t error_offset{};
};

[[nodiscard]] Evaluation evaluate_program(std::string_view source);

[[nodiscard]] Evaluation evaluate_print_statement(std::string_view source);

}  // namespace wfc
