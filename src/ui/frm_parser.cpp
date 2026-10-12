// Parser for Visual Basic 6 form files (.frm) and reader for .frx resources.
// Internal to the WFC runtime; not part of the public API.

#include "frm_parser.hpp"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>

namespace wfc::ui {
namespace {

[[nodiscard]] std::string trim(const std::string_view text) {
    std::size_t first = 0;
    std::size_t last = text.size();
    while (first < last &&
           std::isspace(static_cast<unsigned char>(text[first])) != 0) {
        ++first;
    }
    while (last > first &&
           std::isspace(static_cast<unsigned char>(text[last - 1U])) != 0) {
        --last;
    }
    return std::string(text.substr(first, last - first));
}

[[nodiscard]] std::string lower(const std::string_view text) {
    std::string result;
    for (const char c : text) {
        result.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
}

[[nodiscard]] bool starts_with_word(const std::string& line,
                                    const std::string_view word) {
    if (line.size() < word.size() ||
        lower(line.substr(0, word.size())) != word) {
        return false;
    }
    return line.size() == word.size() ||
           std::isspace(static_cast<unsigned char>(line[word.size()])) != 0;
}

// Parses the right-hand side of `Name = value` into `property`.
void parse_value(const std::string& text, FrmProperty& property) {
    std::size_t at = 0;
    if (!text.empty() && text.front() == '"') {
        ++at;
        while (at < text.size()) {
            if (text[at] == '"') {
                if (at + 1U < text.size() && text[at + 1U] == '"') {
                    property.value.push_back('"');
                    at += 2U;
                    continue;
                }
                break;
            }
            property.value.push_back(text[at++]);
        }
        property.is_string = true;
        if (at < text.size()) {
            ++at;
        }
        // `"Form1.frx":0042`
        const auto rest = trim(std::string_view(text).substr(at));
        if (!rest.empty() && rest.front() == ':') {
            property.in_resource = true;
            property.resource_file = property.value;
            property.resource_offset =
                std::strtol(rest.c_str() + 1, nullptr, 16);
        }
        return;
    }
    // A number, boolean or enumeration: up to a comment.
    std::string value;
    while (at < text.size() && text[at] != '\'') {
        value.push_back(text[at++]);
    }
    property.value = trim(value);
}

// Splits `Name = value`; returns false when the line has no `=`.
[[nodiscard]] bool split_assignment(const std::string& line, std::string& name,
                                    std::string& value) {
    const auto equals = line.find('=');
    if (equals == std::string::npos) {
        return false;
    }
    name = trim(std::string_view(line).substr(0, equals));
    value = trim(std::string_view(line).substr(equals + 1U));
    return !name.empty();
}

}  // namespace

const FrmProperty* FrmObject::find(const std::string_view property) const {
    const auto wanted = lower(property);
    for (const auto& item : properties) {
        if (lower(item.name) == wanted) {
            return &item;
        }
    }
    return nullptr;
}

FrmFile parse_frm(const std::string_view text) {
    FrmFile result;
    // Split into lines, keeping the original separators for the code part.
    std::vector<std::string> lines;
    {
        std::size_t start = 0;
        while (start <= text.size()) {
            auto end = text.find('\n', start);
            if (end == std::string_view::npos) {
                end = text.size();
            }
            auto line = std::string(text.substr(start, end - start));
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            lines.push_back(std::move(line));
            start = end + 1U;
        }
    }
    std::size_t index = 0;
    const auto fail = [&](const std::string& message) {
        result.ok = false;
        result.error = message;
        result.error_line = index + 1U;
        return result;
    };
    // VERSION and the form block.
    while (index < lines.size() && trim(lines[index]).empty()) {
        ++index;
    }
    if (index < lines.size() &&
        starts_with_word(trim(lines[index]), "version")) {
        result.version = trim(trim(lines[index]).substr(7));
        ++index;
    }
    while (index < lines.size() && trim(lines[index]).empty()) {
        ++index;
    }
    std::vector<FrmObject*> stack;
    std::vector<FrmProperty*> groups;
    bool root_started = false;
    bool root_finished = false;
    std::size_t header_end = index;
    for (; index < lines.size() && !root_finished; ++index) {
        const std::string line = trim(lines[index]);
        header_end = index + 1U;
        if (line.empty()) {
            continue;
        }
        if (!root_started && starts_with_word(line, "object")) {
            // `Object = "{GUID}#2.0#0"; "MSCOMCTL.OCX"`
            std::string name;
            std::string value;
            if (split_assignment(line, name, value)) {
                FrmProperty parsed;
                parse_value(value, parsed);
                const auto first = parsed.value.find('#');
                const auto second = first == std::string::npos
                                        ? first
                                        : parsed.value.find('#', first + 1U);
                if (first != std::string::npos) {
                    result.objects.push_back(parsed.value.substr(0, second));
                }
            }
            continue;
        }
        if (starts_with_word(line, "begin")) {
            // `Begin Type Name`
            const auto rest = trim(line.substr(5));
            const auto space = rest.find_first_of(" \t");
            if (rest.empty() || space == std::string::npos) {
                return fail("expected 'Begin Type Name'");
            }
            FrmObject object;
            object.type = rest.substr(0, space);
            object.name = trim(rest.substr(space));
            if (!root_started) {
                root_started = true;
                result.root = std::move(object);
                stack.push_back(&result.root);
            } else if (stack.empty()) {
                return fail("control outside the form block");
            } else {
                stack.back()->children.push_back(std::move(object));
                stack.push_back(&stack.back()->children.back());
            }
            continue;
        }
        if (starts_with_word(line, "beginproperty")) {
            if (stack.empty()) {
                return fail("BeginProperty outside a form block");
            }
            FrmProperty group;
            group.name = trim(line.substr(13));
            auto& target = groups.empty() ? stack.back()->properties
                                          : groups.back()->nested;
            target.push_back(std::move(group));
            groups.push_back(&target.back());
            continue;
        }
        if (starts_with_word(line, "endproperty")) {
            if (groups.empty()) {
                return fail("EndProperty without BeginProperty");
            }
            groups.pop_back();
            continue;
        }
        if (lower(line) == "end") {
            if (stack.empty()) {
                return fail("End without Begin");
            }
            stack.pop_back();
            if (stack.empty()) {
                root_finished = true;
            }
            continue;
        }
        std::string name;
        std::string value;
        if (!split_assignment(line, name, value)) {
            return fail("expected 'Name = value'");
        }
        if (stack.empty()) {
            return fail("property outside a form block");
        }
        FrmProperty property;
        property.name = name;
        parse_value(value, property);
        auto& target =
            groups.empty() ? stack.back()->properties : groups.back()->nested;
        target.push_back(std::move(property));
    }
    if (!root_started || !root_finished) {
        return fail("missing form block");
    }
    // The code part: blank the header lines, keep the rest.
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i < header_end) {
            // keep the line break only
        } else {
            result.code += lines[i];
        }
        if (i + 1U < lines.size()) {
            result.code += "\n";
        }
    }
    result.name = result.root.name;
    for (std::size_t i = header_end; i < lines.size(); ++i) {
        const auto line = trim(lines[i]);
        if (starts_with_word(line, "attribute")) {
            std::string name;
            std::string value;
            if (split_assignment(line.substr(9), name, value) &&
                lower(name) == "vb_name") {
                FrmProperty parsed;
                parse_value(value, parsed);
                result.name = parsed.value;
            }
        }
    }
    result.ok = true;
    return result;
}

