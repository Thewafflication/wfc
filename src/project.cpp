#include "wfc/project.hpp"
#include <cstdint>

#include "module_linker.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <string_view>

namespace wfc {
namespace {

[[nodiscard]] std::string lowered(const std::string_view text) {
    std::string result;
    for (const char c : text) {
        result.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
}

[[nodiscard]] std::string trimmed(const std::string_view text) {
    std::size_t first = 0;
    std::size_t last = text.size();
    while (first < last &&
           std::isspace(static_cast<unsigned char>(text[first])) != 0) {
        ++first;
    }
    while (last > first &&
           std::isspace(static_cast<unsigned char>(text[last - 1])) != 0) {
        --last;
    }
    return std::string(text.substr(first, last - first));
}

[[nodiscard]] bool is_valid_utf8(const std::string_view text) {
    std::size_t i = 0;
    while (i < text.size()) {
        const auto lead = static_cast<unsigned char>(text[i]);
        std::size_t length = lead < 0x80U                      ? 1U
                             : (lead >= 0xC2U && lead < 0xE0U) ? 2U
                             : (lead >= 0xE0U && lead < 0xF0U) ? 3U
                             : (lead >= 0xF0U && lead < 0xF5U) ? 4U
                                                               : 0U;
        if (length == 0U || i + length > text.size()) {
            return false;
        }
        for (std::size_t k = 1; k < length; ++k) {
            if ((static_cast<unsigned char>(text[i + k]) & 0xC0U) != 0x80U) {
                return false;
            }
        }
        i += length;
    }
    return true;
}

// Windows-1252 bytes to UTF-8 (the 0x80-0x9F block maps to its typographic
// characters).
[[nodiscard]] std::string ansi_to_utf8(const std::string_view text) {
    static constexpr std::uint16_t table[32] = {
        0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
        0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
        0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
        0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178};
    std::string result;
    result.reserve(text.size());
    for (const char character : text) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte < 0x80U) {
            result.push_back(character);
            continue;
        }
        const unsigned code = byte <= 0x9FU ? table[byte - 0x80U] : byte;
        if (code < 0x800U) {
            result.push_back(static_cast<char>(0xC0U | (code >> 6U)));
            result.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
        } else {
            result.push_back(static_cast<char>(0xE0U | (code >> 12U)));
            result.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3FU)));
            result.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
        }
    }
    return result;
}

[[nodiscard]] bool read_file(const std::filesystem::path& path,
                             std::string& out) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(stream),
               std::istreambuf_iterator<char>());
    // A byte order mark from an editor: UTF-8 is dropped, UTF-16 is decoded.
    if (out.size() >= 3U && static_cast<unsigned char>(out[0]) == 0xEFU &&
        static_cast<unsigned char>(out[1]) == 0xBBU &&
        static_cast<unsigned char>(out[2]) == 0xBFU) {
        out.erase(0U, 3U);
    } else if (out.size() >= 2U &&
               ((static_cast<unsigned char>(out[0]) == 0xFFU &&
                 static_cast<unsigned char>(out[1]) == 0xFEU) ||
                (static_cast<unsigned char>(out[0]) == 0xFEU &&
                 static_cast<unsigned char>(out[1]) == 0xFFU))) {
        const bool big_endian = static_cast<unsigned char>(out[0]) == 0xFEU;
        std::string utf8;
        for (std::size_t i = 2U; i + 1U < out.size(); i += 2U) {
            const auto low =
                static_cast<unsigned char>(out[i + (big_endian ? 1U : 0U)]);
            const auto high =
                static_cast<unsigned char>(out[i + (big_endian ? 0U : 1U)]);
            unsigned code = static_cast<unsigned>(low) |
                            (static_cast<unsigned>(high) << 8U);
            if (code >= 0xD800U && code < 0xDC00U && i + 3U < out.size()) {
                const auto low2 = static_cast<unsigned char>(
                    out[i + 2U + (big_endian ? 1U : 0U)]);
                const auto high2 = static_cast<unsigned char>(
                    out[i + 2U + (big_endian ? 0U : 1U)]);
                const unsigned second = static_cast<unsigned>(low2) |
                                        (static_cast<unsigned>(high2) << 8U);
                if (second >= 0xDC00U && second < 0xE000U) {
                    code = 0x10000U + ((code - 0xD800U) << 10U) +
                           (second - 0xDC00U);
                    i += 2U;
                }
            }
            if (code < 0x80U) {
                utf8.push_back(static_cast<char>(code));
            } else if (code < 0x800U) {
                utf8.push_back(static_cast<char>(0xC0U | (code >> 6U)));
                utf8.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
            } else if (code < 0x10000U) {
                utf8.push_back(static_cast<char>(0xE0U | (code >> 12U)));
                utf8.push_back(
                    static_cast<char>(0x80U | ((code >> 6U) & 0x3FU)));
                utf8.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
            } else {
                utf8.push_back(static_cast<char>(0xF0U | (code >> 18U)));
                utf8.push_back(
                    static_cast<char>(0x80U | ((code >> 12U) & 0x3FU)));
                utf8.push_back(
                    static_cast<char>(0x80U | ((code >> 6U) & 0x3FU)));
                utf8.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
            }
        }
        out = std::move(utf8);
    }
    if (!is_valid_utf8(out)) {
        out = ansi_to_utf8(out);  // classic VB6 source files are Windows-1252
    }
    return true;
}

