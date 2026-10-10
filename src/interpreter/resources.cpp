// Interpreter: LoadResString / LoadResData over the project's compiled .res
// file (REQ-0283). Internal to the WFC evaluator; not part of the public API.

#include <fstream>
#include <iterator>

#include "interpreter.hpp"

namespace wfc::detail {
namespace {

constexpr std::uint32_t kStringTable = 6;

[[nodiscard]] std::uint32_t read32(const std::string& bytes,
                                   const std::size_t at) {
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < 4U && at + i < bytes.size(); ++i) {
        value |= static_cast<std::uint32_t>(
                     static_cast<unsigned char>(bytes[at + i]))
                 << (8U * i);
    }
    return value;
}

[[nodiscard]] std::uint16_t read16(const std::string& bytes,
                                   const std::size_t at) {
    return static_cast<std::uint16_t>(
        static_cast<unsigned char>(bytes[at]) |
        (static_cast<unsigned char>(bytes[at + 1U]) << 8U));
}

[[nodiscard]] std::string utf16_to_utf8(const std::u16string& units) {
    std::string text;
    for (std::size_t i = 0; i < units.size(); ++i) {
        std::uint32_t code = units[i];
        if (code >= 0xD800U && code < 0xDC00U && i + 1U < units.size()) {
            code = 0x10000U + ((code - 0xD800U) << 10U) +
                   (units[i + 1U] - 0xDC00U);
            ++i;
        }
        if (code < 0x80U) {
            text.push_back(static_cast<char>(code));
        } else if (code < 0x800U) {
            text.push_back(static_cast<char>(0xC0U | (code >> 6U)));
            text.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
        } else if (code < 0x10000U) {
            text.push_back(static_cast<char>(0xE0U | (code >> 12U)));
            text.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3FU)));
            text.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
        } else {
            text.push_back(static_cast<char>(0xF0U | (code >> 18U)));
            text.push_back(static_cast<char>(0x80U | ((code >> 12U) & 0x3FU)));
            text.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3FU)));
            text.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
        }
    }
    return text;
}

[[nodiscard]] std::string lower_text(const std::string& text) {
    std::string result;
    for (const char c : text) {
        result.push_back(ascii_lower(c));
    }
    return result;
}

}  // namespace

bool Interpreter::load_resources() {
    if (resources_loaded_) {
        return !resources_.empty();
    }
    resources_loaded_ = true;
    if (resource_file_.empty()) {
        return false;
    }
    std::ifstream stream(resource_file_, std::ios::binary);
    if (!stream) {
        return false;
    }
    const std::string bytes((std::istreambuf_iterator<char>(stream)),
                            std::istreambuf_iterator<char>());
    // A 32-bit .res file is a sequence of entries: DataSize, HeaderSize,
    // TYPE, NAME (each 0xFFFF + id or a NUL-terminated UTF-16 string, padded
    // to 4 bytes), then 16 more header bytes, then DataSize bytes of data
    // padded to 4 bytes.
    std::size_t at = 0;
    const auto read_id = [&](std::uint32_t& id, std::string& text) {
        text.clear();
        id = 0;
        if (at + 2U > bytes.size()) {
            return false;
        }
        if (read16(bytes, at) == 0xFFFFU) {
            if (at + 4U > bytes.size()) {
                return false;
            }
            id = read16(bytes, at + 2U);
            at += 4U;
            return true;
        }
        std::u16string units;
        while (at + 2U <= bytes.size()) {
            const auto unit = read16(bytes, at);
            at += 2U;
            if (unit == 0U) {
                break;
            }
            units.push_back(static_cast<char16_t>(unit));
        }
        text = utf16_to_utf8(units);
        return true;
    };
    while (at + 8U <= bytes.size()) {
        const std::size_t entry_start = at;
        const std::uint32_t data_size = read32(bytes, at);
        const std::uint32_t header_size = read32(bytes, at + 4U);
        if (header_size < 16U || entry_start + header_size > bytes.size()) {
            break;
        }
        at += 8U;
        ResourceEntry entry;
        if (!read_id(entry.type_id, entry.type_name)) {
            break;
        }
        at = (at + 3U) & ~static_cast<std::size_t>(3U);
        if (!read_id(entry.name_id, entry.name_text)) {
            break;
        }
        at = (at + 3U) & ~static_cast<std::size_t>(3U);
        if (at + 16U > bytes.size()) {
            break;
        }
        entry.language = read16(bytes, at + 6U);
        const std::size_t data_start = entry_start + header_size;
        if (data_start + data_size > bytes.size()) {
            break;
        }
        entry.data = bytes.substr(data_start, data_size);
        if (data_size != 0U || entry.type_id != 0U) {
            resources_.push_back(std::move(entry));
        }
        at = (data_start + data_size + 3U) & ~static_cast<std::size_t>(3U);
    }
    return !resources_.empty();
}

