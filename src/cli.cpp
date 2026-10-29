#include "cli.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "wfc/version.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#pragma comment(lib, "version.lib")
#endif

#ifndef WFC_OCX_VERSION_STRING
#define WFC_OCX_VERSION_STRING "unknown"
#endif

namespace fs = std::filesystem;

namespace wfc::cli {
namespace {

// An option and how many values follow it.
struct Option {
    const char* name;
    int values;
};

constexpr Option kOptions[] = {
    {"--help", 0},
    {"-h", 0},
    {"-?", 0},
    {"/?", 0},
    {"--version", 0},
    {"--verbose", 0},
    {"--gui", 0},
    {"--fullscreen", 0},
    {"--native", 0},
    {"--interpret", 0},
    {"--exit-after", 1},
    {"--ui-script", 1},
    {"--screenshots", 1},
    {"--build", 0},
    {"--check", 0},
    {"-o", 1},
    {"--link", 1},
    {"--bundle", 0},
    {"--serve", 0},
    {"--server", 0},
    {"--server-timeout", 1},
    {"--register", 0},
    {"--unregister", 0},
    {"--write-typelib", 1},
    {"--native-image", 1},
    {"--eval", 1},
    {"--class", 2},
    {"-Embedding", 0},
    {"/Embedding", 0},
};

constexpr const char* kSubcommands[] = {
    "run", "build", "check", "libs", "env", "help", "version", "completion"};

// The PowerShell completer of the commands and options (`wfc completion
// powershell | Out-String | Invoke-Expression`).
void print_powershell_completion() {
    std::cout << R"ps(Register-ArgumentCompleter -Native )ps"
                 R"ps(-CommandName wfc, wfc.exe -ScriptBlock {
    param($wordToComplete, $commandAst, $cursorPosition)
    $elements = $commandAst.CommandElements
    $previous = ''
    if ($elements.Count -gt 1) {
        $previous = $elements[$elements.Count - 1].ToString()
    }
    if ($wordToComplete -eq '' -and $previous -eq '--link') {
        $words = 'static', 'dynamic'
    } else {
        $words = @()ps";
    bool first = true;
    const auto item = [&](const std::string_view name) {
        std::cout << (first ? "" : ", ") << "'" << name << "'";
        first = false;
    };
    for (const char* const name : kSubcommands) {
        item(name);
    }
    for (const auto& option : kOptions) {
        const std::string_view name = option.name;
        if (name.rfind("--", 0) == 0) {
            item(name);
        }
    }
    std::cout << R"ps()
    }
    $words | Where-Object { $_ -like "$wordToComplete*" } | ForEach-Object {
        [System.Management.Automation.CompletionResult]::new(
            $_, $_, 'ParameterValue', $_)
    }
}
)ps";
}

#if defined(_M_ARM64)
constexpr const char* kArchitecture = "arm64";
#elif defined(_M_X64) || defined(__x86_64__)
constexpr const char* kArchitecture = "x64";
#else
constexpr const char* kArchitecture = "x86";
#endif

bool is_option(const std::string_view text) {
    return (!text.empty() && text.front() == '-') || text == "/?";
}

const Option* find_option(const std::string_view text) {
    for (const auto& option : kOptions) {
        if (text == option.name) {
            return &option;
        }
    }
    return nullptr;
}

std::size_t distance(const std::string_view a, const std::string_view b) {
    std::vector<std::size_t> row(b.size() + 1U);
    for (std::size_t j = 0; j <= b.size(); ++j) {
        row[j] = j;
    }
    for (std::size_t i = 1; i <= a.size(); ++i) {
        std::size_t diagonal = row[0];
        row[0] = i;
        for (std::size_t j = 1; j <= b.size(); ++j) {
            const std::size_t above = row[j];
            row[j] = std::min({row[j] + 1U, row[j - 1U] + 1U,
                               diagonal + (a[i - 1U] == b[j - 1U] ? 0U : 1U)});
            diagonal = above;
        }
    }
    return row[b.size()];
}

std::string environment(const char* const name) {
#ifdef _WIN32
    char buffer[4096];
    const DWORD length = GetEnvironmentVariableA(name, buffer, sizeof(buffer));
    return length > 0 && length < sizeof(buffer) ? std::string(buffer, length)
                                                 : std::string{};
#else
    const char* const value = std::getenv(name);
    return value != nullptr ? value : "";
#endif
}

