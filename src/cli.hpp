#pragma once

// The command line of wfc: help, version and library listings, the
// subcommands (`run`, `build`, `check`, `libs`, `env`, `help`) and the checks
// that give a clear message for an unknown option or a missing file.

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace wfc::cli {

// Handles the commands that need no project (help, version, libs, env) and
// the mistakes in the command line. Returns the exit code when it handled the
// command line; otherwise `arguments` holds the command line to run, with a
// subcommand rewritten into the options the evaluator understands.
[[nodiscard]] std::optional<int> dispatch(std::vector<std::string>& arguments);

// The option closest to `option` among the known ones, or empty.
[[nodiscard]] std::string suggestion(std::string_view option);

// The libraries of this installation: name, version, how it is linked
// ("static" inside the program, "dynamic" with the file found) and the path.
struct LibraryInfo {
    std::string name;
    std::string version;
    std::string linking;
    std::string path;
};
[[nodiscard]] std::vector<LibraryInfo> libraries();

// A file of the runtime (wfcl.exe, wfcrt.dll, ...): beside the running wfc, in
// `bin` of the installation, or in the highest version of its library folder
// (`lib\<name>\<version>\<arch>`) of the installation. Empty when not found.
[[nodiscard]] std::string find_runtime_file(std::string_view file);

// The folder wfc is installed in (WFC_HOME, C:\wfc\users\<name> or C:\wfc), or
// empty when none of them holds an installation.
[[nodiscard]] std::string install_root();

}  // namespace wfc::cli
