#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace wfc {

// A VB6 project flattened for the evaluator (REQ-0249): every standard
// module concatenated into one program, every class module separate.
struct LoadedProject final {
    bool ok{};
    std::string error;
    std::string module_source;
    std::vector<std::string> class_names;
    std::vector<std::string> class_sources;
};

// Loads a `.vbp` project (Module=/Class= entries, Startup="Sub Main"), or one
// or more `.bas`/`.cls` files given directly. Form/UserControl entries are
// reported as unsupported. File-format header lines (`VERSION`, `BEGIN`...
// `END`, `Attribute`) are blanked so line numbers are preserved.
[[nodiscard]] LoadedProject load_project(const std::vector<std::filesystem::path>& paths);

}  // namespace wfc