bool color_enabled() {
#ifdef _WIN32
    if (!environment("NO_COLOR").empty()) {
        return false;
    }
    const HANDLE error = GetStdHandle(STD_ERROR_HANDLE);
    DWORD mode = 0;
    if (error == INVALID_HANDLE_VALUE || !GetConsoleMode(error, &mode)) {
        return false;
    }
    return SetConsoleMode(error, mode | 0x0004U) != 0;  // VT output
#else
    return false;
#endif
}

void complain(const std::string& message) {
    static const bool color = color_enabled();
    std::cerr << (color ? "\x1b[31mwfc: error:\x1b[0m " : "wfc: error: ")
              << message << '\n';
}

void print_help() {
    std::cout
        << "wfc " << wfc::version
        << " - Waughtal Form Compiler: run and build Visual Basic 6 "
           "programs\n\n"
           "usage:\n"
           "  wfc <project.vbp>                 run a project (compiled to "
           "machine code)\n"
           "  wfc <file.bas> [class.cls ...]    run modules\n"
           "  wfc run <project.vbp> [options]   the same, spelled out\n"
           "  wfc check <project.vbp>           compile only: report errors, "
           "run nothing\n"
           "  wfc build <project.vbp> -o <app.exe> [options]\n"
           "                                    write a program that runs "
           "without wfc\n"
           "  wfc libs                          the libraries and their "
           "versions\n"
           "  wfc env                           where wfc is installed and "
           "what it finds\n"
           "  wfc --eval <VB source>            run a few lines\n"
           "  wfc help [command]                this text, or help on one "
           "command\n\n"
           "run options:\n"
           "  --interpret          use the interpreter instead of compiling\n"
           "  --native             require the compiler (no fallback)\n"
           "  --gui                show forms in real windows\n"
           "  --fullscreen         scale the forms to fill the screen\n"
           "  --exit-after <ms>    close the program after the time\n"
           "  --ui-script <file>   drive the forms from a script\n"
           "  --screenshots <dir>  save a PNG of each form\n\n"
           "build options:\n"
           "  -o <file>            the program to write (required)\n"
           "  --gui                the program shows its forms in windows\n"
           "  --native             machine code; a project that does not "
           "compile is an error\n"
           "  --interpret          carry the project for the interpreter\n"
           "                       (default: machine code, or the interpreter "
           "when the\n"
           "                       project does not compile)\n"
           "  --link static       one file with the whole runtime "
           "(default)\n"
           "  --link dynamic       a small program that loads wfcrt.dll\n"
           "  --bundle             with --link dynamic: copy the runtime "
           "libraries\n"
           "                       next to the program\n\n"
           "environment:\n"
           "  WFC_HOME             installation folder (default C:\\wfc)\n"
           "  WFC_SCREEN           pretend the screen is WIDTHxHEIGHT\n"
           "  NO_COLOR             no colors in messages\n\n"
           "exit codes: 0 done, 1 the program failed, 2 usage error, 3 "
           "compile error\n"
           "see https://github.com/Thewafflication/wfc\n";
}

void print_command_help(const std::string& command) {
    if (command == "run") {
        std::cout << "wfc run <project.vbp | file.bas ...> [--interpret] "
                     "[--gui] [--fullscreen]\n"
                     "        [--exit-after <ms>] [--ui-script <file>] "
                     "[--screenshots <dir>]\n\n"
                     "Compiles the project to machine code and runs it. A "
                     "project that cannot be\ncompiled, or whose program "
                     "does not start, runs in the interpreter, which\n"
                     "reports what is wrong.\n";
    } else if (command == "check") {
        std::cout << "wfc check <project.vbp | file.bas ...>\n\n"
                     "Compiles without running. Prints the first error with "
                     "its file and line and\nexits 3, or exits 0 when the "
                     "program compiles.\n";
    } else if (command == "build") {
        std::cout << "wfc build <project.vbp> -o <app.exe> [--gui] "
                     "[--native] [--link static|dynamic]\n          "
                     "[--bundle]\n\n"
                     "Writes a program of the architecture of this wfc. "
                     "Static (default) copies\nthe whole runtime into the "
                     "program; dynamic writes a small launcher that\nloads "
                     "wfcrt.dll from beside it or from the installation, and "
                     "--bundle copies\nthe runtime libraries next to the "
                     "program.\n";
    } else if (command == "libs") {
        std::cout << "wfc libs\n\nLists wfc, the runtime and the libraries "
                     "with their versions, and whether\neach is linked into "
                     "the program (static) or loaded from a file "
                     "(dynamic).\n";
    } else if (command == "env") {
        std::cout << "wfc env\n\nShows the installation folder, the "
                     "architecture and the libraries found.\n";
    } else {
        print_help();
    }
}

