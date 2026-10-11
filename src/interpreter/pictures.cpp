// Picture objects (StdPicture), header parsing, LoadPicture and SavePicture
// (REQ-0291). Internal to the WFC evaluator; not part of the public API.

#include "pictures.hpp"

#include <atomic>
#include <cstdint>
#include <fstream>
#include <iterator>

namespace wfc::detail {
namespace {

[[nodiscard]] std::uint32_t le32(const std::string& data,
                                 const std::size_t at) {
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < 4U && at + i < data.size(); ++i) {
        value |=
            static_cast<std::uint32_t>(static_cast<unsigned char>(data[at + i]))
            << (8U * i);
    }
    return value;
}

[[nodiscard]] std::uint32_t le16(const std::string& data,
                                 const std::size_t at) {
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < 2U && at + i < data.size(); ++i) {
        value |=
            static_cast<std::uint32_t>(static_cast<unsigned char>(data[at + i]))
            << (8U * i);
    }
    return value;
}

[[nodiscard]] std::uint32_t be16(const std::string& data,
                                 const std::size_t at) {
    return at + 1U < data.size() ? (static_cast<std::uint32_t>(
                                        static_cast<unsigned char>(data[at]))
                                    << 8U) |
                                       static_cast<unsigned char>(data[at + 1U])
                                 : 0U;
}

[[nodiscard]] std::uint32_t be32(const std::string& data,
                                 const std::size_t at) {
    return (be16(data, at) << 16U) | be16(data, at + 2U);
}

// Pixels at the screen's 96 dpi as HIMETRIC.
[[nodiscard]] long himetric(const std::uint32_t pixels) {
    return static_cast<long>(
        (static_cast<std::uint64_t>(pixels) * 2540U + 48U) / 96U);
}

[[nodiscard]] unsigned char byte_at(const std::string& data,
                                    const std::size_t at) {
    return static_cast<unsigned char>(data[at]);
}

}  // namespace

bool read_picture_header(const std::string& bytes, PictureInfo& info) {
    info = PictureInfo{};
    if (bytes.size() < 14U) {
        return false;
    }
    if (bytes[0] == 'B' && bytes[1] == 'M') {
        // BITMAPFILEHEADER, then a BITMAPCOREHEADER or BITMAPINFOHEADER.
        const std::uint32_t header_size = le32(bytes, 14);
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        if (header_size == 12U) {
            width = le16(bytes, 18);
            height = le16(bytes, 20);
        } else if (header_size >= 40U && bytes.size() >= 26U) {
            width = le32(bytes, 18);
            const auto signed_height =
                static_cast<std::int32_t>(le32(bytes, 22));
            height = static_cast<std::uint32_t>(
                signed_height < 0 ? -signed_height : signed_height);
        } else {
            return false;
        }
        info =
            PictureInfo{PictureType::bitmap, himetric(width), himetric(height)};
        return width != 0U && height != 0U;
    }
    if (byte_at(bytes, 0) == 0 && byte_at(bytes, 1) == 0 &&
        (byte_at(bytes, 2) == 1 || byte_at(bytes, 2) == 2) &&
        byte_at(bytes, 3) == 0 && le16(bytes, 4) >= 1U && bytes.size() >= 22U) {
        const std::uint32_t width =
            byte_at(bytes, 6) == 0 ? 256U : byte_at(bytes, 6);
        const std::uint32_t height =
            byte_at(bytes, 7) == 0 ? 256U : byte_at(bytes, 7);
        info =
            PictureInfo{PictureType::icon, himetric(width), himetric(height)};
        return true;
    }
    if (bytes.compare(0, 4, "GIF8") == 0) {
        info = PictureInfo{PictureType::bitmap, himetric(le16(bytes, 6)),
                           himetric(le16(bytes, 8))};
        return true;
    }
    if (byte_at(bytes, 0) == 0xFFU && byte_at(bytes, 1) == 0xD8U) {
        std::size_t at = 2;
        while (at + 9U < bytes.size()) {
            if (byte_at(bytes, at) != 0xFFU) {
                ++at;
                continue;
            }
            const auto marker = byte_at(bytes, at + 1U);
            if (marker == 0xD8U || marker == 0x01U ||
                (marker >= 0xD0U && marker <= 0xD7U) || marker == 0xFFU) {
                ++at;
                continue;
            }
            if (marker >= 0xC0U && marker <= 0xCFU && marker != 0xC4U &&
                marker != 0xC8U && marker != 0xCCU) {
                info = PictureInfo{PictureType::bitmap,
                                   himetric(be16(bytes, at + 7U)),
                                   himetric(be16(bytes, at + 5U))};
                return true;
            }
            at += 2U + be16(bytes, at + 2U);
        }
        return false;
    }
    if (byte_at(bytes, 0) == 0x89U && bytes.compare(1, 3, "PNG") == 0 &&
        bytes.size() >= 24U) {
        info = PictureInfo{PictureType::bitmap, himetric(be32(bytes, 16)),
                           himetric(be32(bytes, 20))};
        return true;
    }
    if (le32(bytes, 0) == 0x9AC6CDD7U && bytes.size() >= 22U) {
        // Placeable WMF: bounding box in metafile units, units per inch.
        const auto left = static_cast<std::int16_t>(le16(bytes, 6));
        const auto top = static_cast<std::int16_t>(le16(bytes, 8));
        const auto right = static_cast<std::int16_t>(le16(bytes, 10));
        const auto bottom = static_cast<std::int16_t>(le16(bytes, 12));
        const long inch =
            le16(bytes, 14) != 0U ? static_cast<long>(le16(bytes, 14)) : 1440L;
        info = PictureInfo{PictureType::metafile,
                           static_cast<long>((right - left) * 2540L / inch),
                           static_cast<long>((bottom - top) * 2540L / inch)};
        return true;
    }
    if (bytes.size() >= 88U && le32(bytes, 0) == 1U &&
        le32(bytes, 40) == 0x464D4520U) {
        // EMF: the frame rectangle is in 0.01 mm.
        info = PictureInfo{
            PictureType::enhanced_metafile,
            static_cast<long>(static_cast<std::int32_t>(le32(bytes, 32)) -
                              static_cast<std::int32_t>(le32(bytes, 24))),
            static_cast<long>(static_cast<std::int32_t>(le32(bytes, 36)) -
                              static_cast<std::int32_t>(le32(bytes, 28)))};
        return true;
    }
    return false;
}

