#include "package.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <system_error>

#include "ui/frm_parser.hpp"

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

// The `Key=Value` lines of a project file, keys lower case, quotes removed.
[[nodiscard]] std::map<std::string, std::string> project_values(
    const fs::path& project) {
    std::map<std::string, std::string> values;
    std::ifstream stream(project);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto equals = line.find('=');
        if (equals == std::string::npos || line.front() == '[') {
            continue;
        }
        std::string key = line.substr(0, equals);
        std::string value = line.substr(equals + 1U);
        std::transform(key.begin(), key.end(), key.begin(),
                       [](const unsigned char c) {
                           return static_cast<char>(std::tolower(c));
                       });
        if (value.size() >= 2U && value.front() == '"' && value.back() == '"') {
            value = value.substr(1U, value.size() - 2U);
        }
        values.emplace(std::move(key), std::move(value));
    }
    return values;
}

// The icon image data of the project's `IconForm` (its `Icon` property, a
// record of the form's .frx file), or empty.
[[nodiscard]] std::string project_icon(
    const fs::path& project, const std::map<std::string, std::string>& values) {
    const auto icon_form = values.find("iconform");
    if (icon_form == values.end()) {
        return {};
    }
    const fs::path directory = project.parent_path();
    std::ifstream vbp(project);
    std::string line;
    while (std::getline(vbp, line)) {
        if (line.size() < 6U || line.compare(0, 5, "Form=") != 0) {
            continue;
        }
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
            line.pop_back();
        }
        const fs::path form_path = directory / line.substr(5);
        std::ifstream form(form_path);
        std::string row;
        bool header_form = false;
        while (std::getline(form, row)) {
            if (!row.empty() && row.back() == '\r') {
                row.pop_back();
            }
            const auto begin = row.find("Begin VB.Form ");
            if (!header_form) {
                if (begin == 0U) {
                    std::string name = row.substr(14);
                    while (!name.empty() && name.back() == ' ') {
                        name.pop_back();
                    }
                    std::string wanted = icon_form->second;
                    const auto lower = [](std::string text) {
                        for (auto& c : text) {
                            c = static_cast<char>(
                                std::tolower(static_cast<unsigned char>(c)));
                        }
                        return text;
                    };
                    if (lower(name) != lower(wanted)) {
                        break;  // another form
                    }
                    header_form = true;
                }
                continue;
            }
            // Properties of the form itself end at its first nested object.
            const auto first = row.find_first_not_of(' ');
            if (first == std::string::npos) {
                continue;
            }
            if (row.compare(first, 6, "Begin ") == 0) {
                return {};
            }
            if (row.compare(first, 4, "Icon") == 0 &&
                row.find('=') != std::string::npos) {
                const auto quote = row.find('"');
                const auto close = row.find('"', quote + 1U);
                const auto colon = row.find(':', close);
                if (quote == std::string::npos || colon == std::string::npos) {
                    return {};
                }
                const std::string frx =
                    row.substr(quote + 1U, close - quote - 1U);
                const long offset =
                    std::strtol(row.c_str() + colon + 1U, nullptr, 16);
                std::string bytes;
                return wfc::ui::read_frx_record((directory / frx).string(),
                                                offset, bytes)
                           ? bytes
                           : std::string{};
            }
        }
    }
    return {};
}

#ifdef _WIN32