// Blanks VERSION / BEGIN..END / Attribute lines; returns the VB_Name if any.
[[nodiscard]] std::string strip_file_header(std::string& text,
                                            const bool keep_vb_name = false) {
    std::string name;
    std::string result;
    std::istringstream stream(text);
    std::string line;
    bool in_begin_block = false;
    // VERSION and BEGIN..END only form the file header before any code; a
    // later `Version = 1` or `Begin = 2` is an ordinary statement.
    bool in_header = true;
    while (std::getline(stream, line)) {
        const bool had_cr = !line.empty() && line.back() == '\r';
        std::string body = had_cr ? line.substr(0, line.size() - 1) : line;
        const std::string lower = lowered(trimmed(body));
        bool blank = false;
        if (in_begin_block) {
            blank = true;
            if (lower == "end") {
                in_begin_block = false;
            }
        } else if (in_header && lower.rfind("version ", 0) == 0 &&
                   lower.size() > 8U &&
                   std::isdigit(static_cast<unsigned char>(lower[8])) != 0) {
            blank = true;
        } else if (in_header &&
                   (lower == "begin" || lower.rfind("begin ", 0) == 0) &&
                   lower.find('=') == std::string::npos) {
            blank = true;
            in_begin_block = true;
        } else if (lower.rfind("attribute ", 0) == 0) {
            // Keep `Attribute X.VB_UserMemId = 0`: the evaluator reads it as
            // the class's default member.
            blank = lower.find("vb_usermemid") == std::string::npos &&
                    lower.find("vb_predeclaredid") == std::string::npos &&
                    lower.find("vb_globalnamespace") == std::string::npos &&
                    !(keep_vb_name && lower.rfind("attribute vb_name", 0) == 0);
            if (lower.rfind("attribute vb_name", 0) == 0) {
                const auto first = body.find('"');
                const auto last = body.rfind('"');
                if (first != std::string::npos && last > first) {
                    name = body.substr(first + 1, last - first - 1);
                }
            }
        }
        if (!blank && !lower.empty() && lower.front() != '\'' &&
            lower.rfind("attribute ", 0) != 0 && lower.rfind("rem", 0) != 0) {
            in_header = false;
        }
        if (blank) {
            body.clear();
        }
        result += body;
        result += had_cr ? "\r\n" : "\n";
    }
    text = std::move(result);
    return name;
}

