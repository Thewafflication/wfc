#pragma once

#include <cstddef>
#include <map>
#include <optional>
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
    // For a form module (.frm): the file's text (the form description and
    // code) and the directory its .frx resources are read from. `source`
    // then holds the code part with the description blanked.
    std::string_view form_description;
    std::string form_directory;
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
    // The form shown when the program starts (`Startup="Form1"`), and the
    // path of a scripted-input file for the headless UI host.
    std::string startup_form;
    std::string ui_script;
    // With the Win32 host: end the run (closing the forms) after this many
    // milliseconds; 0 runs until the user closes the last form.
    long exit_after_ms{0};
    // "win32" runs forms in real windows; anything else (the default) uses the
    // headless host.
    std::string ui_mode;
    // With the Win32 host: full-screen mode (see Win32Host::set_fit). The
    // reference size is the largest form in twips; `fit_screen_size` is
    // "WIDTHxHEIGHT" to pretend the screen has that size, empty for the real
    // one.
    bool fit_screen{false};
    double fit_width{0.0};
    double fit_height{0.0};
    std::string fit_screen_size;
    // With the Win32 host: a folder that receives a PNG of each form when it
    // is shown and just before it is unloaded.
    std::string screenshot_directory;
    // COM server mode (ADR-0004): "inproc" registers the project's exposed
    // classes as in-process class objects for this run; "server" also serves
    // other processes until the last object is released or
    // `server_timeout_ms` passes (0: no limit). `project_name` is the
    // project's `Name`, the first part of every ProgID.
    std::string serve;
    std::string project_name;
    long server_timeout_ms{600000};
    // With `serve` == "typelib": where to write the type library of the exposed
    // classes (the program is not run); otherwise an optional location for the
    // type library a server publishes.
    std::string typelib_path;
};

// Conditional compilation (`#If`, `#Const`): the source with the directives
// and the inactive branches blanked, line breaks kept. Empty on a malformed
// directive, with `error` and `error_offset` set.
[[nodiscard]] std::optional<std::string> preprocess_conditionals(
    std::string_view source, std::string& error, std::size_t& error_offset);

// `[bracketed names]` and `As IUnknown`: the source in the plain form the
// interpreter and the compiler read.
void rewrite_source_forms(std::string& text);

[[nodiscard]] Evaluation evaluate_program(std::string_view source);

[[nodiscard]] Evaluation evaluate_program(
    std::string_view source, const std::vector<ClassModuleSource>& classes);

[[nodiscard]] Evaluation evaluate_program(
    std::string_view source, const std::vector<ClassModuleSource>& classes,
    const EvaluationOptions& options);

[[nodiscard]] Evaluation evaluate_print_statement(std::string_view source);

}  // namespace wfc
