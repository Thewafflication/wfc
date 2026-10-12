#include "package.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <system_error>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace wfc::package {

namespace {

namespace fs = std::filesystem;

constexpr std::array<char, 8> kFooterMagic{'W', 'F', 'C', 'P',
                                           'K', 'G', '0', '1'};
constexpr std::size_t kFooterSize = kFooterMagic.size() + sizeof(std::uint64_t);
constexpr std::uint32_t kPayloadVersion = 1;
constexpr std::uint64_t kLimit = 512ULL * 1024ULL * 1024ULL;

void put32(std::string& out, const std::uint32_t value) {
    for (unsigned shift = 0; shift < 32U; shift += 8U) {
        out.push_back(static_cast<char>((value >> shift) & 0xFFU));
    }
}

void put64(std::string& out, const std::uint64_t value) {
    for (unsigned shift = 0; shift < 64U; shift += 8U) {
        out.push_back(static_cast<char>((value >> shift) & 0xFFU));
    }
}

void put_text(std::string& out, const std::string& text) {
    put32(out, static_cast<std::uint32_t>(text.size()));
    out += text;
}

// A cursor over the payload; every read reports failure instead of overrunning.
class Reader {
public:
    explicit Reader(const std::string& bytes) : bytes_(bytes) {}

    [[nodiscard]] bool u32(std::uint32_t& value) {
        std::uint64_t wide = 0;
        if (!number(4, wide)) {
            return false;
        }
        value = static_cast<std::uint32_t>(wide);
        return true;
    }

    [[nodiscard]] bool u64(std::uint64_t& value) { return number(8, value); }

    [[nodiscard]] bool text(std::string& value) {
        std::uint32_t length = 0;
        if (!u32(length) || length > bytes_.size() - at_) {
            return false;
        }
        value.assign(bytes_, at_, length);
        at_ += length;
        return true;
    }

    [[nodiscard]] bool bytes(const std::uint64_t length, std::string& value) {
        if (length > bytes_.size() - at_) {
            return false;
        }
        value.assign(bytes_, at_, static_cast<std::size_t>(length));
        at_ += static_cast<std::size_t>(length);
        return true;
    }

private:
    [[nodiscard]] bool number(const std::size_t width, std::uint64_t& value) {
        if (width > bytes_.size() - at_) {
            return false;
        }
        value = 0;
        for (std::size_t i = 0; i < width; ++i) {
            value |= static_cast<std::uint64_t>(
                         static_cast<unsigned char>(bytes_[at_ + i]))
                     << (8U * i);
        }
        at_ += width;
        return true;
    }

    const std::string& bytes_;
    std::size_t at_{0};
};

[[nodiscard]] std::string slurp(const fs::path& path, bool& ok) {
    std::ifstream stream(path, std::ios::binary);
    ok = stream.good();
    return ok ? std::string(std::istreambuf_iterator<char>(stream),
                            std::istreambuf_iterator<char>())
              : std::string{};
}

[[nodiscard]] std::uint64_t fnv1a(const std::string& bytes) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const char c : bytes) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

// The size of the payload an image carries (0 when none) and its image length.
struct Located {
    std::uint64_t payload{0};
    std::uint64_t image{0};
};

[[nodiscard]] Located locate(const fs::path& image) {
    std::ifstream stream(image, std::ios::binary | std::ios::ate);
    if (!stream.good()) {
        return {};
    }
    const auto total = static_cast<std::uint64_t>(stream.tellg());
    if (total < kFooterSize) {
        return {0, total};
    }
    std::array<char, kFooterSize> footer{};
    stream.seekg(static_cast<std::streamoff>(total - kFooterSize));
    stream.read(footer.data(), static_cast<std::streamsize>(footer.size()));
    if (!stream.good() ||
        !std::equal(kFooterMagic.begin(), kFooterMagic.end(), footer.begin())) {
        return {0, total};
    }
    std::uint64_t size = 0;
    for (std::size_t i = 0; i < sizeof(size); ++i) {
        size |= static_cast<std::uint64_t>(
                    static_cast<unsigned char>(footer[kFooterMagic.size() + i]))
                << (8U * i);
    }
    if (size > total - kFooterSize) {
        return {0, total};
    }
    return {size, total - kFooterSize - size};
}

[[nodiscard]] bool skipped(const fs::path& relative) {
    for (const auto& part : relative.parent_path()) {
        if (part == ".git") {
            return true;
        }
    }
    std::string extension = relative.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](const unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });
    return extension == ".exe" || extension == ".dll" || extension == ".pdb" ||
           extension == ".obj";
}

[[nodiscard]] bool safe_relative(const std::string& name) {
    const fs::path path(name);
    if (name.empty() || path.is_absolute() || path.has_root_name()) {
        return false;
    }
    return std::none_of(path.begin(), path.end(),
                        [](const fs::path& part) { return part == ".."; });
}

}  // namespace

fs::path running_image() {
#ifdef _WIN32
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                            static_cast<DWORD>(buffer.size()));
    if (length == 0U || length >= buffer.size()) {
        return {};
    }
    buffer.resize(length);
    return fs::path(buffer);
