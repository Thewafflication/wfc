#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "ast.hpp"
#include "native_image.hpp"

namespace wfc::compiler {

struct CompileResult final {
    NativeImage image;
    std::string error;  // empty on success
    Position error_at;
};

// Whether this machine's architecture has a back end yet.
[[nodiscard]] bool native_back_end_available();

// A class module of a project.
struct ClassSource final {
    std::string name;
    std::string source;
};

// Parses, checks, lowers and generates code for a standard module and the
// class modules of its project.
[[nodiscard]] CompileResult compile_project(
    std::string_view source, const std::vector<ClassSource>& classes,
    Architecture architecture);

// The same for a standard module alone.
[[nodiscard]] CompileResult compile_source(std::string_view source,
                                           Architecture architecture);

}  // namespace wfc::compiler
