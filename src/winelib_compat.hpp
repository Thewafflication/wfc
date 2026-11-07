// The Microsoft C runtime functions that WFC calls and that glibc (Winelib
// builds with wineg++) does not have. Force-included by cmake/winelib.cmake.
#pragma once

#ifdef __WINE__
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>

#ifndef _TRUNCATE
#define _TRUNCATE (static_cast<std::size_t>(-1))
#endif
#define _environ environ

// winegcc compiles with a 16-bit wchar_t but glibc's wcslen and wmem* count
// 32-bit units, and libstdc++'s char_traits<wchar_t> calls them: give it
// functions that count the wchar_t of this program.
static inline std::size_t wfc_wcslen(const wchar_t* text) {
    std::size_t length = 0;
    while (text[length] != 0) {
        ++length;
    }
    return length;
}
static inline wchar_t* wfc_wmemcpy(wchar_t* to, const wchar_t* from,
                                   std::size_t count) {
    std::memcpy(to, from, count * sizeof(wchar_t));
    return to;
}
static inline wchar_t* wfc_wmemmove(wchar_t* to, const wchar_t* from,
                                    std::size_t count) {
    std::memmove(to, from, count * sizeof(wchar_t));
    return to;
}
static inline wchar_t* wfc_wmemset(wchar_t* to, wchar_t value,
                                   std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        to[i] = value;
    }
    return to;
}
static inline int wfc_wmemcmp(const wchar_t* a, const wchar_t* b,
                              std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        if (a[i] != b[i]) {
            return a[i] < b[i] ? -1 : 1;
        }
    }
    return 0;
}
static inline const wchar_t* wfc_wmemchr(const wchar_t* text, wchar_t value,
                                         std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        if (text[i] == value) {
            return text + i;
        }
    }
    return nullptr;
}
#define wcslen wfc_wcslen
#define wmemcpy wfc_wmemcpy
#define wmemmove wfc_wmemmove
#define wmemset wfc_wmemset
#define wmemcmp wfc_wmemcmp
#define wmemchr wfc_wmemchr

// winegcc defines _WIN32, which makes libstdc++ read std::filesystem::path as
// a wide Windows path that its streams cannot open; include those headers
// while it is hidden (the include guards keep it that way).
#pragma push_macro("_WIN32")
#undef _WIN32
#include <filesystem>
#include <fstream>
#pragma pop_macro("_WIN32")

#define IID_PPV_ARGS(ppType) \
    __uuidof(**(ppType)), reinterpret_cast<void**>(ppType)

inline int localtime_s(std::tm* result, const std::time_t* time) {
    return localtime_r(time, result) == nullptr ? errno : 0;
}

inline int gmtime_s(std::tm* result, const std::time_t* time) {
    return gmtime_r(time, result) == nullptr ? errno : 0;
}

inline int _dupenv_s(char** buffer, std::size_t* length, const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr) {
        *buffer = nullptr;
        if (length != nullptr) {
            *length = 0;
        }
        return 0;
    }
    *buffer = static_cast<char*>(std::malloc(std::strlen(value) + 1U));
    if (*buffer == nullptr) {
        return ENOMEM;
    }
    std::strcpy(*buffer, value);
    if (length != nullptr) {
        *length = std::strlen(value) + 1U;
    }
    return 0;
}

inline int fopen_s(std::FILE** file, const char* path, const char* mode) {
    *file = std::fopen(path, mode);
    return *file == nullptr ? errno : 0;
}

template <std::size_t Size>
inline int wcsncpy_s(wchar_t (&destination)[Size], const wchar_t* source,
                     std::size_t count);

inline int wcsncpy_s(wchar_t* destination, std::size_t size,
                     const wchar_t* source, std::size_t count) {
    if (destination == nullptr || size == 0U) {
        return EINVAL;
    }
    std::size_t length = wfc_wcslen(source);
    if (count != _TRUNCATE && count < length) {
        length = count;
    }
    if (length >= size) {
        if (count != _TRUNCATE) {
            destination[0] = L'\0';
            return ERANGE;
        }
        length = size - 1U;
    }
    wfc_wmemcpy(destination, source, length);
    destination[length] = L'\0';
    return 0;
}

template <std::size_t Size>
inline int wcsncpy_s(wchar_t (&destination)[Size], const wchar_t* source,
                     std::size_t count) {
    return wcsncpy_s(&destination[0], Size, source, count);
}
#endif