#else
    std::error_code error;
    const fs::path link = fs::read_symlink("/proc/self/exe", error);
    return error ? fs::path{} : link;
#endif
}

bool build(const fs::path& image, const fs::path& project,
           const fs::path& output, const std::vector<std::string>& options,
           std::string& problem) {
    std::error_code error;
    if (!fs::is_regular_file(project, error)) {
        problem = "project file not found: " + project.string();
        return false;
    }
    const fs::path directory = fs::absolute(project, error).parent_path();
    const fs::path target = fs::absolute(output, error);

    std::string payload;
    put32(payload, kPayloadVersion);
    put32(payload, static_cast<std::uint32_t>(options.size()));
    for (const auto& option : options) {
        put_text(payload, option);
    }
    put_text(payload, project.filename().generic_string());

    std::vector<fs::path> files;
    for (fs::recursive_directory_iterator it(directory, error), end;
         !error && it != end; it.increment(error)) {
        if (!it->is_regular_file(error)) {
            continue;
        }
        const fs::path relative = fs::relative(it->path(), directory, error);
        const bool same = fs::equivalent(it->path(), target, error);
        error.clear();  // the output may not exist yet
        if (skipped(relative) || same) {
            continue;
        }
        files.push_back(relative);
    }
    if (error) {
        problem = "cannot read the project directory: " + error.message();
        return false;
    }
    std::sort(files.begin(), files.end());
    put32(payload, static_cast<std::uint32_t>(files.size()));
    for (const auto& relative : files) {
        bool ok = false;
        const std::string content = slurp(directory / relative, ok);
        if (!ok) {
            problem = "cannot read " + relative.string();
            return false;
        }
        put_text(payload, relative.generic_string());
        put64(payload, content.size());
        payload += content;
        if (payload.size() > kLimit) {
            problem = "the project is larger than 512 MB";
            return false;
        }
    }

    const Located located = locate(image);
    std::ifstream source(image, std::ios::binary);
    if (!source.good()) {
        problem = "cannot read the runtime image " + image.string();
        return false;
    }
    std::string runtime(static_cast<std::size_t>(located.image), '\0');
    source.read(runtime.data(), static_cast<std::streamsize>(runtime.size()));
    if (!source.good()) {
        problem = "cannot read the runtime image " + image.string();
        return false;
    }

    std::string footer(kFooterMagic.begin(), kFooterMagic.end());
    put64(footer, payload.size());
    std::ofstream out(target, std::ios::binary | std::ios::trunc);
    out.write(runtime.data(), static_cast<std::streamsize>(runtime.size()));
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    out.write(footer.data(), static_cast<std::streamsize>(footer.size()));
    out.close();
    if (!out.good()) {
        problem = "cannot write " + target.string();
        return false;
    }
    return true;
}

std::optional<std::vector<std::string>> unpack(const fs::path& image,
                                               std::string& problem) {
    const Located located = locate(image);
    if (located.payload == 0U) {
        return std::nullopt;
    }
    std::ifstream stream(image, std::ios::binary);
    stream.seekg(static_cast<std::streamoff>(located.image));
    std::string payload(static_cast<std::size_t>(located.payload), '\0');
    stream.read(payload.data(), static_cast<std::streamsize>(payload.size()));
    if (!stream.good()) {
        problem = "the packaged program is damaged";
        return std::vector<std::string>{};
    }

    Reader reader(payload);
    std::uint32_t version = 0;
    std::uint32_t option_count = 0;
    std::vector<std::string> command;
    std::string project;
    std::uint32_t file_count = 0;
    bool good = reader.u32(version) && version == kPayloadVersion &&
                reader.u32(option_count);
    std::vector<std::string> options;
    for (std::uint32_t i = 0; good && i < option_count; ++i) {
        std::string option;
        good = reader.text(option);
        options.push_back(option);
    }
    good = good && reader.text(project) && safe_relative(project) &&
           reader.u32(file_count);

    char name[17];
    std::snprintf(name, sizeof(name), "%016llx",
                  static_cast<unsigned long long>(fnv1a(payload)));
    std::error_code error;
    const fs::path root =
        fs::temp_directory_path(error) / (std::string("wfc-pkg-") + name);
    for (std::uint32_t i = 0; good && i < file_count; ++i) {
        std::string relative;
        std::uint64_t size = 0;
        std::string content;
        good = reader.text(relative) && safe_relative(relative) &&
               reader.u64(size) && reader.bytes(size, content);
        if (!good) {
            break;
        }
        const fs::path path = root / fs::path(relative);
        bool exists = false;
        if (fs::is_regular_file(path, error) &&
            slurp(path, exists) == content) {
            continue;  // already extracted and intact
        }
        fs::create_directories(path.parent_path(), error);
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
        out.close();
        if (!out.good()) {
            problem = "cannot extract " + path.string();
            return std::vector<std::string>{};
        }
    }
    if (!good) {
        problem = "the packaged program is damaged";
        return std::vector<std::string>{};
    }
    command.push_back((root / fs::path(project)).string());
    command.insert(command.end(), options.begin(), options.end());
    return command;
}

}  // namespace wfc::package
