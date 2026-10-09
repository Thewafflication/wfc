// VB numeric representations: Integer aliases, Currency, Decimal.
// Internal to the WFC evaluator; not part of the public API.
// Split out of src/evaluator.cpp; see src/interpreter/README.md.

#ifndef WFC_INTERPRETER_VB_NUMERIC_HPP
#define WFC_INTERPRETER_VB_NUMERIC_HPP

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <deque>
#include <exception>
#include <filesystem>
#include <functional>
#include <map>
#include <limits>
#include <memory>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <thread>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace wfc::detail {

using Integer = std::int32_t;

// VB6's Integer (16-bit) type is distinct from the 32-bit Long already
// aliased above as Integer -- an unfortunate naming collision between this
// codebase's C++ alias and VB6's own type name that predates this type's
// introduction. `Int16` is VB6's `Integer`.
using Int16 = std::int16_t;
using Byte = std::uint8_t;  // REQ-0247

// A Currency value is VB6/COM's fixed-point CURRENCY representation: a
// signed 64-bit integer scaled by 10000 (four decimal digits). The scale was
// chosen so the type's documented range, -922337203685477.5808 through
// 922337203685477.5807, maps exactly onto std::int64_t's min/max.
struct Currency {
    std::int64_t scaled{};

    [[nodiscard]] friend bool operator==(const Currency left, const Currency right) noexcept {
        return left.scaled == right.scaled;
    }
};

// Minimal unsigned 128-bit integer support for exact fixed-point Currency
// multiplication and division: a signed 64x64 multiply can overflow 64 bits,
// and computing round(a*b/10000) or round(a*10000/b) exactly (rather than
// through a lossy double intermediate) needs the full-width product.
struct UInt128 {
    std::uint64_t high{};
    std::uint64_t low{};
};

[[nodiscard]] inline UInt128 multiply_u64(const std::uint64_t left, const std::uint64_t right) noexcept {
    const std::uint64_t left_lo = left & 0xFFFFFFFFULL;
    const std::uint64_t left_hi = left >> 32U;
    const std::uint64_t right_lo = right & 0xFFFFFFFFULL;
    const std::uint64_t right_hi = right >> 32U;
    const std::uint64_t lo_lo = left_lo * right_lo;
    const std::uint64_t hi_lo = left_hi * right_lo;
    const std::uint64_t lo_hi = left_lo * right_hi;
    const std::uint64_t hi_hi = left_hi * right_hi;
    const std::uint64_t cross =
        (lo_lo >> 32U) + (hi_lo & 0xFFFFFFFFULL) + (lo_hi & 0xFFFFFFFFULL);
    UInt128 result;
    result.low = (lo_lo & 0xFFFFFFFFULL) | (cross << 32U);
    result.high = hi_hi + (hi_lo >> 32U) + (lo_hi >> 32U) + (cross >> 32U);
    return result;
}

[[nodiscard]] inline int compare_u128(const UInt128 left, const UInt128 right) noexcept {
    if (left.high != right.high) {
        return left.high < right.high ? -1 : 1;
    }
    if (left.low != right.low) {
        return left.low < right.low ? -1 : 1;
    }
    return 0;
}

[[nodiscard]] inline UInt128 subtract_u128(const UInt128 left, const UInt128 right) noexcept {
    UInt128 result;
    result.low = left.low - right.low;
    result.high = left.high - right.high - (left.low < right.low ? 1ULL : 0ULL);
    return result;
}

[[nodiscard]] inline UInt128 shift_left_one_u128(const UInt128 value) noexcept {
    UInt128 result;
    result.high = (value.high << 1U) | (value.low >> 63U);
    result.low = value.low << 1U;
    return result;
}