// Adds an .ico file's images as RT_ICON resources and a RT_GROUP_ICON.
[[nodiscard]] bool stamp_icon(HANDLE update, const std::string& ico,
                              const WORD language) {
    const auto word = [&](const std::size_t at) {
        return static_cast<std::uint16_t>(
            static_cast<unsigned char>(ico[at]) |
            (static_cast<unsigned char>(ico[at + 1U]) << 8U));
    };
    const auto dword = [&](const std::size_t at) {
        return static_cast<std::uint32_t>(word(at)) |
               (static_cast<std::uint32_t>(word(at + 2U)) << 16U);
    };
    if (ico.size() < 6U || word(0) != 0U || word(2) != 1U) {
        return true;  // not an icon: leave the executable's own
    }
    const std::uint16_t count = word(4);
    if (count == 0U || ico.size() < 6U + count * 16U) {
        return true;
    }
    for (std::uint16_t i = 0; i < count; ++i) {
        const std::size_t entry = 6U + i * 16U;
        const std::uint32_t size = dword(entry + 8U);
        const std::uint32_t offset = dword(entry + 12U);
        if (offset > ico.size() || size > ico.size() - offset) {
            return true;
        }
        if (UpdateResourceW(
                update, MAKEINTRESOURCEW(3), MAKEINTRESOURCEW(i + 1U), language,
                const_cast<char*>(ico.data()) + offset, size) == FALSE) {
            return false;
        }
    }
    // The group lists each image with its resource id instead of its offset.
    std::string group;
    const auto put16 = [&group](const std::uint16_t number) {
        group.push_back(static_cast<char>(number & 0xFFU));
        group.push_back(static_cast<char>(number >> 8U));
    };
    put16(0);
    put16(1);
    put16(count);
    for (std::uint16_t i = 0; i < count; ++i) {
        const std::size_t entry = 6U + i * 16U;
        group.append(ico, entry, 12U);
        put16(static_cast<std::uint16_t>(i + 1U));
    }
    return UpdateResourceW(update, MAKEINTRESOURCEW(14), MAKEINTRESOURCEW(1),
                           language, group.data(),
                           static_cast<DWORD>(group.size())) != FALSE;
}

#endif

#ifdef _WIN32

[[nodiscard]] std::wstring widen(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int length = MultiByteToWideChar(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        result.data(), length);
    return result;
}

// A version-resource block: length, value length, type, key, value, children.
void block(std::string& out, const std::wstring& key, const std::string& value,
           const bool text_value, const std::string& children) {
    std::string body;
    const auto put16 = [](std::string& to, const std::uint16_t number) {
        to.push_back(static_cast<char>(number & 0xFFU));
        to.push_back(static_cast<char>(number >> 8U));
    };
    for (const wchar_t c : key) {
        put16(body, static_cast<std::uint16_t>(c));
    }
    put16(body, 0);
    while ((body.size() + 6U) % 4U != 0U) {
        body.push_back('\0');
    }
    body += value;
    while ((body.size() + 6U) % 4U != 0U) {
        body.push_back('\0');
    }
    body += children;
    const std::size_t whole = body.size() + 6U;
    put16(out, static_cast<std::uint16_t>(whole));
    put16(out, static_cast<std::uint16_t>(text_value ? value.size() / 2U
                                                     : value.size()));
    put16(out, text_value ? 1U : 0U);
    out += body;
}

[[nodiscard]] std::string wide_bytes(const std::string& text) {
    const std::wstring wide = widen(text) + L'\0';
    return std::string(reinterpret_cast<const char*>(wide.data()),
                       wide.size() * sizeof(wchar_t));
}

