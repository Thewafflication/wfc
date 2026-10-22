#pragma once

#include <set>
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
struct LowerOptions final {
    // Names of objects of the engine that code reaches by name (the forms).
    std::set<std::string> host_globals;
    bool has_forms{};  // the program ends by running the forms
    std::string startup_form;
};

[[nodiscard]] LowerResult lower_module(
    const Module& module, const std::vector<const Module*>& classes = {},
    const LowerOptions& options = {});

}  // namespace wfc::compiler
