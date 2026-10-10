#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace wfc {

struct Evaluation final {
    bool success{};
    std::string output;
    std::string diagnostic;
    std::size_t error_offset{};
    // On failure, `output` stays empty; the text printed before the error is
    // kept here (the command line shows it ahead of the diagnostic).
    std::string partial_output;
    // Text written by `Debug.Print` (the Immediate window), success or not.
    std::string debug_output;
    // When the failure is a VB run-time error: its number (0 otherwise) and
    // text.
    long vb_error_number{};
    std::string vb_error_description;
    // Where a failure happened: 1-based line and column within the module that
    // failed
    // (`error_module` is empty for the standard module(s), else the class
    // module's name). Zero when the position is unknown.
    std::size_t error_line{};
    std::size_t error_column{};
    std::string error_module;
};

// One named class module source, supplied alongside the standard module
// source, mirroring a real VB6 project's separate .cls files. `name` is the
// class's name (matched case-insensitively against `New <name>`/`As <name>`
// in the module source); `source` is that class's own source text.
struct ClassModuleSource final {
    std::string name;
    std::string_view source;
};

// Behaviors that differ between a compact one-liner snippet and a faithful VB6
// program run.
struct EvaluationOptions final {
    // `Print` reserves a sign position before a number and adds a trailing
    // space, as VB6 does
    // (` 5 `, `-5 `). Off by default: snippets print numbers compactly.
    bool vb6_print_spacing{};
    // Project metadata surfaced through the `App` object, keyed by lower-case
    // property name (`title`, `exename`, `major`, `minor`, `revision`,
    // `companyname`, `productname`, `filedescription`, `comments`,
    // `legalcopyright`). Unset properties keep their defaults.
    std::map<std::string, std::string> app_properties;
    // A multi-module project: `Option Explicit` applies only inside the byte
    // ranges of `source` listed here (the modules that declare it), instead
    // of to the whole text. Class modules always use their own text.
    bool per_module_option_explicit{};
    std::vector<std::pair<std::size_t, std::size_t>> option_explicit_ranges;
    // Path of a compiled `.res` file that `LoadResString` and `LoadResData`
    // read; empty when the project has none.
    std::string resource_file;
    // Type libraries (`{GUID}#major.minor#path`) whose classes, interfaces
    // and enumeration members the program may name (early binding).
    std::vector<std::string> type_libraries;
};

[[nodiscard]] Evaluation evaluate_program(std::string_view source);

[[nodiscard]] Evaluation evaluate_program(
    std::string_view source, const std::vector<ClassModuleSource>& classes);

[[nodiscard]] Evaluation evaluate_program(
    std::string_view source, const std::vector<ClassModuleSource>& classes,
    const EvaluationOptions& options);

[[nodiscard]] Evaluation evaluate_print_statement(std::string_view source);

}  // namespace wfc
