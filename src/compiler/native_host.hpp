#pragma once

// The loader and the runtime services of native code (ADR-0008): maps a
// generated image executable, fills its entry table with the runtime's
// functions and runs it.

#include <string>

#include "native_image.hpp"

namespace wfc::detail {
struct EngineSetup;
}

namespace wfc::compiler {

// Runs the image on this machine and returns the program's exit code. `error`
// is set (and -1 returned) when the image cannot be loaded. `setup` is what
// the engine needs for the forms of the project (null for none).
[[nodiscard]] int run_native_image(
    const NativeImage& image, std::string& error,
    const wfc::detail::EngineSetup* setup = nullptr);

}  // namespace wfc::compiler