// Binary long division: correct for any 128-bit dividend/divisor, at the
// cost of 128 iterations. Currency operations are not performance-critical,
// so simplicity and correctness are preferred over a faster algorithm.
inline void divide_u128(
    const UInt128 dividend,
    const UInt128 divisor,
    UInt128& quotient,
    UInt128& remainder) noexcept {
    quotient = UInt128{};
    remainder = UInt128{};
    for (int bit = 127; bit >= 0; --bit) {
        remainder = shift_left_one_u128(remainder);
        const bool dividend_bit = bit >= 64
            ? (((dividend.high >> (bit - 64)) & 1ULL) != 0ULL)
            : (((dividend.low >> bit) & 1ULL) != 0ULL);
        if (dividend_bit) {
            remainder.low |= 1ULL;
        }
        if (compare_u128(remainder, divisor) >= 0) {
            remainder = subtract_u128(remainder, divisor);
            if (bit >= 64) {
                quotient.high |= (std::uint64_t{1} << (bit - 64));
            } else {
                quotient.low |= (std::uint64_t{1} << bit);
            }
        }
    }
}

[[nodiscard]] inline std::uint64_t magnitude_of(const std::int64_t value) noexcept {
    return value < 0 ? (~static_cast<std::uint64_t>(value) + 1ULL)
                      : static_cast<std::uint64_t>(value);
}

// round(left * right / 10000) computed with an exact 128-bit intermediate
// product, using banker's rounding on the discarded remainder. Returns
// nullopt when the mathematical result does not fit in int64_t.
[[nodiscard]] inline std::optional<std::int64_t> currency_multiply(
    const std::int64_t left,
    const std::int64_t right) noexcept {
    const bool negative = (left < 0) != (right < 0);
    const UInt128 product = multiply_u64(magnitude_of(left), magnitude_of(right));
    const UInt128 divisor{0ULL, 10000ULL};
    UInt128 quotient{};
    UInt128 remainder{};
    divide_u128(product, divisor, quotient, remainder);
    const int comparison = compare_u128(shift_left_one_u128(remainder), divisor);
    const bool round_up =
        comparison > 0 || (comparison == 0 && (quotient.low % 2ULL) != 0ULL);
    if (round_up) {
        if (quotient.low == std::numeric_limits<std::uint64_t>::max()) {
            quotient.low = 0ULL;
            ++quotient.high;
        } else {
            ++quotient.low;
        }
    }
    if (quotient.high != 0ULL ||
        quotient.low > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
        return std::nullopt;
    }
    const auto magnitude = static_cast<std::int64_t>(quotient.low);
    return negative ? -magnitude : magnitude;
}

// round(left * 10000 / right) computed with an exact 128-bit intermediate
// numerator, using banker's rounding on the discarded remainder. `right`
// must be nonzero. Returns nullopt on overflow.
[[nodiscard]] inline std::optional<std::int64_t> currency_divide(
    const std::int64_t left,
    const std::int64_t right) noexcept {
    const bool negative = (left < 0) != (right < 0);
    const UInt128 numerator = multiply_u64(magnitude_of(left), 10000ULL);
    const UInt128 divisor{0ULL, magnitude_of(right)};
    UInt128 quotient{};
    UInt128 remainder{};
    divide_u128(numerator, divisor, quotient, remainder);
    const int comparison = compare_u128(shift_left_one_u128(remainder), divisor);
    const bool round_up =
        comparison > 0 || (comparison == 0 && (quotient.low % 2ULL) != 0ULL);
    if (round_up) {
        if (quotient.low == std::numeric_limits<std::uint64_t>::max()) {
            quotient.low = 0ULL;
            ++quotient.high;
        } else {
            ++quotient.low;
        }
    }
    if (quotient.high != 0ULL ||
        quotient.low > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
        return std::nullopt;
    }
    const auto magnitude = static_cast<std::int64_t>(quotient.low);
    return negative ? -magnitude : magnitude;
}

// Round a finite double to the nearest Currency tick (banker's rounding,
// matching this evaluator's established Double-to-integer rounding
// convention) and check it against Currency's representable range. Returns
// nullopt for a non-finite or out-of-range input.
[[nodiscard]] inline std::optional<std::int64_t> currency_from_double(const double value) noexcept {
    if (!std::isfinite(value)) {
        return std::nullopt;
    }
    const double scaled = std::nearbyint(value * 10000.0);
    constexpr double minimum = -9223372036854775808.0;  // exactly 2^63, exact in double
    constexpr double upper_bound = 9223372036854775808.0;  // 2^63, exclusive upper bound
    if (!(scaled >= minimum && scaled < upper_bound)) {
        return std::nullopt;
    }
    return static_cast<std::int64_t>(scaled);
}

