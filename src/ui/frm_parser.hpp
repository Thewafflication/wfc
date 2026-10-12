// Parser for the text part of Visual Basic 6 form files (.frm) and reader for
// the binary resource files (.frx) they reference. Internal to the WFC
// runtime; not part of the public API.

#ifndef WFC_UI_FRM_PARSER_HPP
#define WFC_UI_FRM_PARSER_HPP

#include <string>
#include <string_view>
#include <vector>

namespace wfc::ui {

// One `Name = value` line of a form description, or a `BeginProperty` group.
struct FrmProperty {
    std::string name;
    // The value as written, without a trailing comment. A string value is
    // stored unquoted with doubled quotes collapsed and `is_string` set.
    std::string value;
    bool is_string{false};
    // `"Form1.frx":0042` -- a reference into the form's .frx file.
    bool in_resource{false};
    std::string resource_file;
    long resource_offset{0};
    // The members of a `BeginProperty` ... `EndProperty` group (a Font).
    std::vector<FrmProperty> nested;
};

// A `Begin Type Name` ... `End` block: the form itself, a control, or a menu.
struct FrmObject {
    std::string type;
    std::string name;
    std::vector<FrmProperty> properties;
    std::vector<FrmObject> children;

    // The property `name` (case-insensitive), or nullptr.
    [[nodiscard]] const FrmProperty* find(std::string_view property) const;
};

struct FrmFile {
    bool ok{false};
    std::string error;
    std::size_t error_line{0};
    std::string version;
    // `Object = "{GUID}#major.minor#flags"; "file"` lines: the control
    // libraries the form uses, as "{GUID}#major.minor".
    std::vector<std::string> objects;
    FrmObject root;
    // The `Attribute VB_Name = "..."` value, or the root object's name.
    std::string name;
    // The code part with every header line blanked, so line numbers match the
    // file.
    std::string code;
};

[[nodiscard]] FrmFile parse_frm(std::string_view text);

// The header of a form file (VERSION through the closing End) written the way
// VB6 writes it: three-space indentation, `Name = value` aligned at column 24,
// strings quoted with doubled quotes, resource references as `"file":0000`.
// Parsing the result gives an equivalent FrmFile (comments are not kept).
[[nodiscard]] std::string write_frm_header(const FrmFile& form);

// The bytes of the .frx record at `offset`, or false when the file or record
// is missing or malformed. Text records return their text; picture records
// return the image data after the record header.
[[nodiscard]] bool read_frx_record(const std::string& path, long offset,
                                   std::string& bytes);

}  // namespace wfc::ui

#endif
