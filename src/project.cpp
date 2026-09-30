#include "wfc/project.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string_view>

namespace wfc {
namespace {

[[nodiscard]] std::string lowered(const std::string_view text) {
    std::string result;
    for (const char c : text) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
}

[[nodiscard]] std::string trimmed(const std::string_view text) {
    std::size_t first = 0;
    std::size_t last = text.size();
    while (first < last && std::isspace(static_cast<unsigned char>(text[first])) != 0) ++first;
    while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1])) != 0) --last;
    return std::string(text.substr(first, last - first));
}

[[nodiscard]] bool read_file(const std::filesystem::path& path, std::string& out) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    return true;
}

// Blanks VERSION / BEGIN..END / Attribute lines; returns the VB_Name if any.
[[nodiscard]] std::string strip_file_header(std::string& text) {
    std::string name;
    std::string result;
    std::istringstream stream(text);
    std::string line;
    bool in_begin_block = false;
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
        } else if (lower.rfind("version ", 0) == 0) {
            blank = true;
        } else if (lower == "begin" || lower.rfind("begin ", 0) == 0) {
            blank = true;
            in_begin_block = true;
        } else if (lower.rfind("attribute ", 0) == 0) {
            // Keep `Attribute X.VB_UserMemId = 0`: the evaluator reads it as
            // the class's default member.
            blank = lower.find("vb_usermemid") == std::string::npos;
            if (lower.rfind("attribute vb_name", 0) == 0) {
                const auto first = body.find('"');
                const auto last = body.rfind('"');
                if (first != std::string::npos && last > first) {
                    name = body.substr(first + 1, last - first - 1);
                }
            }
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
    std::vector<std::pair<std::string, std::filesystem::path>> classes;
    bool startup_sub_main = true;

    for (const auto& path : paths) {
        const std::string extension = lowered(path.extension().string());
        if (extension == ".vbp") {
            std::string text;
            if (!read_file(path, text)) {
                return fail("cannot read project file: " + path.string());
            }
            const auto directory = path.has_parent_path() ? path.parent_path() : std::filesystem::path(".");
            std::istringstream stream(text);
            std::string line;
            while (std::getline(stream, line)) {
                line = trimmed(line);
                const auto equals = line.find('=');
                if (equals == std::string::npos) continue;
                const std::string key = lowered(trimmed(line.substr(0, equals)));
                std::string value = trimmed(line.substr(equals + 1));
                if (key == "module" || key == "class") {
                    const auto semicolon = value.find(';');
                    if (semicolon == std::string::npos) {
                        return fail("malformed " + key + " entry: " + line);
                    }
                    const std::string name = trimmed(value.substr(0, semicolon));
                    const auto file = directory / normalize(trimmed(value.substr(semicolon + 1)));
                    if (key == "module") {
                        modules.push_back(file);
                    } else {
                        classes.emplace_back(name, file);
                    }
                } else if (key == "form" || key == "usercontrol" || key == "propertypage" ||
                           key == "designer") {
                    return fail("unsupported project item (visual designer): " + line);
                } else if (key == "startup") {
                    if (!value.empty() && value.front() == '"') {
                        value = value.substr(1, value.size() >= 2 ? value.size() - 2 : 0);
                    }
                    startup_sub_main = lowered(value) == "sub main" || value.empty();
                    if (!startup_sub_main && lowered(value) != "(none)") {
                        return fail("unsupported Startup object: " + value);
                    }
                }
            }
        } else if (extension == ".bas") {
            modules.push_back(path);
        } else if (extension == ".cls") {
            classes.emplace_back(std::string{}, path);
        } else {
            return fail("unsupported file type: " + path.string());
        }
    }

    bool has_main = false;
    for (const auto& module_path : modules) {
        std::string text;
        if (!read_file(module_path, text)) {
            return fail("cannot read module: " + module_path.string());
        }
        static_cast<void>(strip_file_header(text));
        has_main = has_main || declares_sub_main(text);
        project.module_source += text;
        project.module_source += "\n";
    }
    for (const auto& [declared_name, class_path] : classes) {
        std::string text;
        if (!read_file(class_path, text)) {
            return fail("cannot read class: " + class_path.string());
        }
        std::string name = strip_file_header(text);
        if (!declared_name.empty()) {
            name = declared_name;
        }
        if (name.empty()) {
            name = class_path.stem().string();
        }
        project.class_names.push_back(std::move(name));
        project.class_sources.push_back(std::move(text));
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