[[nodiscard]] bool declares_sub_main(const std::string& text) {
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        std::string lower = lowered(trimmed(line));
        for (const char* prefix : {"public ", "private ", "friend "}) {
            if (lower.rfind(prefix, 0) == 0) {
                lower = trimmed(lower.substr(std::string_view(prefix).size()));
            }
        }
        if (lower.rfind("sub main", 0) == 0 &&
            (lower.size() == 8 || lower[8] == '(' || lower[8] == ' ')) {
            return true;
        }
    }
    return false;
}

// The `App` property a lower-cased .vbp key feeds, or empty.
[[nodiscard]] std::string app_property_for(const std::string& key) {
    static const std::map<std::string, std::string> keys = {
        {"title", "title"},
        {"exename32", "exename"},
        {"majorver", "major"},
        {"minorver", "minor"},
        {"revisionver", "revision"},
        {"versioncompanyname", "companyname"},
        {"versionproductname", "productname"},
        {"versionfiledescription", "filedescription"},
        {"versioncomments", "comments"},
        {"versionlegalcopyright", "legalcopyright"},
        {"versionlegaltrademarks", "legaltrademarks"}};
    const auto found = keys.find(key);
    return found == keys.end() ? std::string{} : found->second;
}

[[nodiscard]] std::filesystem::path normalize(std::string text) {
    std::replace(text.begin(), text.end(), '\\', '/');
    return std::filesystem::path(text);
}

}  // namespace