#ifdef _WIN32
std::string file_version(const fs::path& file) {
    DWORD ignored = 0;
    const DWORD size = GetFileVersionInfoSizeW(file.c_str(), &ignored);
    if (size == 0U) {
        return {};
    }
    std::vector<char> data(size);
    if (!GetFileVersionInfoW(file.c_str(), 0, size, data.data())) {
        return {};
    }
    void* value = nullptr;
    UINT length = 0;
    if (!VerQueryValueW(data.data(), L"\\", &value, &length) ||
        length < sizeof(VS_FIXEDFILEINFO)) {
        return {};
    }
    const auto* info = static_cast<const VS_FIXEDFILEINFO*>(value);
    return std::to_string(HIWORD(info->dwFileVersionMS)) + "." +
           std::to_string(LOWORD(info->dwFileVersionMS)) + "." +
           std::to_string(HIWORD(info->dwFileVersionLS));
}

fs::path running_folder() {
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                            static_cast<DWORD>(buffer.size()));
    buffer.resize(length);
    return fs::path(buffer).parent_path();
}
#endif

void print_libraries(const bool header) {
    if (header) {
        std::cout << "wfc " << wfc::version << " (" << kArchitecture << ")\n";
    }
    for (const auto& library : libraries()) {
        std::string name = library.name;
        name.resize(std::max<std::size_t>(10U, name.size()), ' ');
        std::cout << "  " << name << library.version << "  " << library.linking;
        if (!library.path.empty()) {
            std::cout << "  " << library.path;
        }
        std::cout << '\n';
    }
}

void print_environment() {
    const auto root = install_root();
    const auto home = environment("WFC_HOME");
    std::cout << "version       " << wfc::version << '\n'
              << "architecture  " << kArchitecture << '\n'
              << "WFC_HOME      " << (home.empty() ? "(not set)" : home) << '\n'
              << "install root  " << (root.empty() ? "(none found)" : root)
              << '\n'
              << "libraries\n";
    print_libraries(false);
}

}  // namespace

std::string suggestion(const std::string_view option) {
    std::string best;
    std::size_t best_distance = 3U;
    for (const auto& known : kOptions) {
        const auto d = distance(option, known.name);
        if (d < best_distance) {
            best_distance = d;
            best = known.name;
        }
    }
    return best;
}

std::string install_root() {
    std::vector<fs::path> candidates;
    if (const auto home = environment("WFC_HOME"); !home.empty()) {
        candidates.emplace_back(home);  // names the installation; no other
    } else {
        // The installation this wfc.exe lies in (<root>in\wfc.exe).
#ifdef _WIN32
        const auto parent = running_folder().parent_path();
        std::error_code ignored;
        if (fs::exists(parent / "install.json", ignored)) {
            candidates.push_back(parent);
        }
#endif
        if (const auto user = environment("USERNAME"); !user.empty()) {
            candidates.push_back(fs::path("C:\\wfc\\users") / user);
        }
        candidates.emplace_back("C:\\wfc");
    }
    std::error_code error;
    for (const auto& candidate : candidates) {
        if (fs::exists(candidate / "bin" / "wfc.exe", error)) {
            return candidate.string();
        }
    }
    return {};
}

bool library_is_loaded(const char* const file) {
#ifdef _WIN32
    return GetModuleHandleA(file) != nullptr;
#else
    static_cast<void>(file);
    return false;
#endif
}

std::string find_runtime_file(const std::string_view file) {
#ifdef _WIN32
    std::error_code error;
    const auto beside = running_folder() / file;
    if (fs::exists(beside, error)) {
        return beside.string();
    }
    const auto root = install_root();
    if (root.empty()) {
        return {};
    }
    const auto in_bin = fs::path(root) / "bin" / file;
    if (fs::exists(in_bin, error)) {
        return in_bin.string();
    }
    // lib\<name>\<version>\<arch>\<file>, the highest version first.
    const auto library = fs::path(root) / "lib" / fs::path(file).stem();
    std::string best_version;
    fs::path best;
    for (const auto& entry : fs::directory_iterator(library, error)) {
        const auto candidate = entry.path() / kArchitecture / file;
        if (fs::exists(candidate, error) &&
            entry.path().filename().string() > best_version) {
            best_version = entry.path().filename().string();
            best = candidate;
        }
    }
    return best.string();
#else
    static_cast<void>(file);
    return {};
#endif
}

