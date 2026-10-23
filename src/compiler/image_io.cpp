#include "image_io.hpp"

#include <cstdint>
#include <cstring>

namespace wfc::compiler {

namespace {

constexpr std::string_view kMagic = "WFCNATIV";
// Changes whenever the layout of an image or the entry table changes.
constexpr std::uint32_t kVersion = 1;

void put32(std::string& out, const std::uint32_t value) {
    out.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

void put64(std::string& out, const std::uint64_t value) {
    out.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

void put_text(std::string& out, const std::string& text) {
    put32(out, static_cast<std::uint32_t>(text.size()));
    out += text;
}

class Reader final {
public:
    explicit Reader(const std::string_view text) : text_(text) {}

    [[nodiscard]] bool u32(std::uint32_t& value) {
        return take(&value, sizeof(value));
    }

    [[nodiscard]] bool u64(std::uint64_t& value) {
        return take(&value, sizeof(value));
    }

    [[nodiscard]] bool text(std::string& value) {
        std::uint32_t size = 0;
        if (!u32(size) || text_.size() - at_ < size) {
            return false;
        }
        value.assign(text_.substr(at_, size));
        at_ += size;
        return true;
    }

    [[nodiscard]] bool bytes(std::uint64_t size,
                             std::vector<std::uint8_t>& value) {
        if (text_.size() - at_ < size) {
            return false;
        }
        value.assign(text_.begin() + static_cast<std::ptrdiff_t>(at_),
                     text_.begin() + static_cast<std::ptrdiff_t>(at_ + size));
        at_ += static_cast<std::size_t>(size);
        return true;
    }

private:
    [[nodiscard]] bool take(void* into, const std::size_t size) {
        if (text_.size() - at_ < size) {
            return false;
        }
        std::memcpy(into, text_.data() + at_, size);
        at_ += size;
        return true;
    }

    std::string_view text_;
    std::size_t at_{};
};

}  // namespace

std::string serialize_image(const NativeImage& image) {
    std::string out(kMagic);
    put32(out, kVersion);
    put32(out, static_cast<std::uint32_t>(image.architecture));
    put32(out, static_cast<std::uint32_t>(kRuntimeApiVersion));
    put64(out, image.table_count);
    put64(out, image.entry);
    put64(out, image.error_flag);
    put32(out, static_cast<std::uint32_t>(image.classes.size()));
    for (const auto& item : image.classes) {
        put_text(out, item.name);
        put64(out, item.record);
    }
    put64(out, image.bytes.size());
    out.append(reinterpret_cast<const char*>(image.bytes.data()),
               image.bytes.size());
    return out;
}

bool deserialize_image(const std::string_view text, NativeImage& image) {
    if (text.substr(0, kMagic.size()) != kMagic) {
        return false;
    }
    Reader reader(text.substr(kMagic.size()));
    std::uint32_t version = 0;
    std::uint32_t architecture = 0;
    std::uint32_t api = 0;
    std::uint64_t table = 0;
    std::uint64_t entry = 0;
    std::uint64_t flag = 0;
    std::uint32_t count = 0;
    if (!reader.u32(version) || version != kVersion ||
        !reader.u32(architecture) || !reader.u32(api) ||
        api != static_cast<std::uint32_t>(kRuntimeApiVersion) ||
        !reader.u64(table) || !reader.u64(entry) || !reader.u64(flag) ||
        !reader.u32(count)) {
        return false;
    }
    NativeImage loaded;
    loaded.architecture = static_cast<Architecture>(architecture);
    loaded.table_count = static_cast<std::size_t>(table);
    loaded.entry = static_cast<std::size_t>(entry);
    loaded.error_flag = static_cast<std::size_t>(flag);
    for (std::uint32_t i = 0; i < count; ++i) {
        ImageClass item;
        std::uint64_t record = 0;
        if (!reader.text(item.name) || !reader.u64(record)) {
            return false;
        }
        item.record = static_cast<std::size_t>(record);
        loaded.classes.push_back(std::move(item));
    }
    std::uint64_t size = 0;
    if (!reader.u64(size) || !reader.bytes(size, loaded.bytes)) {
        return false;
    }
    image = std::move(loaded);
    return true;
}

}  // namespace wfc::compiler
