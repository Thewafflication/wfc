// The launcher of a program linked dynamically to the WFC runtime: a small
// executable that finds wfcrt.dll (the runtime library carrying the version
// it was built for), checks the version and hands the command line over.
//
// The runtime is looked for in this order: beside the program, in
// `%WFC_HOME%\bin`, in `%WFC_HOME%\lib\wfcrt\<version>\<arch>`, and in the
// installation roots (`C:\wfc` for all users, `C:\wfc\users\<name>` for one).
// The library folders below `lib` of that root are added to the DLL search path
// so wfcrt.dll finds the libraries it uses (wfcocx.dll, wfcvbrun.dll).

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "wfc/version.hpp"

namespace fs = std::filesystem;

namespace {

#if defined(_M_ARM64)
constexpr const wchar_t* kArchitecture = L"arm64";
#elif defined(_M_X64)
constexpr const wchar_t* kArchitecture = L"x64";
#else
constexpr const wchar_t* kArchitecture = L"x86";
#endif

using EntryFunction = int(__cdecl*)(int, const char* const*);
using VersionFunction = const char*(__cdecl*)();

std::wstring environment(const wchar_t* const name) {
    wchar_t buffer[2048];
    const DWORD length = GetEnvironmentVariableW(name, buffer, 2048);
    return length > 0 && length < 2048 ? std::wstring(buffer, length)
                                       : std::wstring{};
}

fs::path executable_path() {
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                            static_cast<DWORD>(buffer.size()));
    buffer.resize(length);
    return fs::path(buffer);
}

std::string narrow(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(),
                                         static_cast<int>(text.size()), nullptr,
                                         0, nullptr, nullptr);
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        result.data(), size, nullptr, nullptr);
    return result;
}

// The installation roots, most specific first.
std::vector<fs::path> roots() {
    std::vector<fs::path> result;
    if (const auto home = environment(L"WFC_HOME"); !home.empty()) {
        // WFC_HOME names the installation to use; no other is searched.
        result.emplace_back(home);
        return result;
    }
    const auto user = environment(L"USERNAME");
    if (!user.empty()) {
        result.push_back(fs::path(L"C:\\wfc\\users") / user);
    }
    result.emplace_back(L"C:\\wfc");
    return result;
}

// The folder of the highest version of the library `name` for this
// architecture below `<root>\lib\<name>`, or empty.
fs::path library_folder(const fs::path& root, const std::wstring& name) {
    std::error_code error;
    fs::path best;
    std::wstring best_version;
    for (const auto& entry :
         fs::directory_iterator(root / L"lib" / name, error)) {
        const auto candidate = entry.path() / kArchitecture;
        if (entry.is_directory(error) && fs::is_directory(candidate, error) &&
            entry.path().filename().wstring() > best_version) {
            best_version = entry.path().filename().wstring();
            best = candidate;
        }
    }
    return best;
}

// Makes the library folders of a root known to the DLL search path.
void add_library_folders(const fs::path& root) {
    std::error_code error;
    for (const auto& entry : fs::directory_iterator(root / L"lib", error)) {
        if (!entry.is_directory(error)) {
            continue;
        }
        const auto folder =
            library_folder(root, entry.path().filename().wstring());
        if (!folder.empty()) {
            AddDllDirectory(folder.c_str());
        }
    }
}

HMODULE load_runtime(fs::path& found) {
    std::vector<fs::path> candidates;
    candidates.push_back(executable_path().parent_path() / L"wfcrt.dll");
    for (const auto& root : roots()) {
        candidates.push_back(root / L"bin" / L"wfcrt.dll");
        const auto folder = library_folder(root, L"wfcrt");
        if (!folder.empty()) {
            candidates.push_back(folder / L"wfcrt.dll");
        }
    }
    std::error_code error;
    for (const auto& root : roots()) {
        add_library_folders(root);
    }
    for (const auto& candidate : candidates) {
        if (!fs::exists(candidate, error)) {
            continue;
        }
        if (const HMODULE module =
                LoadLibraryExW(candidate.c_str(), nullptr,
                               LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
                                   LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
                                   LOAD_LIBRARY_SEARCH_USER_DIRS)) {
            found = candidate;
            return module;
        }
    }
    return nullptr;
}

// "major.minor" of a version.
std::string series(const std::string& version) {
    const auto first = version.find('.');
    const auto second =
        first == std::string::npos ? first : version.find('.', first + 1U);
    return version.substr(0, second);
}

}  // namespace

int main(const int argument_count, const char* const arguments[]) {
    fs::path found;
    const HMODULE runtime = load_runtime(found);
    const std::string expected(wfc::version);
    if (runtime == nullptr) {
        std::fprintf(stderr,
                     "%s needs the WFC runtime library wfcrt.dll %s, which was "
                     "not found beside the program, in WFC_HOME or in C:\\wfc. "
                     "Install WFC or copy wfcrt.dll next to the program.\n",
                     narrow(executable_path().filename().wstring()).c_str(),
                     expected.c_str());
        return 9009;
    }
    const auto version = reinterpret_cast<VersionFunction>(
        GetProcAddress(runtime, "wfc_runtime_version"));
    const auto entry =
        reinterpret_cast<EntryFunction>(GetProcAddress(runtime, "wfc_entry"));
    if (version == nullptr || entry == nullptr ||
        series(version()) != series(expected)) {
        std::fprintf(stderr,
                     "%s was built for the WFC runtime %s, but %s is version "
                     "%s.\n",
                     narrow(executable_path().filename().wstring()).c_str(),
                     expected.c_str(), narrow(found.wstring()).c_str(),
                     version != nullptr ? version() : "unknown");
        return 9009;
    }
    return entry(argument_count, arguments);
}
