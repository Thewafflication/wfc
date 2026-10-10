// Links the standard modules of a .vbp into one program: each module keeps
// its own namespace by renaming colliding module-level names, and the
// per-module Option statements are merged. Internal to the WFC project loader.

#ifndef WFC_MODULE_LINKER_HPP
#define WFC_MODULE_LINKER_HPP

#include <string>
#include <vector>

namespace wfc::detail {

// Rewrites `modules` (and the `Module.Name` references in `classes`) in
// place, keeping every line where it was, and returns the Option statements
// that should precede the concatenated modules (one per line, possibly
// empty). `names` holds each module's name, parallel to `modules`.
[[nodiscard]] std::string link_modules(std::vector<std::string>& modules,
                                       const std::vector<std::string>& names,
                                       std::vector<std::string>& classes);

}  // namespace wfc::detail

#endif