enum class NumericStringStatus { valid, malformed, out_of_range };

// A minimal fixed-width unsigned big integer (256 bits, eight 32-bit limbs,
// least-significant first) used only for exact Decimal arithmetic. Decimal's
// mantissa is at most 96 bits; a 96-bit x 96-bit product needs up to 192
// bits, so 256 bits leaves headroom without dynamic allocation.
struct BigUInt {
    std::array<std::uint32_t, 8> limb{};
};

[[nodiscard]] inline bool is_zero_big(const BigUInt& value) noexcept {
    for (const auto word : value.limb) {
        if (word != 0U) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] inline int compare_big(const BigUInt& left, const BigUInt& right) noexcept {
    for (int i = 7; i >= 0; --i) {
        if (left.limb[static_cast<std::size_t>(i)] != right.limb[static_cast<std::size_t>(i)]) {
            return left.limb[static_cast<std::size_t>(i)] < right.limb[static_cast<std::size_t>(i)]
                       ? -1
                       : 1;
        }
    }
    return 0;
}

[[nodiscard]] inline BigUInt add_big(const BigUInt& left, const BigUInt& right) noexcept {
    BigUInt result;
    std::uint64_t carry = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        const std::uint64_t sum =
            static_cast<std::uint64_t>(left.limb[i]) + right.limb[i] + carry;
        result.limb[i] = static_cast<std::uint32_t>(sum);
        carry = sum >> 32U;
    }
    return result;
}

// Requires left >= right.
[[nodiscard]] inline BigUInt subtract_big(const BigUInt& left, const BigUInt& right) noexcept {
    BigUInt result;
    std::int64_t borrow = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        std::int64_t difference =
            static_cast<std::int64_t>(left.limb[i]) - right.limb[i] - borrow;
        if (difference < 0) {
            difference += (std::int64_t{1} << 32U);
            borrow = 1;
        } else {
            borrow = 0;
        }
        result.limb[i] = static_cast<std::uint32_t>(difference);
    }
    return result;
}

// The Decimal callers only ever multiply values already confirmed to be at
// most 96 bits, so the product is always within 192 bits and never
// overflows this 256-bit representation.
[[nodiscard]] inline BigUInt multiply_big(const BigUInt& left, const BigUInt& right) noexcept {
    BigUInt result;
    for (std::size_t i = 0; i < 8; ++i) {
        if (left.limb[i] == 0U) {
            continue;
        }
        std::uint64_t carry = 0;
        for (std::size_t j = 0; j + i < 8; ++j) {
            const std::uint64_t product = static_cast<std::uint64_t>(left.limb[i]) * right.limb[j] +
                                           result.limb[i + j] + carry;
            result.limb[i + j] = static_cast<std::uint32_t>(product);
            carry = product >> 32U;
        }
    }
    return result;
}

[[nodiscard]] inline BigUInt shift_left_one_big(const BigUInt& value) noexcept {
    BigUInt result;
    std::uint32_t carry = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        result.limb[i] = (value.limb[i] << 1U) | carry;
        carry = value.limb[i] >> 31U;
    }
    return result;
}

// Binary long division: 256 iterations, favoring correctness and simplicity
// over speed. Decimal arithmetic is not performance-critical.
inline void divide_big(
    const BigUInt& dividend,
    const BigUInt& divisor,
    BigUInt& quotient,
    BigUInt& remainder) noexcept {
    quotient = BigUInt{};
    remainder = BigUInt{};
    for (int bit = 255; bit >= 0; --bit) {
        remainder = shift_left_one_big(remainder);
        const auto word = static_cast<std::size_t>(bit) / 32U;
        const auto offset = static_cast<unsigned>(bit) % 32U;
        if (((dividend.limb[word] >> offset) & 1U) != 0U) {
            remainder.limb[0] |= 1U;
        }
        if (compare_big(remainder, divisor) >= 0) {
            remainder = subtract_big(remainder, divisor);
            quotient.limb[word] |= (std::uint32_t{1} << offset);
        }
    }
}

[[nodiscard]] inline BigUInt big_from_u32(const std::uint32_t value) noexcept {
    BigUInt result;
    result.limb[0] = value;
    return result;
}