std::vector<LibraryInfo> libraries() {
    std::vector<LibraryInfo> result;
    result.push_back({"wfc", std::string(wfc::version), "program", ""});
#ifdef _WIN32
    const auto find = [](const char* const dll) {
        return fs::path(find_runtime_file(dll));
    };
    const struct {
        const char* name;
        const char* dll;
        const char* built_in;
    } entries[] = {
        {"wfcrt", "wfcrt.dll", nullptr},
        {"wfcocx", "wfcocx.dll", WFC_OCX_VERSION_STRING},
        {"wfcvbrun", "wfcvbrun.dll", nullptr},
    };
    for (const auto& entry : entries) {
        // A library this wfc carries inside itself is static, whether or not
        // a copy of its DLL lies around.
        const bool carried =
            entry.built_in != nullptr && !library_is_loaded(entry.dll);
        const auto file = carried ? fs::path{} : find(entry.dll);
        if (!file.empty()) {
            const auto found = file_version(file);
            result.push_back({entry.name, found.empty() ? "unknown" : found,
                              "dynamic", file.string()});
        } else if (entry.built_in != nullptr) {
            result.push_back({entry.name, entry.built_in, "static", ""});
        } else {
            result.push_back({entry.name, "-", "not found", ""});
        }
    }
#else
    result.push_back({"wfcocx", WFC_OCX_VERSION_STRING, "static", ""});
#endif
    return result;
}

std::optional<int> dispatch(std::vector<std::string>& arguments) {
    if (arguments.size() < 2U) {
        print_help();
        return 2;
    }
    const std::string first = arguments[1];
    if (first == "-h" || first == "--help" || first == "-?" || first == "/?") {
        print_help();
        return 0;
    }
    if (first == "help") {
        print_command_help(arguments.size() > 2U ? arguments[2] : "");
        return 0;
    }
    if (first == "version" || (first == "--version" && arguments.size() > 2U &&
                               arguments[2] == "--verbose")) {
        print_libraries(true);
        return 0;
    }
    if (first == "libs") {
        print_libraries(true);
        return 0;
    }
    if (first == "completion") {
        if (arguments.size() < 3U || arguments[2] != "powershell") {
            complain("usage: wfc completion powershell");
            return 2;
        }
        print_powershell_completion();
        return 0;
    }
    if (first == "env") {
        print_environment();
        return 0;
    }

    // `wfc run|check|build <input> [options]` is the long spelling; a file
    // of that name wins.
    std::error_code error;
    const bool subcommand =
        std::any_of(std::begin(kSubcommands), std::end(kSubcommands),
                    [&](const char* const name) { return first == name; });
    if (subcommand && !fs::exists(first, error)) {
        if (first == "run") {
            arguments.erase(arguments.begin() + 1);
        } else if (first == "build") {
            arguments[1] = "--build";
        } else if (first == "check") {
            arguments[1] = "--check";
        }
    }
    if (arguments.size() < 2U) {
        complain("'run' needs a project or a source file; try 'wfc help run'");
        return 2;
    }

    // Each argument is an option (known, with its values) or an input that
    // must exist.
    for (std::size_t i = 1; i < arguments.size(); ++i) {
        const std::string& argument = arguments[i];
        if (!is_option(argument)) {
            if (arguments[1] != "--eval" && !fs::exists(argument, error)) {
                complain("cannot open '" + argument + "': no such file");
                return 2;
            }
            continue;
        }
        const Option* const option = find_option(argument);
        if (option == nullptr) {
            std::string message = "unknown option '" + argument + "'";
            if (const auto closest = suggestion(argument); !closest.empty()) {
                message += " (did you mean '" + closest + "'?)";
            }
            complain(message + "; try 'wfc --help'");
            return 2;
        }
        if (option->values > 0 &&
            i + static_cast<std::size_t>(option->values) >= arguments.size()) {
            complain("option '" + argument + "' needs a value");
            return 2;
        }
        if (argument == "--link" && arguments[i + 1U] != "static" &&
            arguments[i + 1U] != "dynamic") {
            complain("--link takes 'static' or 'dynamic', not '" +
                     arguments[i + 1U] + "'");
            return 2;
        }
        i += static_cast<std::size_t>(option->values);
    }
    return std::nullopt;
}

}  // namespace wfc::cli
