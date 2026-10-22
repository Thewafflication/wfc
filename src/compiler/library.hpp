#pragma once

#include <string_view>

namespace wfc::compiler {

// Whether the lower-case name is a function of the VBA library that native
// programs reach through the shared runtime (Format, IIf, Now, ...).
[[nodiscard]] bool is_library_function(std::string_view name);

}  // namespace wfc::compiler