[[nodiscard]] inline bool has_bits_beyond_96(const BigUInt& value) noexcept {
    for (std::size_t i = 3; i < 8; ++i) {
        if (value.limb[i] != 0U) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] inline BigUInt power_of_ten_big(const int exponent) noexcept {
    BigUInt result = big_from_u32(1U);
    for (int i = 0; i < exponent; ++i) {
        result = multiply_big(result, big_from_u32(10U));
    }
    return result;
}

// Divide `value` by 10, rounding the quotient to the nearest integer with
// banker's rounding (round half to even), matching this evaluator's
// established Double-to-integer rounding convention.
[[nodiscard]] inline BigUInt divide_by_ten_rounded_big(const BigUInt& value) noexcept {
    BigUInt quotient;
    BigUInt remainder;
    divide_big(value, big_from_u32(10U), quotient, remainder);
    const std::uint32_t digit = remainder.limb[0];
    const bool round_up = digit > 5U || (digit == 5U && (quotient.limb[0] & 1U) != 0U);
    return round_up ? add_big(quotient, big_from_u32(1U)) : quotient;
}

// VB6/COM's DECIMAL: a sign, a variable scale (0 through 28 digits after the
// decimal point), and a 96-bit unsigned mantissa. The mantissa occupies the
// low three limbs of a BigUInt; the remaining limbs are scratch space
// shared with the arithmetic helpers above and are always zero at rest.
struct Decimal {
    bool negative{};
    std::uint8_t scale{};
    BigUInt mantissa{};

    [[nodiscard]] friend bool operator==(const Decimal& left, const Decimal& right) noexcept {
        return left.negative == right.negative && left.scale == right.scale &&
               left.mantissa.limb == right.mantissa.limb;
    }
};

inline constexpr int decimal_max_scale = 28;

// Rescale `mantissa` up by `scale_diff` decimal places (multiply by
// 10^scale_diff). Returns nullopt if the result would exceed 96 bits.
[[nodiscard]] inline std::optional<BigUInt> rescale_mantissa_up(
    const BigUInt& mantissa,
    const int scale_diff) noexcept {
    if (scale_diff == 0) {
        return mantissa;
    }
    const BigUInt product = multiply_big(mantissa, power_of_ten_big(scale_diff));
    if (has_bits_beyond_96(product)) {
        return std::nullopt;
    }
    return product;
}

[[nodiscard]] inline std::optional<Decimal> decimal_add(const Decimal& left, const Decimal& right) noexcept {
    const auto scale = std::max(left.scale, right.scale);
    const auto left_mantissa = rescale_mantissa_up(left.mantissa, scale - left.scale);
    const auto right_mantissa = rescale_mantissa_up(right.mantissa, scale - right.scale);
    if (!left_mantissa.has_value() || !right_mantissa.has_value()) {
        return std::nullopt;
    }
    Decimal result;
    result.scale = scale;
    if (left.negative == right.negative) {
        result.mantissa = add_big(*left_mantissa, *right_mantissa);
        result.negative = left.negative;
    } else {
        const int comparison = compare_big(*left_mantissa, *right_mantissa);
        if (comparison == 0) {
            result.negative = false;
        } else if (comparison > 0) {
            result.mantissa = subtract_big(*left_mantissa, *right_mantissa);
            result.negative = left.negative;
        } else {
            result.mantissa = subtract_big(*right_mantissa, *left_mantissa);
            result.negative = right.negative;
        }
    }
    if (has_bits_beyond_96(result.mantissa)) {
        return std::nullopt;
    }
    return result;
}

[[nodiscard]] inline std::optional<Decimal> decimal_subtract(
    const Decimal& left,
    const Decimal& right) noexcept {
    Decimal negated_right = right;
    if (!is_zero_big(negated_right.mantissa)) {
        negated_right.negative = !negated_right.negative;
    }
    return decimal_add(left, negated_right);
}

[[nodiscard]] inline std::optional<Decimal> decimal_multiply(
    const Decimal& left,
    const Decimal& right) noexcept {
    BigUInt product = multiply_big(left.mantissa, right.mantissa);
    int scale = static_cast<int>(left.scale) + static_cast<int>(right.scale);
    while (scale > 0 && (has_bits_beyond_96(product) || scale > decimal_max_scale)) {
        product = divide_by_ten_rounded_big(product);
        --scale;
    }
    if (has_bits_beyond_96(product)) {
        return std::nullopt;
    }
    Decimal result;
    result.negative = (left.negative != right.negative) && !is_zero_big(product);
    result.scale = static_cast<std::uint8_t>(scale);
    result.mantissa = product;
    return result;
}

// Divide with maximum representable precision: the numerator is scaled up
// (extra factors of 10) before dividing so the quotient carries as many
// significant decimal digits as fit within scale 0-28 and a 96-bit
// mantissa, then rounded (banker's rounding) to the nearest representable
// value. `right` must be nonzero; the caller reports WFC0008 separately.
[[nodiscard]] inline std::optional<Decimal> decimal_divide(
    const Decimal& left,
    const Decimal& right) noexcept {
    int result_scale = static_cast<int>(left.scale) - static_cast<int>(right.scale);
    BigUInt numerator = left.mantissa;
    while (result_scale < decimal_max_scale) {
        numerator = multiply_big(numerator, big_from_u32(10U));
        ++result_scale;
    }
    BigUInt quotient;
    BigUInt remainder;
    divide_big(numerator, right.mantissa, quotient, remainder);
    const BigUInt doubled_remainder = shift_left_one_big(remainder);
    const int comparison = compare_big(doubled_remainder, right.mantissa);
    const bool round_up = comparison > 0 || (comparison == 0 && (quotient.limb[0] & 1U) != 0U);
    if (round_up) {
        quotient = add_big(quotient, big_from_u32(1U));
    }
    while (result_scale > 0 && has_bits_beyond_96(quotient)) {
        quotient = divide_by_ten_rounded_big(quotient);
        --result_scale;
    }
    if (has_bits_beyond_96(quotient) || result_scale < 0 || result_scale > decimal_max_scale) {
        return std::nullopt;
    }
    Decimal result;
    result.negative = (left.negative != right.negative) && !is_zero_big(quotient);
    result.scale = static_cast<std::uint8_t>(result_scale);
    result.mantissa = quotient;
    return result;
}

[[nodiscard]] inline double decimal_to_double(const Decimal& value) noexcept {
    double magnitude = 0.0;
    for (int i = 7; i >= 0; --i) {
        magnitude = magnitude * 4294967296.0 + static_cast<double>(value.mantissa.limb[static_cast<std::size_t>(i)]);
    }
    for (int i = 0; i < value.scale; ++i) {
        magnitude /= 10.0;
    }
    return value.negative ? -magnitude : magnitude;
}

[[nodiscard]] inline std::string render_decimal(const Decimal& value) {
    std::string digits;
    if (is_zero_big(value.mantissa)) {
        digits = "0";
    } else {
        BigUInt remaining = value.mantissa;
        while (!is_zero_big(remaining)) {
            BigUInt quotient;
            BigUInt remainder;
            divide_big(remaining, big_from_u32(10U), quotient, remainder);
            digits.push_back(static_cast<char>('0' + remainder.limb[0]));
            remaining = quotient;
        }
        std::reverse(digits.begin(), digits.end());
    }
    std::string result;
    if (value.scale == 0U) {
        result = digits;
    } else {
        if (digits.size() <= value.scale) {
            digits.insert(digits.begin(), static_cast<std::size_t>(value.scale) - digits.size() + 1U, '0');
        }
        const auto point = digits.size() - value.scale;
        std::string integer_part = digits.substr(0, point);
        std::string fraction_part = digits.substr(point);
        while (!fraction_part.empty() && fraction_part.back() == '0') {
            fraction_part.pop_back();
        }
        result = integer_part;
        if (!fraction_part.empty()) {
            result += '.';
            result += fraction_part;
        }
    }
    if (value.negative && result != "0") {
        result.insert(result.begin(), '-');
    }
    return result;
}

// Parse a complete, finite decimal/exponent String (the same grammar
// accepted elsewhere by parse_numeric_string) directly into an exact
// Decimal, without a lossy double intermediate. Surrounding ASCII
// whitespace and a leading sign are accepted.
struct DecimalStringResult {
    NumericStringStatus status{NumericStringStatus::malformed};
    Decimal value{};
};

[[nodiscard]] inline DecimalStringResult parse_decimal_string(const std::string_view text) {
    std::size_t first = 0;
    std::size_t last = text.size();
    const auto is_ascii_whitespace = [](const char character) {
        return character == ' ' || character == '\t' || character == '\r' || character == '\n';
    };
    while (first < last && is_ascii_whitespace(text[first])) {
        ++first;
    }
    while (last > first && is_ascii_whitespace(text[last - 1U])) {
        --last;
    }
    if (first == last) {
        return {};
    }
    bool negative = false;
    if (text[first] == '+') {
        ++first;
    } else if (text[first] == '-') {
        negative = true;
        ++first;
    }
    if (first == last) {
        return {};
    }

    BigUInt mantissa{};
    int scale = 0;
    bool any_digit = false;
    bool mantissa_overflow = false;
    const auto consume_digit = [&](const char character) {
        any_digit = true;
        mantissa = multiply_big(mantissa, big_from_u32(10U));
        mantissa = add_big(mantissa, big_from_u32(static_cast<std::uint32_t>(character - '0')));
        if (has_bits_beyond_96(mantissa)) {
            mantissa_overflow = true;
        }
    };
    std::size_t index = first;
    while (index < last && std::isdigit(static_cast<unsigned char>(text[index])) != 0) {
        consume_digit(text[index]);
        ++index;
    }
    if (index < last && text[index] == '.') {
        ++index;
        while (index < last && std::isdigit(static_cast<unsigned char>(text[index])) != 0) {
            consume_digit(text[index]);
            ++scale;
            ++index;
        }
    }
    if (!any_digit) {
        return {};
    }
    int exponent = 0;
    if (index < last && (text[index] == 'e' || text[index] == 'E')) {
        ++index;
        bool exponent_negative = false;
        if (index < last && (text[index] == '+' || text[index] == '-')) {
            exponent_negative = text[index] == '-';
            ++index;
        }
        const auto exponent_start = index;
        int exponent_magnitude = 0;
        while (index < last && std::isdigit(static_cast<unsigned char>(text[index])) != 0) {
            if (exponent_magnitude < 1'000'000) {
                exponent_magnitude = exponent_magnitude * 10 + (text[index] - '0');
            }
            ++index;
        }
        if (index == exponent_start) {
            return {};
        }
        exponent = exponent_negative ? -exponent_magnitude : exponent_magnitude;
    }
    if (index != last) {
        return {};
    }

    scale -= exponent;
    while (scale < 0) {
        mantissa = multiply_big(mantissa, big_from_u32(10U));
        if (has_bits_beyond_96(mantissa)) {
            mantissa_overflow = true;
        }
        ++scale;
    }
    while (scale > decimal_max_scale) {
        mantissa = divide_by_ten_rounded_big(mantissa);
        --scale;
    }
    if (mantissa_overflow || has_bits_beyond_96(mantissa)) {
        return {NumericStringStatus::out_of_range, {}};
    }

    DecimalStringResult result;
    result.status = NumericStringStatus::valid;
    result.value.negative = negative && !is_zero_big(mantissa);
    result.value.scale = static_cast<std::uint8_t>(scale);
    result.value.mantissa = mantissa;
    return result;
}

// Convert a finite double to an exact Decimal by parsing the double's own
// shortest round-tripping decimal text, rather than scaling the binary
// value directly; this avoids compounding binary-to-decimal error on top of
// the conversion. Returns nullopt for a non-finite or out-of-range input.
[[nodiscard]] inline std::optional<Decimal> decimal_from_double(const double value) noexcept {
    if (!std::isfinite(value)) {
        return std::nullopt;
    }
    char buffer[32];
    const auto conversion = std::to_chars(buffer, buffer + sizeof(buffer), value);
    const auto parsed = parse_decimal_string(
        std::string_view(buffer, static_cast<std::size_t>(conversion.ptr - buffer)));
    if (parsed.status != NumericStringStatus::valid) {
        return std::nullopt;
    }
    return parsed.value;
}

}  // namespace wfc::detail

#endif  // WFC_INTERPRETER_VB_NUMERIC_HPP
