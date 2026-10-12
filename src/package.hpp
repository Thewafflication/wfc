#pragma once

// Packaged programs (ADR-0007, REQ-0303): a copy of the wfc image followed by a
// payload holding the project's files and a fixed trailer.

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace wfc::package {

// The path of the running executable, or empty when unknown.
[[nodiscard]] std::filesystem::path running_image();

// Writes `output`: the image at `image` (without any payload it already
// carries), the files of the directory of `project`, and the trailer.
// `options` are the command-line options the program starts with (`--gui`).
// Returns false and sets `problem` on failure.
[[nodiscard]] bool build(const std::filesystem::path& image,
                         const std::filesystem::path& project,
                         const std::filesystem::path& output,
                         const std::vector<std::string>& options,
                         std::string& problem);

// When `image` carries a payload, extracts it below the temporary directory and
// returns the command line to run (the project file followed by the stored
// options); an empty optional when it carries none. `problem` is set when it
// does but cannot be extracted.
[[nodiscard]] std::optional<std::vector<std::string>> unpack(
    const std::filesystem::path& image, std::string& problem);

}  // namespace wfc::package
