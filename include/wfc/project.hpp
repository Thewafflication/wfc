#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <utility>
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
    // Where each standard module starts inside `module_source` (1-based line),
    // so a line number in the concatenated program maps back to a file; and
    // each class module's file.
    struct ModuleSpan final {
        std::string file;
        std::size_t first_line{};
    };
    std::vector<ModuleSpan> module_spans;
    std::vector<std::string> class_files;
    // `App` object metadata from the .vbp (Title, ExeName32, MajorVer, ...),
    // keyed by lower-case `App` property name.
    std::map<std::string, std::string> app_properties;
    // Byte ranges of `module_source` belonging to modules that declare
    // `Option Explicit` (REQ-0283).
    std::vector<std::pair<std::size_t, std::size_t>> option_explicit_ranges;
    // True when the ranges above are authoritative (a linked multi-module
    // project); false for a single module, which carries its own Option line.
    bool per_module_option_explicit{};
};

// Loads a `.vbp` project (Module=/Class= entries, Startup="Sub Main"), or one
// or more `.bas`/`.cls` files given directly. Form/UserControl entries are
// reported as unsupported. File-format header lines (`VERSION`, `BEGIN`...
// `END`, `Attribute`) are blanked so line numbers are preserved.
[[nodiscard]] LoadedProject load_project(
    const std::vector<std::filesystem::path>& paths);

}  // namespace wfc
