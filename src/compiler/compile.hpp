#pragma once

#include <string>
#include <string_view>

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

// Parses, checks, lowers and generates code for a standard module.
[[nodiscard]] CompileResult compile_source(std::string_view source,
                                           Architecture architecture);

}  // namespace wfc::compiler
