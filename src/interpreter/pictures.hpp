// Picture objects (StdPicture) behind LoadPicture and the Picture properties
// of forms and controls (REQ-0291). Internal to the WFC evaluator; not part
// of the public API.

#ifndef WFC_INTERPRETER_PICTURES_HPP
#define WFC_INTERPRETER_PICTURES_HPP

#include <string>

#include "interpreter.hpp"

namespace wfc::detail {

enum class PictureType : int {
    none = 0,
    bitmap = 1,
    metafile = 2,
    icon = 3,
    enhanced_metafile = 4,
};

// What the header of an image file says about it.
struct PictureInfo {
    PictureType type{PictureType::none};
    long width{0};   // HIMETRIC (0.01 mm)
    long height{0};  // HIMETRIC
};

// Reads the type and natural size of image data (BMP, ICO/CUR, GIF, JPEG,
// PNG, WMF, EMF); false when the data is not a picture.
[[nodiscard]] bool read_picture_header(const std::string& bytes,
                                       PictureInfo& info);

// A StdPicture: immutable image bytes plus their header information. An empty
// picture (no bytes) has handle 0 and type 0.
class PictureObject final : public NativeObject {
public:
    PictureObject() = default;
    PictureObject(std::string image_bytes, const PictureInfo& header)
        : bytes(std::move(image_bytes)), info(header) {}

    [[nodiscard]] std::string type_name() const override { return "Picture"; }
    [[nodiscard]] bool has_member(const std::string& member) const override;
    [[nodiscard]] bool get(Interpreter& interpreter, const std::string& member,
                           std::vector<CallArgument>& arguments, Value& result,
                           NativeFailure& failure) override;
    [[nodiscard]] bool put(Interpreter& interpreter, const std::string& member,
                           std::vector<CallArgument>& arguments,
                           const Value& value, NativeFailure& failure) override;

    std::string bytes;
    PictureInfo info;
    // The picture's handle for programs: stable per object, 0 when empty.
    long handle{0};
    // The host's id for the decoded image (0 until first drawn).
    long host_id{0};
    // IPicture.KeepOriginalFormat and the palette handle (always 0 here).
    bool keep_original{true};
    long palette{0};
};

// The picture an object value holds, or nullptr.
[[nodiscard]] PictureObject* picture_of(const Value& value);

// Wraps a picture as an object value.
[[nodiscard]] Value make_picture_value(std::string bytes,
                                       const PictureInfo& info);
[[nodiscard]] Value make_empty_picture_value();

}  // namespace wfc::detail

#endif
