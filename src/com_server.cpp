// COM server support: exposed classes, class ids and per-user registration.
// Internal to the WFC runtime; not part of the public API.

#include "com_server.hpp"

#include <cctype>
#include <cstdint>
#include <cstdio>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace wfc {
namespace {

[[nodiscard]] std::string lower(const std::string& text) {
    std::string result;
    for (const char c : text) {
        result.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
}

// Whether the class text has `Attribute <name> = True` (any spacing).
[[nodiscard]] bool has_true_attribute(const std::string& source,
                                      const std::string& name) {
    std::size_t position = 0;
    while (position < source.size()) {
        auto end = source.find('\n', position);
        if (end == std::string::npos) {
            end = source.size();
        }
        std::string line;
        for (const char c : source.substr(position, end - position)) {
            if (c != ' ' && c != '\t' && c != '\r') {
                line.push_back(static_cast<char>(
                    std::tolower(static_cast<unsigned char>(c))));
            }
        }
        position = end + 1U;
        if (line == "attribute" + lower(name) + "=true") {
            return true;
        }
    }
    return false;
}

}  // namespace

std::string clsid_for_progid(const std::string& progid) {
    // Two 64-bit FNV-1a hashes of the lower-cased ProgID make the 128 bits.
    const std::string key = "wfc:" + lower(progid);
    std::uint64_t high = 1469598103934665603ULL;
    std::uint64_t low = 1099511628211ULL ^ 0x9E3779B97F4A7C15ULL;
    for (const char c : key) {
        high = (high ^ static_cast<unsigned char>(c)) * 1099511628211ULL;
        low = (low ^ static_cast<unsigned char>(c)) * 14029467366897019727ULL;
    }
    // Version 5 style: version nibble 5, variant bits 10.
    high = (high & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000005000ULL;
    low = (low & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
    char text[64];
    std::snprintf(text, sizeof(text), "{%08X-%04X-%04X-%04X-%012llX}",
                  static_cast<unsigned>(high >> 32),
                  static_cast<unsigned>((high >> 16) & 0xFFFFU),
                  static_cast<unsigned>(high & 0xFFFFU),
                  static_cast<unsigned>(low >> 48),
                  static_cast<unsigned long long>(low & 0xFFFFFFFFFFFFULL));
    return text;
}

std::vector<ServedClass> served_classes(
    const std::string& project_name,
    const std::vector<ClassModuleSource>& classes) {
    std::vector<ServedClass> served;
    for (const auto& module : classes) {
        if (!module.form_description.empty()) {
            continue;  // forms and controls are not creatable servers
        }
        const std::string source(module.source);
        if (has_true_attribute(source, "VB_Exposed") &&
            has_true_attribute(source, "VB_Creatable")) {
            ServedClass entry;
            entry.class_name = module.name;
            entry.progid = (project_name.empty() ? "Project1" : project_name) +
                           "." + module.name;
            entry.clsid = clsid_for_progid(entry.progid);
            served.push_back(std::move(entry));
        }
    }
    return served;
}

#ifdef _WIN32

namespace {

[[nodiscard]] std::wstring widen(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int length = MultiByteToWideChar(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        result.data(), length);
    return result;
}

[[nodiscard]] bool set_default(const std::wstring& key,
                               const std::wstring& value) {
    HKEY handle = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, key.c_str(), 0, nullptr, 0,
                        KEY_WRITE, nullptr, &handle,
                        nullptr) != ERROR_SUCCESS) {
        return false;
    }
    const LONG status = RegSetValueExW(
        handle, nullptr, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(value.c_str()),
        static_cast<DWORD>((value.size() + 1U) * sizeof(wchar_t)));
    RegCloseKey(handle);
    return status == ERROR_SUCCESS;
}

}  // namespace

bool register_server(const std::vector<ServedClass>& classes,
                     const std::string& executable,
                     const std::string& project_path, std::string& error) {
    const std::wstring command = L"\"" + widen(executable) + L"\" \"" +
                                 widen(project_path) + L"\" --server";
    for (const auto& entry : classes) {
        const std::wstring progid = widen(entry.progid);
        const std::wstring clsid = widen(entry.clsid);
        const std::wstring base = L"Software\\Classes\\";
        if (!set_default(base + progid, widen(entry.class_name)) ||
            !set_default(base + progid + L"\\CLSID", clsid) ||
            !set_default(base + L"CLSID\\" + clsid, widen(entry.class_name)) ||
            !set_default(base + L"CLSID\\" + clsid + L"\\ProgID", progid) ||
            !set_default(base + L"CLSID\\" + clsid + L"\\LocalServer32",
                         command)) {
            error = "cannot write the registration of " + entry.progid;
            return false;
        }
    }
    return true;
}

bool unregister_server(const std::vector<ServedClass>& classes,
                       std::string& error) {
    for (const auto& entry : classes) {
        const std::wstring base = L"Software\\Classes\\";
        for (const std::wstring& key :
             {base + widen(entry.progid),
              base + L"CLSID\\" + widen(entry.clsid)}) {
            const LONG status = RegDeleteTreeW(HKEY_CURRENT_USER, key.c_str());
            if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND) {
                error = "cannot remove the registration of " + entry.progid;
                return false;
            }
        }
    }
    return true;
}

#else

bool register_server(const std::vector<ServedClass>&, const std::string&,
                     const std::string&, std::string& error) {
    error = "COM registration needs Windows";
    return false;
}

bool unregister_server(const std::vector<ServedClass>&, std::string& error) {
    error = "COM registration needs Windows";
    return false;
}

#endif

}  // namespace wfc
