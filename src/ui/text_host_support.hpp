// Helpers shared by the parts of the text host (text_host.cpp,
// text_host_dialogs.cpp). Internal to the WFC runtime.

#ifndef WFC_UI_TEXT_HOST_SUPPORT_HPP
#define WFC_UI_TEXT_HOST_SUPPORT_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

#include "../interpreter/vb_text.hpp"
#include "wfc/brun71/forms.hpp"

namespace wfc::ui::text {

using brun71::Region;

// A character cell in twips (VB6's ScaleMode 4, vbCharacters).
constexpr double kCellWidth = 120.0;
constexpr double kCellHeight = 240.0;
// How long a wait for a key lasts before the timers are checked again.
constexpr int kWaitSlice = 50;

constexpr std::uint8_t kDisabledAttribute = 0x78;  // dark grey on grey
constexpr std::uint8_t kAccessAttribute = 0x7F;    // white on grey

inline int cells(const double twips, const double size) {
    return static_cast<int>(std::lround(twips / size));
}

inline Region intersect(const Region& a, const Region& b) {
    const int top = std::max(a.top, b.top);
    const int left = std::max(a.left, b.left);
    const int bottom = std::min(a.bottom(), b.bottom());
    const int right = std::min(a.right(), b.right());
    return Region{top, left, std::max(bottom - top + 1, 0),
                  std::max(right - left + 1, 0)};
}

inline bool inside(const Region& region, const int row, const int column) {
    return row >= region.top && row <= region.bottom() &&
           column >= region.left && column <= region.right();
}

// The virtual-key code of a BIOS scan code of an extended key, with the
// shift mask (1 Shift, 2 Ctrl, 4 Alt), or 0.
inline long extended_key(const int code, long& shift) {
    shift = 0;
    switch (code) {
        case 15:
            shift = 1;
            return 9;  // Shift+Tab
        case 71:
            return 36;  // Home
        case 72:
            return 38;  // Up
        case 73:
            return 33;  // Page Up
        case 75:
            return 37;  // Left
        case 77:
            return 39;  // Right
        case 79:
            return 35;  // End
        case 80:
            return 40;  // Down
        case 81:
            return 34;  // Page Down
        case 82:
            return 45;  // Insert
        case 83:
            return 46;  // Delete
        case 133:
            return 122;  // F11
        case 134:
            return 123;  // F12
        default:
            break;
    }
    if (code >= 59 && code <= 68) {
        return 112 + (code - 59);  // F1-F10
    }
    // Alt and a letter: the scan codes of the three letter rows.
    static constexpr const char* kRows[] = {"QWERTYUIOP", "ASDFGHJKL",
                                            "ZXCVBNM"};
    static constexpr int kFirst[] = {16, 30, 44};
    for (int row = 0; row < 3; ++row) {
        const std::string_view letters = kRows[row];
        if (code >= kFirst[row] &&
            code < kFirst[row] + static_cast<int>(letters.size())) {
            shift = 4;
            return letters[static_cast<std::size_t>(code - kFirst[row])];
        }
    }
    if (code >= 120 && code <= 129) {
        shift = 4;
        return code == 129 ? '0' : '1' + (code - 120);  // Alt and a digit
    }
    return 0;
}

// The characters of UTF-8 text, and back.
inline std::u32string decode(const std::string& text) {
    std::u32string characters;
    for (std::size_t i = 0; i < text.size();) {
        const auto lead = static_cast<unsigned char>(text[i]);
        const std::size_t length = lead < 0x80U   ? 1U
                                   : lead < 0xE0U ? 2U
                                   : lead < 0xF0U ? 3U
                                                  : 4U;
        char32_t code = length == 1U   ? lead
                        : length == 2U ? lead & 0x1FU
                        : length == 3U ? lead & 0x0FU
                                       : lead & 0x07U;
        for (std::size_t k = 1; k < length && i + k < text.size(); ++k) {
            code = (code << 6U) |
                   (static_cast<unsigned char>(text[i + k]) & 0x3FU);
        }
        characters.push_back(code);
        i += length;
    }
    return characters;
}

inline std::string encode(const std::u32string& characters) {
    std::string text;
    for (const char32_t code : characters) {
        wfc::detail::append_utf8_unit(text, code);
    }
    return text;
}

// What a key did to a line of text being edited.
enum class Edit { ignored, moved, changed };

// Edits one line of text at its caret: a character (`ascii` 32 and up, an
// ANSI code) goes in, Backspace (8) deletes before the caret, Delete (code
// 46) after it, and Left, Right, Home and End (codes 37, 39, 36, 35) move
// it. `code` is 0 for a character, `ascii` 0 for a key.
inline Edit edit_line(std::u32string& characters, std::size_t& caret,
                      const long code, const long ascii) {
    caret = std::min(caret, characters.size());
    if (ascii == 8) {
        if (caret == 0) {
            return Edit::moved;
        }
        characters.erase(--caret, 1);
        return Edit::changed;
    }
    if (ascii >= 32) {
        characters.insert(caret++, 1,
                          static_cast<char32_t>(wfc::detail::ansi_to_unicode(
                              static_cast<unsigned>(ascii))));
        return Edit::changed;
    }
    if (ascii != 0) {
        return Edit::ignored;  // Tab, Enter, Esc, the Ctrl keys
    }
    switch (code) {
        case 37:
            caret -= caret > 0 ? 1U : 0U;
            return Edit::moved;
        case 39:
            caret += caret < characters.size() ? 1U : 0U;
            return Edit::moved;
        case 36:
            caret = 0;
            return Edit::moved;
        case 35:
            caret = characters.size();
            return Edit::moved;
        case 46:
            if (caret < characters.size()) {
                characters.erase(caret, 1);
                return Edit::changed;
            }
            return Edit::moved;
        default:
            return Edit::ignored;
    }
}

}  // namespace wfc::ui::text

#endif