std::optional<Value> Interpreter::load_resource_builtin(
    const std::string_view name, const std::vector<Value>& arguments,
    const std::size_t offset) {
    const bool is_string = name == "loadresstring";
    if (arguments.size() != (is_string ? 1U : 2U)) {
        set_error("WFC0072", "function received the wrong number of arguments",
                  offset);
        return std::nullopt;
    }
    if (!execute_) {
        return is_string ? Value{std::string{}} : Value{Empty{}};
    }
    const auto not_found = [&](const std::string& what) {
        static_cast<void>(raise_runtime(
            326, "Resource with identifier '" + what + "' not found", offset));
        return std::nullopt;
    };
    const auto identifier_of = [&](const Value& value, std::uint32_t& id,
                                   std::string& text) {
        id = 0;
        text.clear();
        if (const auto* name_text = std::get_if<std::string>(&value)) {
            text = lower_text(*name_text);
            return true;
        }
        if (const auto whole = whole_value(value)) {
            id = static_cast<std::uint32_t>(*whole);
            return true;
        }
        return false;
    };
    std::uint32_t id = 0;
    std::string id_text;
    if (!identifier_of(arguments[0], id, id_text)) {
        set_error("WFC0073", "Type mismatch", offset);
        return std::nullopt;
    }
    const std::string shown = id_text.empty() ? std::to_string(id) : id_text;
    if (!load_resources()) {
        return not_found(shown);
    }
    if (is_string) {
        // String ids live 16 to a block; block number = id / 16 + 1.
        const std::uint32_t block = id / 16U + 1U;
        for (const auto& entry : resources_) {
            if (entry.type_id != kStringTable || !entry.type_name.empty() ||
                entry.name_id != block) {
                continue;
            }
            std::size_t at = 0;
            for (std::uint32_t slot = 0; slot < 16U; ++slot) {
                if (at + 2U > entry.data.size()) {
                    break;
                }
                const std::size_t length = read16(entry.data, at);
                at += 2U;
                if (at + length * 2U > entry.data.size()) {
                    break;
                }
                if (slot == id % 16U && length != 0U) {
                    std::u16string units;
                    for (std::size_t k = 0; k < length; ++k) {
                        units.push_back(static_cast<char16_t>(
                            read16(entry.data, at + k * 2U)));
                    }
                    return Value{utf16_to_utf8(units)};
                }
                at += length * 2U;
            }
        }
        return not_found(shown);
    }
    std::uint32_t type_id = 0;
    std::string type_text;
    if (!identifier_of(arguments[1], type_id, type_text)) {
        set_error("WFC0073", "Type mismatch", offset);
        return std::nullopt;
    }
    for (const auto& entry : resources_) {
        const bool type_match =
            type_text.empty()
                ? (entry.type_name.empty() && entry.type_id == type_id)
                : lower_text(entry.type_name) == type_text;
        const bool name_match =
            id_text.empty() ? (entry.name_text.empty() && entry.name_id == id)
                            : lower_text(entry.name_text) == id_text;
        if (!type_match || !name_match) {
            continue;
        }
        ArrayValue bytes{};
        bytes.element_type_index = Value{Byte{}}.index();
        bytes.lower_bound = 0;
        bytes.is_dynamic = true;
        for (const char c : entry.data) {
            bytes.elements.push_back(Value{static_cast<Byte>(c)});
        }
        return Value{std::move(bytes)};
    }
    return not_found(shown);
}

}  // namespace wfc::detail
