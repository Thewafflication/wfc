// Interpreter: Format and numeric rendering (fixed, scientific, and custom pictures).
// Internal to the WFC evaluator; not part of the public API.
// Split out of src/evaluator.cpp; see src/interpreter/README.md.

#include "interpreter.hpp"

namespace wfc::detail {

std::string Interpreter::render_currency(const std::int64_t scaled) {
    const bool negative = scaled < 0;
    const std::uint64_t magnitude = negative
        ? (~static_cast<std::uint64_t>(scaled) + 1ULL)
        : static_cast<std::uint64_t>(scaled);
    const std::uint64_t whole_part = magnitude / 10000ULL;
    const std::uint64_t fraction_part = magnitude % 10000ULL;
    std::string result = std::to_string(whole_part);
    if (fraction_part != 0ULL) {
        std::string fraction = std::to_string(fraction_part);
        fraction.insert(fraction.begin(), 4U - fraction.size(), '0');
        while (fraction.back() == '0') {
            fraction.pop_back();
        }
        result += '.';
        result += fraction;
    }
    if (negative) {
        result.insert(result.begin(), '-');
    }
    return result;
}

std::string Interpreter::fixed_half_up(const double magnitude, const int places) {
    if (magnitude == 0.0) {
        return places > 0 ? "0." + std::string(static_cast<std::size_t>(places), '0') : "0";
    }
    char buffer[64];
    const auto converted = std::to_chars(
        buffer, buffer + sizeof(buffer), magnitude, std::chars_format::scientific, 14);
    const std::string text(buffer, converted.ptr);
    const auto e_position = text.find('e');
    std::string digits;
    for (std::size_t i = 0; i < e_position; ++i) {
        if (text[i] != '.') digits.push_back(text[i]);
    }
    const int exponent = std::atoi(text.c_str() + e_position + 1);
    const int point = exponent + 1;  // digits before the decimal point
    std::string integer_part;
    std::string fraction_part;
    if (point <= 0) {
        integer_part = "0";
        fraction_part = std::string(static_cast<std::size_t>(-point), '0') + digits;
    } else if (static_cast<std::size_t>(point) >= digits.size()) {
        integer_part = digits + std::string(static_cast<std::size_t>(point) - digits.size(), '0');
    } else {
        integer_part = digits.substr(0, static_cast<std::size_t>(point));
        fraction_part = digits.substr(static_cast<std::size_t>(point));
    }
    const auto wanted = static_cast<std::size_t>(places);
    bool round_up = fraction_part.size() > wanted && fraction_part[wanted] >= '5';
    fraction_part.resize(wanted, '0');
    std::string all = integer_part + fraction_part;
    if (round_up) {
        std::size_t i = all.size();
        while (i > 0) {
            --i;
            if (all[i] == '9') {
                all[i] = '0';
            } else {
                ++all[i];
                round_up = false;
                break;
            }
        }
        if (round_up) all.insert(all.begin(), '1');
    }
    const std::size_t integer_size = all.size() - wanted;
    std::string result = all.substr(0, integer_size);
    if (wanted > 0) result += "." + all.substr(integer_size);
    return result;
}

std::string Interpreter::render_fixed_style(
    const double value,
    const bool grouping) {
    const bool negative = value < 0.0;
    const double magnitude = std::fabs(value);
    std::string digits = fixed_half_up(magnitude, 2);
    if (grouping) {
        const auto dot = digits.find('.');
        std::string integer_part = digits.substr(0, dot);
        const std::string fraction_part = digits.substr(dot);
        for (int i = static_cast<int>(integer_part.size()) - 3; i > 0; i -= 3) {
            integer_part.insert(static_cast<std::size_t>(i), ",");
        }
        digits = integer_part + fraction_part;
    }
    if (negative && digits != "0.00") {
        digits.insert(digits.begin(), '-');
    }
    return digits;
}

std::string Interpreter::render_scientific_style(const double value) {
    const bool negative = value < 0.0;
    const double magnitude = std::fabs(value);
    char buffer[64];
    const auto result = std::to_chars(
        buffer, buffer + sizeof(buffer), magnitude, std::chars_format::scientific, 2);
    const std::string text(buffer, result.ptr);
    const auto e_position = text.find('e');
    std::string mantissa = text.substr(0, e_position);
    const char sign = text[e_position + 1U];
    std::string exponent_digits = text.substr(e_position + 2U);
    while (exponent_digits.size() < 2U) {
        exponent_digits.insert(exponent_digits.begin(), '0');
    }
    std::string rendered = mantissa + "E" + sign + exponent_digits;
    if (negative) {
        rendered.insert(rendered.begin(), '-');
    }
    return rendered;
}

auto Interpreter::parse_picture_escapes(const std::string& section) -> EscapedPicture {
    EscapedPicture result;
    bool in_quotes = false;
    for (std::size_t index = 0U; index < section.size(); ++index) {
        if (!in_quotes && section[index] == '\\' && index + 1U < section.size()) {
            result.text.push_back(section[index + 1U]);
            result.forced_literal.push_back(true);
            ++index;
        } else if (section[index] == '"') {
            // The quote delimiter itself is never part of the output,
            // matching real VB6: `"` toggles whether the characters
            // between a pair of them are forced literal, the same
            // effect `\` has one character at a time.
            in_quotes = !in_quotes;
        } else {
            result.text.push_back(section[index]);
            result.forced_literal.push_back(in_quotes);
        }
    }
    return result;
}

std::string Interpreter::render_custom_numeric_picture(
    const double value, const std::string& picture) {
    std::vector<std::string> sections;
    std::string current_section;
    bool in_quotes = false;
    for (std::size_t index = 0U; index < picture.size(); ++index) {
        if (!in_quotes && picture[index] == '\\' && index + 1U < picture.size()) {
            current_section.push_back(picture[index]);
            current_section.push_back(picture[index + 1U]);
            ++index;
        } else if (picture[index] == '"') {
            in_quotes = !in_quotes;
            current_section.push_back(picture[index]);
        } else if (picture[index] == ';' && !in_quotes) {
            sections.push_back(std::move(current_section));
            current_section.clear();
        } else {
            current_section.push_back(picture[index]);
        }
    }
    sections.push_back(std::move(current_section));
    if (sections.size() == 1U) {
        return render_custom_numeric_picture_section(value, sections.front());
    }
    if (sections.size() >= 3U && value == 0.0) {
        return render_custom_numeric_picture_section(0.0, sections[2]);
    }
    if (value < 0.0) {
        return render_custom_numeric_picture_section(std::fabs(value), sections[1]);
    }
    return render_custom_numeric_picture_section(value, sections.front());
}

std::string Interpreter::render_custom_numeric_picture_section(
    const double value, const std::string& picture) {
    const bool negative = value < 0.0;

    // Scientific picture: `0.00E+00` / `0.0e-0` (mantissa picture, then
    // `E+`/`E-` and at least one exponent digit placeholder).
    {
        std::size_t e_position = std::string::npos;
        for (std::size_t i = 0; i < picture.size(); ++i) {
            const char c = picture[i];
            if (c == '\\') {
                ++i;
            } else if (c == '"') {
                const auto close = picture.find('"', i + 1U);
                if (close == std::string::npos) break;
                i = close;
            } else if ((c == 'E' || c == 'e') && i + 2U < picture.size() &&
                       (picture[i + 1U] == '+' || picture[i + 1U] == '-') &&
                       picture[i + 2U] == '0' && i > 0U) {
                e_position = i;
                break;
            }
        }
        if (e_position != std::string::npos) {
            const bool force_sign = picture[e_position + 1U] == '+';
            std::size_t exponent_digits = 0U;
            std::size_t after = e_position + 2U;
            while (after < picture.size() && picture[after] == '0') {
                ++exponent_digits;
                ++after;
            }
            const std::string mantissa_picture = picture.substr(0, e_position);
            std::size_t integer_places = 0U;
            for (const char c : mantissa_picture) {
                if (c == '.') break;
                if (c == '0' || c == '#') ++integer_places;
            }
            integer_places = std::max<std::size_t>(integer_places, 1U);
            std::size_t fraction_places = 0U;
            if (const auto dot = mantissa_picture.find('.'); dot != std::string::npos) {
                for (std::size_t i = dot + 1U; i < mantissa_picture.size(); ++i) {
                    if (mantissa_picture[i] == '0' || mantissa_picture[i] == '#') ++fraction_places;
                }
            }
            const double magnitude = std::fabs(value);
            int exponent = 0;
            double mantissa = 0.0;
            if (magnitude != 0.0) {
                exponent = static_cast<int>(std::floor(std::log10(magnitude))) -
                           static_cast<int>(integer_places - 1U);
                mantissa = magnitude / std::pow(10.0, exponent);
                const double scale = std::pow(10.0, static_cast<double>(fraction_places));
                const double limit = std::pow(10.0, static_cast<double>(integer_places));
                if (std::round(mantissa * scale) / scale >= limit) {
                    ++exponent;
                    mantissa = magnitude / std::pow(10.0, exponent);
                }
            }
            std::string exponent_text = std::to_string(std::abs(exponent));
            while (exponent_text.size() < exponent_digits) exponent_text.insert(0, "0");
            std::string result = render_custom_numeric_picture_section(mantissa, mantissa_picture);
            result.push_back(picture[e_position]);
            if (exponent < 0) result.push_back('-');
            else if (force_sign) result.push_back('+');
            result += exponent_text;
            result += picture.substr(after);
            return negative && magnitude != 0.0 ? "-" + result : result;
        }
    }

    // REQ-0221: expand every `\`-escaped pair first, so a placeholder/
    // decimal-point/grouping-comma character that was actually
    // written as `\0`/`\#`/`\,`/`\.` etc. is unambiguously a literal
    // at every check below, keyed by index into `escaped.text`
    // alongside `escaped.forced_literal` rather than by the character
    // value alone.
    const EscapedPicture escaped = parse_picture_escapes(picture);

    // REQ-0223: an unescaped, unquoted `%` anywhere in the picture
    // scales the value by 100 before any digit is matched against a
    // placeholder -- the same scaling the named `Percent` style
    // already applies -- while the `%` itself needs no special
    // handling at all to appear in the output: it was never one of
    // this format's own special characters, so it already passes
    // through as an ordinary literal at its own position.
    bool has_percent = false;
    for (std::size_t index = 0U; index < escaped.text.size(); ++index) {
        if (escaped.text[index] == '%' && !escaped.forced_literal[index]) {
            has_percent = true;
            break;
        }
    }
    const double magnitude = std::fabs(value) * (has_percent ? 100.0 : 1.0);
    std::size_t dot_position = escaped.text.size();
    for (std::size_t index = 0U; index < escaped.text.size(); ++index) {
        if (escaped.text[index] == '.' && !escaped.forced_literal[index]) {
            dot_position = index;
            break;
        }
    }
    const bool has_dot = dot_position < escaped.text.size();
    const std::string integer_pic = escaped.text.substr(0, dot_position);
    const std::vector<bool> integer_pic_forced(
        escaped.forced_literal.begin(), escaped.forced_literal.begin() + dot_position);
    const std::string fraction_pic =
        has_dot ? escaped.text.substr(dot_position + 1U) : std::string{};
    const std::vector<bool> fraction_pic_forced =
        has_dot ? std::vector<bool>(
                      escaped.forced_literal.begin() + static_cast<std::ptrdiff_t>(dot_position) + 1,
                      escaped.forced_literal.end())
                : std::vector<bool>{};

    std::size_t fraction_digit_count = 0U;
    for (std::size_t index = 0U; index < fraction_pic.size(); ++index) {
        if ((fraction_pic[index] == '0' || fraction_pic[index] == '#') &&
            !fraction_pic_forced[index]) {
            ++fraction_digit_count;
        }
    }

    const std::string rendered_magnitude =
        fixed_half_up(magnitude, static_cast<int>(fraction_digit_count));
    const auto rendered_dot = rendered_magnitude.find('.');
    std::string integer_digits = rendered_dot == std::string::npos
                                      ? rendered_magnitude
                                      : rendered_magnitude.substr(0, rendered_dot);
    if (integer_digits == "0") {
        integer_digits.clear();  // a zero integer part shows no digit (`0` placeholders still pad)
    }
    const std::string fraction_digits = rendered_dot == std::string::npos
                                             ? std::string{}
                                             : rendered_magnitude.substr(rendered_dot + 1U);

    // Fraction section: left to right, one placeholder per rounded
    // digit, trimming a trailing run of zero digits that came from an
    // optional '#' placeholder.
    std::string fraction_output;
    std::vector<bool> fraction_is_trimmable_zero;
    std::size_t fraction_digit_index = 0U;
    for (std::size_t index = 0U; index < fraction_pic.size(); ++index) {
        const char character = fraction_pic[index];
        if ((character == '0' || character == '#') && !fraction_pic_forced[index]) {
            const char digit = fraction_digit_index < fraction_digits.size()
                                    ? fraction_digits[fraction_digit_index]
                                    : '0';
            ++fraction_digit_index;
            fraction_output.push_back(digit);
            fraction_is_trimmable_zero.push_back(character == '#' && digit == '0');
        } else {
            fraction_output.push_back(character);
            fraction_is_trimmable_zero.push_back(false);
        }
    }
    while (!fraction_output.empty() && fraction_is_trimmable_zero.back()) {
        fraction_output.pop_back();
        fraction_is_trimmable_zero.pop_back();
    }

    // Integer section: right to left, one placeholder per digit; an
    // unescaped comma is a grouping instruction (stripped before
    // matching, not a literal output position) -- an escaped `\,`
    // keeps its own forced-literal marker instead, so it survives
    // into `integer_pic_digits_only` as an ordinary literal character.
    std::string integer_pic_digits_only;
    std::vector<bool> integer_pic_digits_only_forced;
    bool has_unescaped_comma = false;
    for (std::size_t index = 0U; index < integer_pic.size(); ++index) {
        if (integer_pic[index] == ',' && !integer_pic_forced[index]) {
            has_unescaped_comma = true;
            continue;
        }
        integer_pic_digits_only.push_back(integer_pic[index]);
        integer_pic_digits_only_forced.push_back(integer_pic_forced[index]);
    }
    std::string integer_output_reversed;
    // Parallel to `integer_output_reversed`: true at every position
    // that holds an actual digit (from a placeholder or from the
    // overflow below), false at a literal. Comma grouping below walks
    // this to group only the digit run itself, so a literal sitting
    // after the last placeholder (a closing `)` on a parenthesized
    // negative picture, for example) is not miscounted as part of it.
    std::vector<bool> integer_output_is_digit_reversed;
    std::size_t digit_source_index = integer_digits.size();
    // Any digit beyond the leftmost placeholder is inserted right next
    // to that placeholder's own digit, not shoved past whatever
    // literal text happens to sit further left in the picture (for
    // example a `(`/`$` prefix on a negative/currency picture) --
    // `leftmost_placeholder_point` remembers where, in this
    // right-to-left buffer, that insertion belongs: updated every time
    // a placeholder is processed, so its final value is wherever the
    // *last* (i.e. leftmost) one landed. A picture with no placeholder
    // at all never updates it, leaving it at the buffer's eventual
    // full length -- overflow digits then land at the very front of
    // the rendered result instead, ahead of every literal character,
    // matching this function's other documented "no placeholders"
    // behavior.
    std::size_t leftmost_placeholder_point = 0U;
    bool seen_placeholder = false;
    for (std::size_t reverse_index = integer_pic_digits_only.size(); reverse_index-- > 0;) {
        const char character = integer_pic_digits_only[reverse_index];
        const bool forced = integer_pic_digits_only_forced[reverse_index];
        if (character == '0' && !forced) {
            integer_output_reversed.push_back(
                digit_source_index > 0U ? integer_digits[--digit_source_index] : '0');
            integer_output_is_digit_reversed.push_back(true);
            leftmost_placeholder_point = integer_output_reversed.size();
            seen_placeholder = true;
        } else if (character == '#' && !forced) {
            if (digit_source_index > 0U) {
                integer_output_reversed.push_back(integer_digits[--digit_source_index]);
                integer_output_is_digit_reversed.push_back(true);
            }
            leftmost_placeholder_point = integer_output_reversed.size();
            seen_placeholder = true;
        } else {
            integer_output_reversed.push_back(character);
            integer_output_is_digit_reversed.push_back(false);
        }
    }
    if (digit_source_index > 0U) {
        std::string overflow_digits;
        while (digit_source_index > 0U) {
            overflow_digits.push_back(integer_digits[--digit_source_index]);
        }
        const std::size_t insertion_point =
            seen_placeholder ? leftmost_placeholder_point : integer_output_reversed.size();
        integer_output_reversed.insert(insertion_point, overflow_digits);
        integer_output_is_digit_reversed.insert(
            integer_output_is_digit_reversed.begin() +
                static_cast<std::ptrdiff_t>(insertion_point),
            overflow_digits.size(), true);
    }
    std::string integer_output(integer_output_reversed.rbegin(), integer_output_reversed.rend());
    if (has_unescaped_comma) {
        const auto total_digits = static_cast<std::size_t>(std::count(
            integer_output_is_digit_reversed.begin(), integer_output_is_digit_reversed.end(),
            true));
        std::string grouped_reversed;
        std::size_t digits_seen = 0U;
        for (std::size_t index = 0U; index < integer_output_reversed.size(); ++index) {
            grouped_reversed.push_back(integer_output_reversed[index]);
            if (integer_output_is_digit_reversed[index]) {
                ++digits_seen;
                if (digits_seen % 3U == 0U && digits_seen < total_digits) {
                    grouped_reversed.push_back(',');
                }
            }
        }
        integer_output.assign(grouped_reversed.rbegin(), grouped_reversed.rend());
    }

    std::string rendered = integer_output;
    if (has_dot) {
        rendered += "." + fraction_output;
    }
    const bool all_zero =
        std::all_of(
            integer_digits.begin(), integer_digits.end(),
            [](const char character) { return character == '0'; }) &&
        std::all_of(
            fraction_digits.begin(), fraction_digits.end(),
            [](const char character) { return character == '0'; });
    if (negative && !all_zero) {
        rendered.insert(rendered.begin(), '-');
    }
    return rendered;
}

std::string Interpreter::render_floating(const double number, const int digits) {
    if (std::isnan(number) || std::isinf(number)) {
        return std::to_string(number);
    }
    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), "%.*G", digits, number);
    std::string text = buffer;
    // VB writes 1E+15 / 1E-05 exactly like C; but negative zero is "0".
    if (text == "-0") {
        return "0";
    }
    return text;
}