// Writes the project's version information into the executable's resources
// (RT_VERSION, English US). A project without version keys is left alone.
[[nodiscard]] bool stamp_version(const fs::path& exe, const fs::path& project,
                                 std::string& problem) {
    const auto values = project_values(project);
    const auto find = [&](const char* key) {
        const auto found = values.find(key);
        return found == values.end() ? std::string{} : found->second;
    };
    const auto number = [&](const char* key) {
        return static_cast<std::uint32_t>(std::atol(find(key).c_str()));
    };
    const std::string icon = project_icon(project, values);
    if (find("majorver").empty() && find("versioncompanyname").empty() &&
        find("versionproductname").empty() && find("title").empty() &&
        icon.empty()) {
        return true;
    }
    const std::uint32_t major = number("majorver");
    const std::uint32_t minor = number("minorver");
    const std::uint32_t revision = number("revisionver");
    const std::string dotted = std::to_string(major) + "." +
                               std::to_string(minor) + ".0." +
                               std::to_string(revision);

    std::string strings;
    const auto add = [&](const wchar_t* key, const std::string& text) {
        std::string one;
        block(one, key, wide_bytes(text), true, {});
        while (one.size() % 4U != 0U) {
            one.push_back('\0');
        }
        strings += one;
    };
    const std::string title =
        find("title").empty() ? exe.stem().string() : find("title");
    add(L"CompanyName", find("versioncompanyname"));
    add(L"FileDescription", find("versionfiledescription").empty()
                                ? title
                                : find("versionfiledescription"));
    add(L"FileVersion", dotted);
    add(L"InternalName", exe.stem().string());
    add(L"LegalCopyright", find("versionlegalcopyright"));
    add(L"OriginalFilename", exe.filename().string());
    add(L"ProductName", find("versionproductname").empty()
                            ? title
                            : find("versionproductname"));
    add(L"ProductVersion", dotted);
    add(L"Comments", find("versioncomments"));

    std::string table;
    block(table, L"040904B0", {}, true, strings);
    while (table.size() % 4U != 0U) {
        table.push_back('\0');
    }
    std::string string_info;
    block(string_info, L"StringFileInfo", {}, true, table);
    while (string_info.size() % 4U != 0U) {
        string_info.push_back('\0');
    }
    std::string translation(4, '\0');
    translation[0] = '\x09';
    translation[1] = '\x04';
    translation[2] = '\xB0';
    translation[3] = '\x04';
    std::string var;
    block(var, L"Translation", translation, false, {});
    while (var.size() % 4U != 0U) {
        var.push_back('\0');
    }
    std::string var_info;
    block(var_info, L"VarFileInfo", {}, true, var);

    VS_FIXEDFILEINFO fixed{};
    fixed.dwSignature = 0xFEEF04BDU;
    fixed.dwStrucVersion = 0x00010000U;
    fixed.dwFileVersionMS = (major << 16U) | (minor & 0xFFFFU);
    fixed.dwFileVersionLS = revision & 0xFFFFU;
    fixed.dwProductVersionMS = fixed.dwFileVersionMS;
    fixed.dwProductVersionLS = fixed.dwFileVersionLS;
    fixed.dwFileFlagsMask = 0x3FU;
    fixed.dwFileOS = VOS_NT_WINDOWS32;
    fixed.dwFileType = VFT_APP;
    std::string root;
    block(root, L"VS_VERSION_INFO",
          std::string(reinterpret_cast<const char*>(&fixed), sizeof(fixed)),
          false, string_info + var_info);

    const std::wstring path = exe.wstring();
    HANDLE update = BeginUpdateResourceW(path.c_str(), FALSE);
    if (update == nullptr) {
        problem = "cannot open " + exe.string() + " to write version resources";
        return false;
    }
    // Replace the language of any existing version resource too.
    const WORD language = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    const BOOL written =
        UpdateResourceW(update, MAKEINTRESOURCEW(16), MAKEINTRESOURCEW(1),
                        language, root.data(), static_cast<DWORD>(root.size()));
    const bool iconed =
        written == FALSE || icon.empty() || stamp_icon(update, icon, language);
    if (EndUpdateResourceW(update, written && iconed ? FALSE : TRUE) == FALSE ||
        written == FALSE || !iconed) {
        problem = "cannot write version resources to " + exe.string();
        return false;
    }
    return true;
}

#else

[[nodiscard]] bool stamp_version(const fs::path&, const fs::path&,
                                 std::string&) {
    return true;
}

#endif

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
    {
        std::ofstream out(target, std::ios::binary | std::ios::trunc);
        out.write(runtime.data(), static_cast<std::streamsize>(runtime.size()));
        out.close();
        if (!out.good()) {
            problem = "cannot write " + target.string();
            return false;
        }
    }
    // The resources go in first: rewriting them would drop appended data.
    if (!stamp_version(target, project, problem)) {
        return false;
    }
    std::ofstream out(target, std::ios::binary | std::ios::app);
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
