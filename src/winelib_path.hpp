// The Unix path of a Windows path (Winelib builds). A Win32 call returned a
// Windows path; a Winelib program is a Linux program, so std::filesystem
// wants a Unix path, and its wide conversions do not work with the 16-bit
// wchar_t of Wine's headers.
#pragma once

#ifdef __WINE__
#include <cstdlib>
#include <filesystem>
#include <string>

#include <windows.h>

inline std::filesystem::path wine_unix_path(const WCHAR* dos) {
    std::string text;  // UTF-16 to UTF-8, by hand
    for (const WCHAR* unit = dos; *unit != 0; ++unit) {
        unsigned long code = *unit;
        if (code >= 0xD800UL && code < 0xDC00UL && unit[1] >= 0xDC00U &&
            unit[1] < 0xE000U) {
            code = 0x10000UL + ((code - 0xD800UL) << 10U) +
                   (static_cast<unsigned long>(*++unit) - 0xDC00UL);
        }
        if (code < 0x80UL) {
            text += static_cast<char>(code == 92UL ? 47UL : code);
        } else if (code < 0x800UL) {
            text += static_cast<char>(0xC0UL | (code >> 6U));
            text += static_cast<char>(0x80UL | (code & 0x3FUL));
        } else if (code < 0x10000UL) {
            text += static_cast<char>(0xE0UL | (code >> 12U));
            text += static_cast<char>(0x80UL | ((code >> 6U) & 0x3FUL));
            text += static_cast<char>(0x80UL | (code & 0x3FUL));
        } else {
            text += static_cast<char>(0xF0UL | (code >> 18U));
            text += static_cast<char>(0x80UL | ((code >> 12U) & 0x3FUL));
            text += static_cast<char>(0x80UL | ((code >> 6U) & 0x3FUL));
            text += static_cast<char>(0x80UL | (code & 0x3FUL));
        }
    }
    if (text.size() >= 2U && text[1] == ':') {
        const char drive = static_cast<char>(std::tolower(text[0]));
        const std::string rest = text.substr(2U);
        if (drive == 'z') {
            return std::filesystem::path(rest.empty() ? "/" : rest);
        }
        // Other drives are links in the prefix: <prefix>/dosdevices/c:
        const char* prefix = std::getenv("WINEPREFIX");
        const char* home = std::getenv("HOME");
        std::filesystem::path link =
            prefix != nullptr
                ? std::filesystem::path(prefix)
                : std::filesystem::path(home != nullptr ? home : "") / ".wine";
        link /= std::string("dosdevices/") + drive + ":";
        std::error_code ignored;
        const auto target = std::filesystem::read_symlink(link, ignored);
        std::filesystem::path base = rest;
        if (!ignored) {
            base = target.is_absolute() ? target : link.parent_path() / target;
            base += rest;
        }
        return base;
    }
    return std::filesystem::path(text);
}
#endif