std::string Interpreter::render(const Value& value) {
    if (const auto* integer = std::get_if<Integer>(&value)) {
        return std::to_string(*integer);
    }
    if (const auto* short_integer = std::get_if<Int16>(&value)) {
        return std::to_string(*short_integer);
    }
    if (const auto* byte = std::get_if<Byte>(&value)) {
        return std::to_string(static_cast<unsigned>(*byte));
    }
    if (const auto* number = std::get_if<double>(&value)) {
        return render_floating(*number, 15);
    }
    if (const auto* single = std::get_if<float>(&value)) {
        return render_floating(static_cast<double>(*single), 7);
    }
    if (const auto* currency = std::get_if<Currency>(&value)) {
        return render_currency(currency->scaled);
    }
    if (const auto* decimal = std::get_if<Decimal>(&value)) {
        return render_decimal(*decimal);
    }
    if (const auto* date = std::get_if<DateValue>(&value)) {
        return render_date(date->serial);
    }
    if (const auto* error = std::get_if<ErrorValue>(&value)) {
        return "Error " + std::to_string(error->code);
    }
    if (const auto* string = std::get_if<std::string>(&value)) {
        return *string;
    }
    // Empty renders as an empty string (verified: Print/CStr of an
    // uninitialized Variant produces no text). Null also renders as an
    // empty string here as a safe, non-crashing fallback for this
    // unconditional static formatter; every explicit conversion path
    // that must instead raise "Invalid use of Null" (CStr, concatenation
    // of two Nulls, etc.) rejects Null before ever calling render().
    if (std::holds_alternative<Empty>(value) || std::holds_alternative<Null>(value)) {
        return "";
    }
    // Nothing, a live instance, and arrays render as empty strings here
    // as a safe, non-crashing fallback for this unconditional static
    // formatter, matching Null/Empty's convention; CStr explicitly
    // rejects an object reference (mirroring its Null rejection) before
    // ever calling render().
    if (is_object_reference(value) || std::holds_alternative<ArrayValue>(value)) {
        return "";
    }
    return std::get<bool>(value) ? "True" : "False";
}

}  // namespace wfc::detail