namespace {

void write_property(const FrmProperty& property, const std::size_t depth,
                    std::string& out) {
    const std::string indent(depth * 3U, ' ');
    if (!property.nested.empty()) {
        out += indent + "BeginProperty " + property.name + " \r\n";
        for (const auto& part : property.nested) {
            write_property(part, depth + 1U, out);
        }
        out += indent + "EndProperty\r\n";
        return;
    }
    std::string name = property.name;
    if (name.size() < 16U) {
        name.resize(16U, ' ');
    }
    std::string value;
    if (property.in_resource) {
        char offset[16];
        std::snprintf(offset, sizeof(offset), "%04lX",
                      property.resource_offset);
        value = "\"" + property.resource_file + "\":" + offset;
    } else if (property.is_string) {
        value = "\"";
        for (const char c : property.value) {
            value += c == '"' ? std::string("\"\"") : std::string(1U, c);
        }
        value += "\"";
    } else {
        value = property.value;
    }
    out += indent + name + "=   " + value + "\r\n";
}

void write_object(const FrmObject& object, const std::size_t depth,
                  std::string& out) {
    const std::string indent(depth * 3U, ' ');
    out += indent + "Begin " + object.type + " " + object.name + " \r\n";
    for (const auto& property : object.properties) {
        write_property(property, depth + 1U, out);
    }
    for (const auto& child : object.children) {
        write_object(child, depth + 1U, out);
    }
    out += indent + "End\r\n";
}

}  // namespace

std::string write_frm_header(const FrmFile& form) {
    std::string out = "VERSION " + form.version + "\r\n";
    write_object(form.root, 0, out);
    return out;
}

bool read_frx_record(const std::string& path, const long offset,
                     std::string& bytes) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream || offset < 0) {
        return false;
    }
    const std::string all((std::istreambuf_iterator<char>(stream)),
                          std::istreambuf_iterator<char>());
    const auto start = static_cast<std::size_t>(offset);
    if (start + 4U > all.size()) {
        return false;
    }
    const auto read32 = [&](const std::size_t at) {
        std::uint32_t value = 0;
        for (std::size_t i = 0; i < 4U; ++i) {
            value |= static_cast<std::uint32_t>(
                         static_cast<unsigned char>(all[at + i]))
                     << (8U * i);
        }
        return value;
    };
    std::size_t length = read32(start);
    std::size_t data = start + 4U;
    // A picture record is "lt" (0x746C) followed by the image size.
    if (length >= 8U && data + 8U <= all.size() && read32(data) == 0x746CU) {
        length = read32(data + 4U);
        data += 8U;
    }
    if (data + length > all.size()) {
        return false;
    }
    bytes = all.substr(data, length);
    return true;
}

}  // namespace wfc::ui
