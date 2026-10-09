// Three-valued logic, UTF-16 string views, identifiers, and VBA constants.
// Internal to the WFC evaluator; not part of the public API.
// Split out of src/evaluator.cpp; see src/interpreter/README.md.

#ifndef WFC_INTERPRETER_VB_TEXT_HPP
#define WFC_INTERPRETER_VB_TEXT_HPP

#include "vb_date.hpp"

namespace wfc::detail {

// Kleene three-valued logic for the logical operators: nullopt represents
// Null (unknown); a definite Boolean is represented as its own value. These
// tables are verified against the local VB6 6.00.8176 reference for And, Or,
// and Not; Xor/Eqv/Imp are derived algebraically from those verified
// primitives (Eqv = Not(Xor(a,b)), Imp = Not(a) Or b), which VBA's own
// documentation defines them as.
[[nodiscard]] inline std::optional<bool> ternary_and(
    const std::optional<bool> left, const std::optional<bool> right) noexcept {
    if (left == false || right == false) {
        return false;
    }
    if (left.has_value() && right.has_value()) {
        return true;
    }
    return std::nullopt;
}

[[nodiscard]] inline std::optional<bool> ternary_or(
    const std::optional<bool> left, const std::optional<bool> right) noexcept {
    if (left == true || right == true) {
        return true;
    }
    if (left.has_value() && right.has_value()) {
        return false;
    }
    return std::nullopt;
}

[[nodiscard]] inline std::optional<bool> ternary_not(
    const std::optional<bool> value) noexcept {
    if (!value.has_value()) {
        return std::nullopt;
    }
    return !*value;
}

[[nodiscard]] inline std::optional<bool> ternary_xor(
    const std::optional<bool> left, const std::optional<bool> right) noexcept {
    if (left.has_value() && right.has_value()) {
        return *left != *right;
    }
    return std::nullopt;
}

[[nodiscard]] inline std::optional<bool> ternary_eqv(
    const std::optional<bool> left, const std::optional<bool> right) noexcept {
    return ternary_not(ternary_xor(left, right));
}

[[nodiscard]] inline std::optional<bool> ternary_imp(
    const std::optional<bool> left, const std::optional<bool> right) noexcept {
    return ternary_or(ternary_not(left), right);
}

[[nodiscard]] inline char ascii_lower(const char character) noexcept {
    if (character >= 'A' && character <= 'Z') {
        return static_cast<char>(character + ('a' - 'A'));
    }
    return character;
}

[[nodiscard]] inline char ascii_upper(const char character) noexcept {
    if (character >= 'a' && character <= 'z') {
        return static_cast<char>(character - ('a' - 'A'));
    }
    return character;
}

// String model: a VB String is a sequence of UTF-16 code units. WFC stores it
// as UTF-8 (so I/O and source text need no conversion) and counts, slices and
// searches by UTF-16 unit. Bytes that are not valid UTF-8 count as one unit
// each (their Latin-1 value). ASCII-only text takes the plain byte paths.
[[nodiscard]] inline bool is_ascii_text(const std::string_view text) noexcept {
    for (const char character : text) {
        if (static_cast<unsigned char>(character) >= 0x80U) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] inline bool is_byte_half(const char16_t unit) noexcept {
    return unit >= 0xF700U && unit <= 0xF7FFU;
}

[[nodiscard]] inline std::u16string to_utf16_units(
    const std::string_view text) {
    std::u16string units;
    units.reserve(text.size());
    std::size_t i = 0;
    while (i < text.size()) {
        const auto lead = static_cast<unsigned char>(text[i]);
        std::size_t length = lead < 0x80U                      ? 1U
                             : (lead >= 0xF0U && lead < 0xF8U) ? 4U
                             : (lead >= 0xE0U && lead < 0xF0U) ? 3U
                             : (lead >= 0xC2U && lead < 0xE0U) ? 2U
                                                               : 0U;
        bool valid = length != 0 && i + length <= text.size();
        std::uint32_t code = length == 1U   ? lead
                             : length == 2U ? (lead & 0x1FU)
                             : length == 3U ? (lead & 0x0FU)
                                            : (lead & 0x07U);
        for (std::size_t k = 1; valid && k < length; ++k) {
            const auto next = static_cast<unsigned char>(text[i + k]);
            valid = (next & 0xC0U) == 0x80U;
            code = (code << 6) | (next & 0x3FU);
        }
        if (!valid) {
            units.push_back(static_cast<char16_t>(lead));
            ++i;
            continue;
        }
        i += length;
        if (code >= 0x10000U) {
            code -= 0x10000U;
            units.push_back(static_cast<char16_t>(0xD800U + (code >> 10U)));
            units.push_back(static_cast<char16_t>(0xDC00U + (code & 0x3FFU)));
        } else {
            units.push_back(static_cast<char16_t>(code));
        }
    }
    return units;
}

inline void append_utf8_unit(std::string& out, const std::uint32_t code) {
    if (code < 0x80U) {
        out.push_back(static_cast<char>(code));
    } else if (code < 0x800U) {
        out.push_back(static_cast<char>(0xC0U | (code >> 6U)));
        out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
    } else if (code < 0x10000U) {
        out.push_back(static_cast<char>(0xE0U | (code >> 12U)));
        out.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
    } else {
        out.push_back(static_cast<char>(0xF0U | (code >> 18U)));
        out.push_back(static_cast<char>(0x80U | ((code >> 12U) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
    }
}

[[nodiscard]] inline std::string from_utf16_units(
    const std::u16string_view units) {
    std::string out;
    out.reserve(units.size());
    for (std::size_t i = 0; i < units.size(); ++i) {
        std::uint32_t code = units[i];
        if (code >= 0xD800U && code < 0xDC00U && i + 1U < units.size() &&
            units[i + 1U] >= 0xDC00U && units[i + 1U] < 0xE000U) {
            code = 0x10000U + ((code - 0xD800U) << 10U) +
                   (units[i + 1U] - 0xDC00U);
            ++i;
        }
        append_utf8_unit(out, code);
    }
    return out;
}

// Number of UTF-16 units in `text`.
[[nodiscard]] inline std::size_t utf16_length(const std::string& text) {
    return is_ascii_text(text) ? text.size() : to_utf16_units(text).size();
}

// Pads with spaces or truncates `text` to exactly `units` UTF-16 units (a
// fixed-length String).
inline void fit_to_units(std::string& text, const std::size_t units) {
    if (is_ascii_text(text)) {
        text.resize(units, ' ');
        return;
    }
    auto wide = to_utf16_units(text);
    wide.resize(units, u' ');
    text = from_utf16_units(wide);
}

// The Unicode character a Windows-1252 byte stands for (what Chr() yields).
[[nodiscard]] inline std::uint32_t ansi_to_unicode(
    const unsigned code) noexcept {
    static constexpr std::uint16_t table[32] = {
        0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
        0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
        0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
        0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178};
    return code >= 0x80U && code <= 0x9FU ? table[code - 0x80U] : code;
}

// File text is ANSI (Windows-1252) on disk; Strings are Unicode in memory.
[[nodiscard]] inline std::string ansi_bytes_to_text(const std::string& bytes) {
    if (is_ascii_text(bytes)) {
        return bytes;
    }
    std::string text;
    text.reserve(bytes.size() + 8U);
    for (const char byte : bytes) {
        append_utf8_unit(text,
                         ansi_to_unicode(static_cast<unsigned char>(byte)));
    }
    return text;
}

// The Windows-1252 byte for a character, or '?' when it has none.
[[nodiscard]] inline unsigned unicode_to_ansi(
    const std::uint32_t code) noexcept {
    if (code < 0x80U || (code >= 0xA0U && code <= 0xFFU)) {
        return code;
    }
    for (unsigned byte = 0x80U; byte <= 0x9FU; ++byte) {
        if (ansi_to_unicode(byte) == code) {
            return byte;
        }
    }
    return static_cast<unsigned>('?');
}

[[nodiscard]] inline std::string text_to_ansi_bytes(const std::string& text) {
    if (is_ascii_text(text)) {
        return text;
    }
    std::string bytes;
    for (const char16_t unit : to_utf16_units(text)) {
        bytes.push_back(static_cast<char>(
            is_byte_half(unit) ? (unit & 0xFFU) : unicode_to_ansi(unit)));
    }
    return bytes;
}

// The B functions (LenB, AscB, ChrB, LeftB, MidB, ...) see a String as its
// UTF-16LE bytes. A lone byte -- which ChrB and an odd-length slice produce --
// is held as a private-use "half unit" U+F700+byte, so that `ChrB(65) &
// ChrB(0)` can rejoin into the unit "A".
[[nodiscard]] inline std::string string_to_bytes(const std::string& text) {
    std::string bytes;
    bytes.reserve(text.size() * 2U);
    for (const char16_t unit : to_utf16_units(text)) {
        if (is_byte_half(unit)) {
            bytes.push_back(static_cast<char>(unit & 0xFFU));
        } else {
            bytes.push_back(static_cast<char>(unit & 0xFFU));
            bytes.push_back(static_cast<char>(unit >> 8U));
        }
    }
    return bytes;
}

[[nodiscard]] inline std::string bytes_to_string(const std::string& bytes) {
    std::u16string units;
    for (std::size_t i = 0; i < bytes.size(); i += 2U) {
        const auto low = static_cast<unsigned char>(bytes[i]);
        if (i + 1U < bytes.size()) {
            units.push_back(static_cast<char16_t>(
                low | (static_cast<unsigned char>(bytes[i + 1U]) << 8U)));
        } else {
            units.push_back(static_cast<char16_t>(0xF700U | low));
        }
    }
    return from_utf16_units(units);
}

// After `text` was extended past `left_size` bytes: two half units that now
// touch merge into one real unit.
inline void merge_byte_halves(std::string& text, const std::size_t left_size) {
    if (left_size < 3U || left_size + 3U > text.size()) {
        return;
    }
    const auto half_at = [&](const std::size_t at) -> int {
        if (static_cast<unsigned char>(text[at]) != 0xEFU) {
            return -1;
        }
        const auto second = static_cast<unsigned char>(text[at + 1U]);
        if (second < 0x9CU || second > 0x9FU) {
            return -1;
        }
        const unsigned code =
            0xF000U | ((second & 0x3FU) << 6U) |
            (static_cast<unsigned char>(text[at + 2U]) & 0x3FU);
        return is_byte_half(static_cast<char16_t>(code))
                   ? static_cast<int>(code & 0xFFU)
                   : -1;
    };
    const int low = half_at(left_size - 3U);
    const int high = half_at(left_size);
    if (low < 0 || high < 0) {
        return;
    }
    std::string merged;
    append_utf8_unit(merged, static_cast<std::uint32_t>(low | (high << 8)));
    text.replace(left_size - 3U, 6U, merged);
}

// Half units print as the raw byte they hold.
[[nodiscard]] inline std::string show_byte_halves(const std::string& text) {
    if (text.find('\xEF') == std::string::npos) {
        return text;
    }
    std::string shown;
    for (const char16_t unit : to_utf16_units(text)) {
        if (is_byte_half(unit)) {
            shown.push_back(static_cast<char>(unit & 0xFFU));
        } else {
            append_utf8_unit(shown, unit);
        }
    }
    return shown;
}

// Latin Extended-A letters pair upper/lower as even/odd, except two runs where
// the odd one is the capital.
[[nodiscard]] inline bool latin_ext_a_letter(const char16_t c) noexcept {
    return c >= 0x100U && c <= 0x17FU && c != 0x130U && c != 0x131U &&
           c != 0x138U && c != 0x149U && c != 0x178U && c != 0x17FU;
}

[[nodiscard]] inline bool latin_ext_a_odd_upper(const char16_t c) noexcept {
    return (c >= 0x139U && c <= 0x148U) || (c >= 0x179U && c <= 0x17EU);
}

[[nodiscard]] inline char16_t unit_to_lower(const char16_t c) noexcept {
    if (c >= u'A' && c <= u'Z') {
        return static_cast<char16_t>(c + 32);
    }
    if (c >= 0xC0U && c <= 0xDEU && c != 0xD7U) {
        return static_cast<char16_t>(c + 32);
    }
    if (c == 0x178U) {
        return 0xFFU;
    }
    if (c >= 0x391U && c <= 0x3A9U && c != 0x3A2U) {
        return static_cast<char16_t>(c + 32);
    }
    if (c >= 0x410U && c <= 0x42FU) {
        return static_cast<char16_t>(c + 32);
    }
    if (c >= 0x400U && c <= 0x40FU) {
        return static_cast<char16_t>(c + 80);
    }
    if (latin_ext_a_letter(c) &&
        (c % 2U == (latin_ext_a_odd_upper(c) ? 1U : 0U))) {
        return static_cast<char16_t>(c + 1);
    }
    return c;
}

[[nodiscard]] inline char16_t unit_to_upper(const char16_t c) noexcept {
    if (c >= u'a' && c <= u'z') {
        return static_cast<char16_t>(c - 32);
    }
    if (c >= 0xE0U && c <= 0xFEU && c != 0xF7U) {
        return static_cast<char16_t>(c - 32);
    }
    if (c == 0xFFU) {
        return 0x178U;
    }
    if (c >= 0x3B1U && c <= 0x3C9U && c != 0x3C2U) {
        return static_cast<char16_t>(c - 32);
    }
    if (c >= 0x430U && c <= 0x44FU) {
        return static_cast<char16_t>(c - 32);
    }
    if (c >= 0x450U && c <= 0x45FU) {
        return static_cast<char16_t>(c - 80);
    }
    if (latin_ext_a_letter(c) &&
        (c % 2U == (latin_ext_a_odd_upper(c) ? 0U : 1U))) {
        return static_cast<char16_t>(c - 1);
    }
    return c;
}

// Case-fold `text` for a text comparison (ASCII bytes, or full units when
// non-ASCII).
[[nodiscard]] inline std::string fold_case(const std::string& text) {
    if (is_ascii_text(text)) {
        std::string folded = text;
        for (char& character : folded) {
            character = ascii_lower(character);
        }
        return folded;
    }
    auto units = to_utf16_units(text);
    for (auto& unit : units) {
        unit = unit_to_lower(unit);
    }
    return from_utf16_units(units);
}

[[nodiscard]] inline bool is_identifier_start(const char character) noexcept {
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z');
}

[[nodiscard]] inline bool is_identifier_part(const char character) noexcept {
    return is_identifier_start(character) ||
           (character >= '0' && character <= '9') || character == '_';
}

// Source-visible VBA constant enumerations. Each name resolves to the value in
// its `REQ-008x`/`REQ-0089` type-library contract and is a reserved identifier
// so source cannot shadow it. Members are exposed for source compatibility;
// they carry no runtime behavior beyond their integer value.
[[nodiscard]] inline std::optional<Integer> vba_constant_value(
    const std::string_view identifier) {
    static const std::unordered_map<std::string_view, Integer> table = {
        // VbCompareMethod (REQ-0089)
        {"vbbinarycompare", 0},
        {"vbtextcompare", 1},
        {"vbdatabasecompare", 2},
        {"vbusecompareoption", -1},
        // ColorConstants (RGB values as Long)
        {"vbblack", 0},
        {"vbred", 255},
        {"vbgreen", 65280},
        {"vbyellow", 65535},
        {"vbblue", 16711680},
        {"vbmagenta", 16711935},
        {"vbcyan", 16776960},
        {"vbwhite", 16777215},
        // Common VBRUN constants (key codes, shift masks, mouse buttons, Show
        // modes)
        {"vbkeyreturn", 13},
        {"vbkeyescape", 27},
        {"vbkeyspace", 32},
        {"vbkeyback", 8},
        {"vbkeytab", 9},
        {"vbkeyleft", 37},
        {"vbkeyup", 38},
        {"vbkeyright", 39},
        {"vbkeydown", 40},
        {"vbkeydelete", 46},
        {"vbkeyinsert", 45},
        {"vbkeyhome", 36},
        {"vbkeyend", 35},
        {"vbkeypageup", 33},
        {"vbkeypagedown", 34},
        {"vbkeyshift", 16},
        {"vbkeycontrol", 17},
        {"vbkeymenu", 18},
        {"vbkeyf1", 112},
        {"vbkeyf2", 113},
        {"vbkeyf3", 114},
        {"vbkeyf4", 115},
        {"vbkeyf5", 116},
        {"vbkeyf6", 117},
        {"vbkeyf7", 118},
        {"vbkeyf8", 119},
        {"vbkeyf9", 120},
        {"vbkeyf10", 121},
        {"vbkeyf11", 122},
        {"vbkeyf12", 123},
        {"vbshiftmask", 1},
        {"vbctrlmask", 2},
        {"vbaltmask", 4},
        {"vbleftbutton", 1},
        {"vbrightbutton", 2},
        {"vbmiddlebutton", 4},
        {"vbmodeless", 0},
        {"vbmodal", 1},
        // VbVarType (REQ-0080)
        {"vbempty", 0},
        {"vbnull", 1},
        {"vbinteger", 2},
        {"vblong", 3},
        {"vbsingle", 4},
        {"vbdouble", 5},
        {"vbcurrency", 6},
        {"vbdate", 7},
        {"vbstring", 8},
        {"vbobject", 9},
        {"vberror", 10},
        {"vbboolean", 11},
        {"vbvariant", 12},
        {"vbdataobject", 13},
        {"vbdecimal", 14},
        {"vbbyte", 17},
        {"vbuserdefinedtype", 36},
        {"vbarray", 8192},
        // VbStrConv (REQ-0084)
        {"vbuppercase", 1},
        {"vblowercase", 2},
        {"vbpropercase", 3},
        {"vbwide", 4},
        {"vbnarrow", 8},
        {"vbkatakana", 16},
        {"vbhiragana", 32},
        {"vbunicode", 64},
        {"vbfromunicode", 128},
        // VbTriState (REQ-0092)
        {"vbusedefault", -2},
        {"vbtrue", -1},
        {"vbfalse", 0},
        // VbCallType (REQ-0093)
        {"vbmethod", 1},
        {"vbget", 2},
        {"vblet", 4},
        {"vbset", 8},
        // VbFileAttribute (REQ-0083)
        {"vbnormal", 0},
        {"vbreadonly", 1},
        {"vbhidden", 2},
        {"vbsystem", 4},
        {"vbvolume", 8},
        {"vbdirectory", 16},
        {"vbarchive", 32},
        {"vbalias", 64},
        // VbMsgBoxResult (REQ-0082)
        {"vbok", 1},
        {"vbcancel", 2},
        {"vbabort", 3},
        {"vbretry", 4},
        {"vbignore", 5},
        {"vbyes", 6},
        {"vbno", 7},
        // VbDayOfWeek (REQ-0085)
        {"vbusesystemdayofweek", 0},
        {"vbsunday", 1},
        {"vbmonday", 2},
        {"vbtuesday", 3},
        {"vbwednesday", 4},
        {"vbthursday", 5},
        {"vbfriday", 6},
        {"vbsaturday", 7},
        // VbMsgBoxStyle (REQ-0081)
        {"vbokonly", 0},
        {"vbokcancel", 1},
        {"vbabortretryignore", 2},
        {"vbyesnocancel", 3},
        {"vbyesno", 4},
        {"vbretrycancel", 5},
        {"vbcritical", 16},
        {"vbquestion", 32},
        {"vbexclamation", 48},
        {"vbinformation", 64},
        {"vbdefaultbutton1", 0},
        {"vbdefaultbutton2", 256},
        {"vbdefaultbutton3", 512},
        {"vbdefaultbutton4", 768},
        {"vbapplicationmodal", 0},
        {"vbsystemmodal", 4096},
        {"vbmsgboxhelpbutton", 16384},
        {"vbmsgboxright", 524288},
        {"vbmsgboxrtlreading", 1048576},
        {"vbmsgboxsetforeground", 65536},
        // VbAppWinStyle (REQ-0088)
        {"vbhide", 0},
        {"vbnormalfocus", 1},
        {"vbminimizedfocus", 2},
        {"vbmaximizedfocus", 3},
        {"vbnormalnofocus", 4},
        {"vbminimizednofocus", 6},
        // VbFirstWeekOfYear (REQ-0086)
        {"vbusesystem", 0},
        {"vbfirstjan1", 1},
        {"vbfirstfourdays", 2},
        {"vbfirstfullweek", 3},
        // VbCalendar (REQ-0090)
        {"vbcalgreg", 0},
        {"vbcalhijri", 1},
        // VbDateTimeFormat (REQ-0091)
        {"vbgeneraldate", 0},
        {"vblongdate", 1},
        {"vbshortdate", 2},
        {"vblongtime", 3},
        {"vbshorttime", 4},
        // VbIMEStatus (REQ-0087)
        {"vbimenoop", 0},
        {"vbimemodenocontrol", 0},
        {"vbimeon", 1},
        {"vbimemodeon", 1},
        {"vbimeoff", 2},
        {"vbimemodeoff", 2},
        {"vbimedisable", 3},
        {"vbimemodedisable", 3},
        {"vbimehiragana", 4},
        {"vbimemodehiragana", 4},
        {"vbimekatakanadbl", 5},
        {"vbimemodekatakana", 5},
        {"vbimekatakanasng", 6},
        {"vbimemodekatakanahalf", 6},
        {"vbimealphadbl", 7},
        {"vbimemodealphafull", 7},
        {"vbimealphasng", 8},
        {"vbimemodealpha", 8},
        {"vbimemodehangulfull", 9},
        {"vbimemodehangul", 10},
        // General constants (REQ-0094), integer member
        {"vbobjecterror", -2147221504},
    };
    const auto entry = table.find(identifier);
    if (entry == table.end()) {
        // vbKeyA..vbKeyZ and vbKey0..vbKey9 are their ASCII codes.
        if (identifier.size() == 6U && identifier.substr(0, 5U) == "vbkey") {
            const char last = identifier[5];
            if (last >= 'a' && last <= 'z') {
                return static_cast<Integer>(last - 'a' + 'A');
            }
            if (last >= '0' && last <= '9') {
                return static_cast<Integer>(last);
            }
        }
        return std::nullopt;
    }
    return entry->second;
}

// Source-visible VBA string constants (REQ-0094). Each resolves to its exact
// stored bytes and is reserved so source cannot shadow it.
[[nodiscard]] inline std::optional<std::string> vba_string_constant(
    const std::string_view identifier) {
    static const std::unordered_map<std::string_view, std::string> table = {
        {"vbnullstring", std::string{}},
        {"vbnullchar", std::string(1, '\0')},
        {"vbcrlf", "\r\n"},
        {"vbnewline", "\r\n"},
        {"vbcr", "\r"},
        {"vblf", "\n"},
        {"vbback", "\b"},
        {"vbformfeed", "\f"},
        {"vbtab", "\t"},
        {"vbverticaltab", "\v"},
    };
    const auto entry = table.find(identifier);
    if (entry == table.end()) {
        return std::nullopt;
    }
    return entry->second;
}

[[nodiscard]] inline bool is_reserved_identifier(
    const std::string_view identifier) noexcept {
    return identifier == "and" || identifier == "as" ||
           identifier == "boolean" || identifier == "dim" ||
           identifier == "do" || identifier == "each" || identifier == "eqv" ||
           identifier == "erase" || identifier == "exit" ||
           identifier == "false" || identifier == "for" ||
           identifier == "else" || identifier == "elseif" ||
           identifier == "if" || identifier == "imp" || identifier == "in" ||
           identifier == "is" || identifier == "let" || identifier == "long" ||
           identifier == "case" || identifier == "const" ||
           identifier == "loop" || identifier == "mod" ||
           identifier == "next" || identifier == "not" ||
           identifier == "null" || identifier == "empty" ||
           identifier == "nothing" || identifier == "object" ||
           identifier == "set" || identifier == "sub" ||
           identifier == "function" || identifier == "call" ||
           identifier == "new" || identifier == "property" ||
           identifier == "get" || identifier == "me" || identifier == "byval" ||
           identifier == "byref" || identifier == "optional" ||
           identifier == "paramarray" || identifier == "static" ||
           identifier == "private" || identifier == "public" ||
           identifier == "option" || identifier == "or" ||
           identifier == "preserve" || identifier == "print" ||
           identifier == "randomize" || identifier == "redim" ||
           identifier == "rem" || identifier == "select" ||
           identifier == "string" || identifier == "then" ||
           identifier == "explicit" || identifier == "typeof" ||
           identifier == "erl" || identifier == "type" || identifier == "err" ||
           identifier == "goto" || identifier == "resume" ||
           identifier == "enum" || identifier == "step" || identifier == "to" ||
           identifier == "true" || identifier == "until" ||
           identifier == "wend" || identifier == "while" ||
           identifier == "with" || identifier == "xor" ||
           vba_constant_value(identifier).has_value() ||
           vba_string_constant(identifier).has_value();
}

// Class members may reuse a few statement keywords (`Public Sub Print`).
[[nodiscard]] inline bool is_reserved_member_name(
    const std::string_view identifier) noexcept {
    return identifier != "print" && identifier != "randomize" &&
           is_reserved_identifier(identifier);
}

}  // namespace wfc::detail

#endif  // WFC_INTERPRETER_VB_TEXT_HPP