bool PictureObject::has_member(const std::string& member) const {
    return member == "handle" || member == "type" || member == "width" ||
           member == "height" || member == "hpal" || member == "render" ||
           member == "_default";
}

bool PictureObject::get(Interpreter&, const std::string& member,
                        std::vector<CallArgument>&, Value& result,
                        NativeFailure&) {
    if (member == "handle" || member == "_default") {
        result = Value{static_cast<Integer>(handle)};
    } else if (member == "type") {
        result = Value{static_cast<Int16>(info.type)};
    } else if (member == "width") {
        result = Value{static_cast<Integer>(info.width)};
    } else if (member == "height") {
        result = Value{static_cast<Integer>(info.height)};
    } else if (member == "hpal") {
        result = Value{Integer{0}};
    } else if (member == "render") {
        result = Value{Empty{}};  // drawing happens through PaintPicture
    } else {
        return false;
    }
    return true;
}

bool PictureObject::put(Interpreter&, const std::string& member,
                        std::vector<CallArgument>&, const Value&,
                        NativeFailure& failure) {
    if (!has_member(member)) {
        return false;
    }
    failure = NativeFailure{383, "Property is read-only"};
    return true;
}

PictureObject* picture_of(const Value& value) {
    const auto* holder = std::get_if<ObjectInstance>(&value);
    if (holder == nullptr || holder->data == nullptr) {
        return nullptr;
    }
    return dynamic_cast<PictureObject*>(holder->data->native.get());
}

namespace {

std::atomic<long> next_handle{0x1001};

}  // namespace

Value make_picture_value(std::string bytes, const PictureInfo& info) {
    auto object = std::make_shared<PictureObject>(std::move(bytes), info);
    object->handle = next_handle.fetch_add(4);
    auto data = std::make_shared<InstanceData>();
    data->class_name = "wfcnative";
    data->native = std::move(object);
    return Value{ObjectInstance{std::move(data)}};
}

Value make_empty_picture_value() {
    auto data = std::make_shared<InstanceData>();
    data->class_name = "wfcnative";
    data->native = std::make_shared<PictureObject>();
    return Value{ObjectInstance{std::move(data)}};
}

std::optional<Value> Interpreter::load_picture_builtin(
    const std::vector<Value>& arguments, const std::size_t offset) {
    if (arguments.empty() || arguments.size() > 5U) {
        set_error("WFC0072", "function received the wrong number of arguments",
                  offset);
        return std::nullopt;
    }
    if (!execute_) {
        return make_empty_picture_value();
    }
    if (std::holds_alternative<Empty>(arguments[0])) {
        return make_empty_picture_value();
    }
    const auto* path = std::get_if<std::string>(&arguments[0]);
    if (path == nullptr) {
        set_error("WFC0073", "Type mismatch", offset);
        return std::nullopt;
    }
    if (path->empty()) {
        return make_empty_picture_value();  // LoadPicture("") clears
    }
    std::ifstream stream(*path, std::ios::binary);
    if (!stream) {
        static_cast<void>(raise_runtime(53, "File not found", offset));
        return std::nullopt;
    }
    std::string bytes((std::istreambuf_iterator<char>(stream)),
                      std::istreambuf_iterator<char>());
    PictureInfo info;
    if (!read_picture_header(bytes, info)) {
        static_cast<void>(raise_runtime(481, "Invalid picture", offset));
        return std::nullopt;
    }
    return make_picture_value(std::move(bytes), info);
}

bool Interpreter::parse_savepicture_statement(const std::size_t offset) {
    skip_horizontal_whitespace();
    auto picture = parse_expression();
    if (!picture.has_value()) {
        return false;
    }
    skip_horizontal_whitespace();
    if (!consume(',')) {
        set_error("WFC0005", "expected ','", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    auto path = parse_expression();
    if (!path.has_value()) {
        return false;
    }
    if (!execute_) {
        return true;
    }
    const PictureObject* object = picture_of(*picture);
    const auto* name = std::get_if<std::string>(&*path);
    if (object == nullptr || name == nullptr) {
        set_error("WFC0073", "Type mismatch", offset);
        return false;
    }
    std::ofstream out(*name, std::ios::binary | std::ios::trunc);
    if (!out) {
        static_cast<void>(raise_runtime(75, "Path/File access error", offset));
        return false;
    }
    out.write(object->bytes.data(),
              static_cast<std::streamsize>(object->bytes.size()));
    return true;
}

}  // namespace wfc::detail
