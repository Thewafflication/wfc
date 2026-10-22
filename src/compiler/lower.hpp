#pragma once

#include <string>
#include <vector>

#include "ast.hpp"
#include "ir.hpp"

namespace wfc::compiler {

struct LowerResult final {
    ir::Program program;
    std::string error;  // empty on success
    Position error_at;
};

// Resolves names and types of a parsed module and lowers it to IR. Constructs
// the compiler does not support yet are reported by name.
[[nodiscard]] LowerResult lower_module(
    const Module& module, const std::vector<const Module*>& classes = {});

}  // namespace wfc::compiler
