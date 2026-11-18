// Interpreter: native emulation of a few common Win32 routines reached through
// `Declare` (profile/INI files, user and path queries). Internal to the WFC
// evaluator; not part of the public API.

#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include "interpreter.hpp"

namespace wfc::detail {
namespace {

[[nodiscard]] std::string lower_text(const std::string_view text) {
    std::string result;
    for (const char c : text) {
        result.push_back(ascii_lower(c));
    }
    return result;
}

[[nodiscard]] std::string trim_text(const std::string_view text) {
    std::size_t first = 0;
    std::size_t last = text.size();
    while (first < last &&
           (text[first] == ' ' || text[first] == '\t' || text[first] == '\r')) {
        ++first;
    }
    while (last > first && (text[last - 1U] == ' ' || text[last - 1U] == '\t' ||
                            text[last - 1U] == '\r')) {
        --last;
    }
    return std::string(text.substr(first, last - first));
}

struct IniSection {
    std::string name;
    std::vector<std::pair<std::string, std::string>> entries;
};

[[nodiscard]] std::vector<IniSection> read_ini(const std::string& path) {
    std::vector<IniSection> sections;
    std::ifstream stream(path);
    std::string line;
    while (std::getline(stream, line)) {
        const auto text = trim_text(line);
        if (text.empty() || text.front() == ';') {
            continue;
        }
        if (text.front() == '[') {
            const auto close = text.find(']');
            sections.push_back(
                {text.substr(1U, close == std::string::npos ? std::string::npos
                                                            : close - 1U),
                 {}});
        } else if (!sections.empty()) {
            const auto equals = text.find('=');
            if (equals != std::string::npos) {
                sections.back().entries.emplace_back(
                    trim_text(text.substr(0, equals)),
                    trim_text(text.substr(equals + 1U)));
            }
        }
    }
    return sections;
}

void write_ini(const std::string& path,
               const std::vector<IniSection>& sections) {
    std::ofstream stream(path, std::ios::trunc | std::ios::binary);
    for (const auto& section : sections) {
        stream << '[' << section.name << "]\r\n";
        for (const auto& [key, value] : section.entries) {
            stream << key << '=' << value << "\r\n";
        }
        stream << "\r\n";
    }
}

[[nodiscard]] const std::string* text_of(const Value& value) {
    return std::get_if<std::string>(&value);
}

}  // namespace

std::optional<Value> Interpreter::emulate_win32_call(
    std::string name, std::vector<CallArgument>& arguments,
    const std::size_t offset) {
    // The ANSI (`A`) and Unicode (`W`) entry points behave alike here.
    static const std::set<std::string> known = {"openprocess",
                                                "waitforsingleobject",
                                                "getexitcodeprocess",
                                                "closehandle",
                                                "getprivateprofilestring",
                                                "writeprivateprofilestring",
                                                "getprivateprofileint",
                                                "getusername",
                                                "getcomputername",
                                                "gettemppath",
                                                "getwindowsdirectory",
                                                "getsystemdirectory",
                                                "getcurrentdirectory",
                                                "setcurrentdirectory",
                                                "getenvironmentvariable",
                                                "lstrlen",
                                                "messagebox",
                                                "getfileattributes",
                                                "deletefile",
                                                "createdirectory",
                                                "removedirectory"};
    if (!known.contains(name) && name.size() > 1U &&
        (name.back() == 'a' || name.back() == 'w') &&
        known.contains(name.substr(0, name.size() - 1U))) {
        name.pop_back();
    }
    const auto argument_text = [&](const std::size_t index) -> std::string {
        if (index >= arguments.size()) {
            return {};
        }
        if (const auto* text = text_of(arguments[index].value)) {
            return *text;
        }
        return {};
    };
    const auto argument_long = [&](const std::size_t index) -> Integer {
        if (index >= arguments.size()) {
            return 0;
        }
        return whole_value(arguments[index].value).value_or(0);
    };
    // Writes `text` (plus a NUL) into a ByRef String buffer argument.
    const auto fill_buffer = [&](const std::size_t index,
                                 const std::string& text,
                                 const Integer capacity) -> Integer {
        if (index >= arguments.size() ||
            arguments[index].byref_target == nullptr) {
            return 0;
        }
        auto* const buffer = text_of(*arguments[index].byref_target);
        if (buffer == nullptr) {
            return 0;
        }
        std::string stored = text;
        if (capacity > 0 &&
            stored.size() > static_cast<std::size_t>(capacity - 1)) {
            stored.resize(static_cast<std::size_t>(capacity - 1));
        }
        std::string updated = stored;
        updated.push_back('\0');
        if (buffer->size() > updated.size()) {
            updated += buffer->substr(updated.size());
        }
        *arguments[index].byref_target = Value{std::move(updated)};
        return static_cast<Integer>(stored.size());
    };

#ifdef _WIN32
    // The "ShellAndWait" idiom: OpenProcess / WaitForSingleObject /
    // GetExitCodeProcess / CloseHandle on the id Shell returned.
    const auto handle_of = [&](const Integer value) {
        return reinterpret_cast<HANDLE>(
            static_cast<std::uintptr_t>(static_cast<std::uint32_t>(value)));
    };
    if (name == "openprocess" && arguments.size() == 3U) {
        const HANDLE opened = OpenProcess(static_cast<DWORD>(argument_long(0)),
                                          argument_long(1) != 0 ? TRUE : FALSE,
                                          static_cast<DWORD>(argument_long(2)));
        return Value{static_cast<Integer>(static_cast<std::int32_t>(
            reinterpret_cast<std::uintptr_t>(opened)))};
    }
    if (name == "waitforsingleobject" && arguments.size() == 2U) {
        const auto milliseconds = argument_long(1);
        return Value{static_cast<Integer>(WaitForSingleObject(
            handle_of(argument_long(0)),
            milliseconds == -1 ? INFINITE : static_cast<DWORD>(milliseconds)))};
    }
    if (name == "getexitcodeprocess" && arguments.size() == 2U) {
        DWORD code = 0;
        const BOOL ok = GetExitCodeProcess(handle_of(argument_long(0)), &code);
        if (arguments[1].byref_target != nullptr) {
            *arguments[1].byref_target = Value{static_cast<Integer>(code)};
        }
        return Value{Integer{ok != 0 ? 1 : 0}};
    }
    if (name == "closehandle" && arguments.size() == 1U) {
        return Value{
            Integer{CloseHandle(handle_of(argument_long(0))) != 0 ? 1 : 0}};
    }
#endif
    if (name == "getprivateprofilestring" && arguments.size() == 6U) {
        const auto sections = read_ini(argument_text(5));
        const auto section_key = lower_text(argument_text(0));
        const auto key = lower_text(argument_text(1));
        std::string found = argument_text(2);
        std::string listing;
        for (const auto& section : sections) {
            if (lower_text(section.name) != section_key) {
                continue;
            }
            for (const auto& [entry_key, entry_value] : section.entries) {
                if (key.empty()) {
                    listing += entry_key;
                    listing.push_back('\0');
                } else if (lower_text(entry_key) == key) {
                    found = entry_value;
                    break;
                }
            }
        }
        const auto capacity = argument_long(4);
        if (key.empty() && !section_key.empty()) {
            return Value{fill_buffer(3, listing, capacity)};
        }
        return Value{fill_buffer(3, found, capacity)};
    }
    if (name == "getprivateprofileint" && arguments.size() == 4U) {
        const auto sections = read_ini(argument_text(3));
        const auto section_key = lower_text(argument_text(0));
        const auto key = lower_text(argument_text(1));
        Integer result = argument_long(2);
        for (const auto& section : sections) {
            if (lower_text(section.name) != section_key) {
                continue;
            }
            for (const auto& [entry_key, entry_value] : section.entries) {
                if (lower_text(entry_key) == key) {
                    result =
                        static_cast<Integer>(std::atol(entry_value.c_str()));
                }
            }
        }
        return Value{result};
    }
    if (name == "writeprivateprofilestring" && arguments.size() == 4U) {
        const auto path = argument_text(3);
        auto sections = read_ini(path);
        const auto section_name = argument_text(0);
        const auto key = argument_text(1);
        auto section = std::find_if(
            sections.begin(), sections.end(), [&](const IniSection& s) {
                return lower_text(s.name) == lower_text(section_name);
            });
        if (section == sections.end()) {
            sections.push_back({section_name, {}});
            section = sections.end() - 1;
        }
        auto entry =
            std::find_if(section->entries.begin(), section->entries.end(),
                         [&](const auto& e) {
                             return lower_text(e.first) == lower_text(key);
                         });
        if (entry == section->entries.end()) {
            section->entries.emplace_back(key, argument_text(2));
        } else {
            entry->second = argument_text(2);
        }
        write_ini(path, sections);
        return Value{Integer{1}};
    }
    if ((name == "getusername" || name == "getcomputername") &&
        arguments.size() == 2U) {
        const std::string value = environment_variable(
            name == "getusername" ? "USERNAME" : "COMPUTERNAME");
        const auto length = fill_buffer(0, value, argument_long(1));
        if (arguments[1].byref_target != nullptr) {
            *arguments[1].byref_target = Value{length + 1};
        }
        return Value{Integer{1}};
    }
    if (name == "gettemppath" && arguments.size() == 2U) {
        std::string path = environment_variable("TEMP");
        if (!path.empty() && path.back() != '\\') {
            path.push_back('\\');
        }
        return Value{fill_buffer(1, path, argument_long(0))};
    }
    if ((name == "getwindowsdirectory" || name == "getsystemdirectory") &&
        arguments.size() == 2U) {
        std::string path = environment_variable("SystemRoot");
        if (name == "getsystemdirectory") {
            path += "\\System32";
        }
        return Value{fill_buffer(0, path, argument_long(1))};
    }
    if (name == "getcurrentdirectory" && arguments.size() == 2U) {
        std::error_code ec;
        return Value{fill_buffer(1, std::filesystem::current_path(ec).string(),
                                 argument_long(0))};
    }
    if (name == "setcurrentdirectory" && arguments.size() == 1U) {
        std::error_code ec;
        std::filesystem::current_path(argument_text(0), ec);
        return Value{Integer{ec ? 0 : 1}};
    }
    if (name == "getenvironmentvariable" && arguments.size() == 3U) {
        const auto value = environment_variable(argument_text(0));
        if (value.empty()) {
            return Value{Integer{0}};
        }
        return Value{fill_buffer(1, value, argument_long(2))};
    }
    if (name == "lstrlen" && arguments.size() == 1U) {
        return Value{
            static_cast<Integer>(text_to_ansi_bytes(argument_text(0)).size())};
    }
    if (name == "getfileattributes" && arguments.size() == 1U) {
        std::error_code ec;
        const std::filesystem::path path(argument_text(0));
        if (!std::filesystem::exists(path, ec)) {
            return Value{Integer{-1}};
        }
        return Value{
            Integer{std::filesystem::is_directory(path, ec) ? 16 : 128}};
    }
    if (name == "deletefile" && arguments.size() == 1U) {
        std::error_code ec;
        return Value{
            Integer{std::filesystem::remove(argument_text(0), ec) ? 1 : 0}};
    }
    if (name == "createdirectory" && !arguments.empty()) {
        std::error_code ec;
        return Value{Integer{
            std::filesystem::create_directory(argument_text(0), ec) ? 1 : 0}};
    }
    if (name == "removedirectory" && arguments.size() == 1U) {
        std::error_code ec;
        return Value{
            Integer{std::filesystem::remove(argument_text(0), ec) ? 1 : 0}};
    }
    static_cast<void>(offset);
    return std::nullopt;
}

std::string system_error_text(const std::uint32_t code) {
    wchar_t* buffer = nullptr;
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
    if (length == 0 || buffer == nullptr) {
        return {};
    }
    std::u16string units(reinterpret_cast<const char16_t*>(buffer), length);
    LocalFree(buffer);
    // The line break that ends the text becomes a blank, as Visual Basic
    // shows it ("Invalid advise flags ").
    bool ended = false;
    while (!units.empty() && (units.back() == u'\r' || units.back() == u'\n')) {
        units.pop_back();
        ended = true;
    }
    if (ended) {
        units.push_back(u' ');
    }
    return from_utf16_units(units);
}

unsigned ansi_best_fit(const std::uint32_t code) noexcept {
    if (code > 0xFFFFU) {
        return 0U;
    }
    const wchar_t wide = static_cast<wchar_t>(code);
    char narrow[4]{};
    BOOL defaulted = FALSE;
    // 1252 is the code page the rest of the text conversions use.
    const int written = WideCharToMultiByte(
        1252, 0, &wide, 1, narrow, sizeof(narrow), nullptr, &defaulted);
    if (written != 1 || defaulted != FALSE) {
        return 0U;
    }
    return static_cast<unsigned char>(narrow[0]);
}

int text_order(const std::string_view left, const std::string_view right) {
    // Letters and digits of ASCII alone order the same either way.
    const auto plain = [](const std::string_view text) {
        for (const char c : text) {
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9'))) {
                return false;
            }
        }
        return true;
    };
    if (plain(left) && plain(right)) {
        const auto common = std::min(left.size(), right.size());
        for (std::size_t index = 0; index < common; ++index) {
            const auto a = static_cast<unsigned char>(ascii_lower(left[index]));
            const auto b =
                static_cast<unsigned char>(ascii_lower(right[index]));
            if (a != b) {
                return a < b ? -1 : 1;
            }
        }
        return left.size() == right.size()
                   ? 0
                   : (left.size() < right.size() ? -1 : 1);
    }
    const std::u16string wide_left = to_utf16_units(std::string(left));
    const std::u16string wide_right = to_utf16_units(std::string(right));
    const int order =
        CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       reinterpret_cast<const wchar_t*>(wide_left.data()),
                       static_cast<int>(wide_left.size()),
                       reinterpret_cast<const wchar_t*>(wide_right.data()),
                       static_cast<int>(wide_right.size()));
    if (order == 0) {
        // The system could not compare them: the folded texts decide.
        const int folded =
            fold_case(std::string(left)).compare(fold_case(std::string(right)));
        return (folded > 0) - (folded < 0);
    }
    return order - 2;  // CSTR_LESS_THAN is 1, CSTR_GREATER_THAN is 3
}

}  // namespace wfc::detail
