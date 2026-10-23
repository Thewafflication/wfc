#pragma once

// A compiled program as a file (a packaged native program, ADR-0008): the
// machine code and data of a `NativeImage` with what the loader needs.

#include <string>
#include <string_view>

#include "native_image.hpp"

namespace wfc::compiler {

[[nodiscard]] std::string serialize_image(const NativeImage& image);

// False when the text is not an image made by `serialize_image` (the same
// version of WFC).
[[nodiscard]] bool deserialize_image(std::string_view text, NativeImage& image);

}  // namespace wfc::compiler