LoadedProject load_project(const std::vector<std::filesystem::path>& paths) {
    LoadedProject project;
    const auto fail = [&](std::string message) {
        project.ok = false;
        project.error = std::move(message);
        return project;
    };
    std::vector<std::filesystem::path> modules;
    std::vector<std::string> module_declared_names;
    std::vector<std::pair<std::string, std::filesystem::path>> classes;
    bool startup_sub_main = true;

    for (const auto& path : paths) {
        const std::string extension = lowered(path.extension().string());
        if (extension == ".vbp") {
            std::string text;
            if (!read_file(path, text)) {
                return fail("cannot read project file: " + path.string());
            }
            const auto directory = path.has_parent_path()
                                       ? path.parent_path()
                                       : std::filesystem::path(".");
            std::istringstream stream(text);
            std::string line;
            while (std::getline(stream, line)) {
                line = trimmed(line);
                const auto equals = line.find('=');
                if (equals == std::string::npos) {
                    continue;
                }
                const std::string key =
                    lowered(trimmed(line.substr(0, equals)));
                std::string value = trimmed(line.substr(equals + 1));
                if (key == "module" || key == "class") {
                    const auto semicolon = value.find(';');
                    if (semicolon == std::string::npos) {
                        return fail("malformed " + key + " entry: " + line);
                    }
                    const std::string name =
                        trimmed(value.substr(0, semicolon));
                    const auto file =
                        directory /
                        normalize(trimmed(value.substr(semicolon + 1)));
                    if (key == "module") {
                        modules.push_back(file);
                        module_declared_names.push_back(name);
                    } else {
                        classes.emplace_back(name, file);
                    }
                } else if (key == "form" || key == "usercontrol" ||
                           key == "propertypage" || key == "designer") {
                    return fail("unsupported project item (visual designer): " +
                                line);
                } else if (const auto app_key = app_property_for(key);
                           !app_key.empty()) {
                    if (value.size() >= 2U && value.front() == '"' &&
                        value.back() == '"') {
                        value = value.substr(1, value.size() - 2U);
                    }
                    project.app_properties[app_key] = value;
                } else if (key == "reference") {
                    // `*\G{GUID}#major.minor#lcid#path#description`
                    const auto brace = value.find('{');
                    const auto close = value.find('}');
                    if (brace != std::string::npos &&
                        close != std::string::npos && close > brace) {
                        std::vector<std::string> parts;
                        std::string part;
                        for (const char c : value) {
                            if (c == '#') {
                                parts.push_back(part);
                                part.clear();
                            } else {
                                part.push_back(c);
                            }
                        }
                        parts.push_back(part);
                        std::string spec =
                            value.substr(brace, close - brace + 1U);
                        spec += '#';
                        spec += parts.size() > 1U ? parts[1] : std::string{};
                        spec += '#';
                        if (parts.size() > 3U && !parts[3].empty()) {
                            spec += (directory / normalize(parts[3])).string();
                        }
                        project.type_libraries.push_back(std::move(spec));
                    }
                } else if (key == "resfile32") {
                    if (value.size() >= 2U && value.front() == '"' &&
                        value.back() == '"') {
                        value = value.substr(1, value.size() - 2U);
                    }
                    project.resource_file =
                        (directory / normalize(value)).string();
                } else if (key == "startup") {
                    if (!value.empty() && value.front() == '"') {
                        value = value.substr(
                            1, value.size() >= 2 ? value.size() - 2 : 0);
                    }
                    startup_sub_main =
                        lowered(value) == "sub main" || value.empty();
                    if (!startup_sub_main && lowered(value) != "(none)") {
                        return fail("unsupported Startup object: " + value);
                    }
                }
            }
        } else if (extension == ".bas") {
            modules.push_back(path);
            module_declared_names.emplace_back();
        } else if (extension == ".cls") {
            classes.emplace_back(std::string{}, path);
        } else {
            return fail("unsupported file type: " + path.string());
        }
    }

    bool has_main = false;
    std::vector<std::string> module_texts;
    std::vector<std::string> module_names;
    for (std::size_t index = 0; index < modules.size(); ++index) {
        const auto& module_path = modules[index];
        std::string text;
        if (!read_file(module_path, text)) {
            return fail("cannot read module: " + module_path.string());
        }
        std::string name = strip_file_header(text, /*keep_vb_name=*/true);
        if (name.empty()) {
            name = module_declared_names[index];
        }
        if (name.empty()) {
            name = module_path.stem().string();
        }
        has_main = has_main || declares_sub_main(text);
        module_texts.push_back(std::move(text));
        module_names.push_back(std::move(name));
    }
    std::vector<std::string> class_texts;
    for (const auto& [declared_name, class_path] : classes) {
        std::string text;
        if (!read_file(class_path, text)) {
            return fail("cannot read class: " + class_path.string());
        }
        class_texts.push_back(std::move(text));
    }
    // Modules keep separate namespaces and one merged Option header.
    std::vector<bool> explicit_modules;
    project.module_source = detail::link_modules(module_texts, module_names,
                                                 class_texts, explicit_modules);
    project.per_module_option_explicit = modules.size() >= 2U;
    for (std::size_t index = 0; index < modules.size(); ++index) {
        project.module_spans.push_back(LoadedProject::ModuleSpan{
            modules[index].string(), static_cast<std::size_t>(std::count(
                                         project.module_source.begin(),
                                         project.module_source.end(), '\n')) +
                                         1U});
        const auto range_begin = project.module_source.size();
        project.module_source += module_texts[index];
        if (explicit_modules[index]) {
            project.option_explicit_ranges.emplace_back(
                range_begin, project.module_source.size());
        }
        project.module_source += "\n";
    }
    std::size_t class_index = 0;
    for (const auto& [declared_name, class_path] : classes) {
        std::string text = std::move(class_texts[class_index++]);
        std::string name = strip_file_header(text);
        if (!declared_name.empty()) {
            name = declared_name;
        }
        if (name.empty()) {
            name = class_path.stem().string();
        }
        project.class_names.push_back(std::move(name));
        project.class_sources.push_back(std::move(text));
        project.class_files.push_back(class_path.string());
    }
    if (startup_sub_main && has_main) {
        project.module_source += "Call Main\n";
    }
    if (project.module_source.empty()) {
        return fail("project has no standard module to run");
    }
    project.ok = true;
    return project;
}

}  // namespace wfc
