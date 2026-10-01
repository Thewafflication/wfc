#include "wfc/evaluator.hpp"

#include "large_stack.hpp"

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

namespace {

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

[[nodiscard]] UInt128 multiply_u64(const std::uint64_t left, const std::uint64_t right) noexcept {
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

[[nodiscard]] int compare_u128(const UInt128 left, const UInt128 right) noexcept {
    if (left.high != right.high) {
        return left.high < right.high ? -1 : 1;
    }
    if (left.low != right.low) {
        return left.low < right.low ? -1 : 1;
    }
    return 0;
}

[[nodiscard]] UInt128 subtract_u128(const UInt128 left, const UInt128 right) noexcept {
    UInt128 result;
    result.low = left.low - right.low;
    result.high = left.high - right.high - (left.low < right.low ? 1ULL : 0ULL);
    return result;
}

[[nodiscard]] UInt128 shift_left_one_u128(const UInt128 value) noexcept {
    UInt128 result;
    result.high = (value.high << 1U) | (value.low >> 63U);
    result.low = value.low << 1U;
    return result;
}

// Binary long division: correct for any 128-bit dividend/divisor, at the
// cost of 128 iterations. Currency operations are not performance-critical,
// so simplicity and correctness are preferred over a faster algorithm.
void divide_u128(
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

[[nodiscard]] std::uint64_t magnitude_of(const std::int64_t value) noexcept {
    return value < 0 ? (~static_cast<std::uint64_t>(value) + 1ULL)
                      : static_cast<std::uint64_t>(value);
}

// round(left * right / 10000) computed with an exact 128-bit intermediate
// product, using banker's rounding on the discarded remainder. Returns
// nullopt when the mathematical result does not fit in int64_t.
[[nodiscard]] std::optional<std::int64_t> currency_multiply(
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
[[nodiscard]] std::optional<std::int64_t> currency_divide(
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
[[nodiscard]] std::optional<std::int64_t> currency_from_double(const double value) noexcept {
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

[[nodiscard]] bool is_zero_big(const BigUInt& value) noexcept {
    for (const auto word : value.limb) {
        if (word != 0U) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] int compare_big(const BigUInt& left, const BigUInt& right) noexcept {
    for (int i = 7; i >= 0; --i) {
        if (left.limb[static_cast<std::size_t>(i)] != right.limb[static_cast<std::size_t>(i)]) {
            return left.limb[static_cast<std::size_t>(i)] < right.limb[static_cast<std::size_t>(i)]
                       ? -1
                       : 1;
        }
    }
    return 0;
}

[[nodiscard]] BigUInt add_big(const BigUInt& left, const BigUInt& right) noexcept {
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
[[nodiscard]] BigUInt subtract_big(const BigUInt& left, const BigUInt& right) noexcept {
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
[[nodiscard]] BigUInt multiply_big(const BigUInt& left, const BigUInt& right) noexcept {
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

[[nodiscard]] BigUInt shift_left_one_big(const BigUInt& value) noexcept {
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
void divide_big(
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

[[nodiscard]] BigUInt big_from_u32(const std::uint32_t value) noexcept {
    BigUInt result;
    result.limb[0] = value;
    return result;
}

[[nodiscard]] bool has_bits_beyond_96(const BigUInt& value) noexcept {
    for (std::size_t i = 3; i < 8; ++i) {
        if (value.limb[i] != 0U) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] BigUInt power_of_ten_big(const int exponent) noexcept {
    BigUInt result = big_from_u32(1U);
    for (int i = 0; i < exponent; ++i) {
        result = multiply_big(result, big_from_u32(10U));
    }
    return result;
}

// Divide `value` by 10, rounding the quotient to the nearest integer with
// banker's rounding (round half to even), matching this evaluator's
// established Double-to-integer rounding convention.
[[nodiscard]] BigUInt divide_by_ten_rounded_big(const BigUInt& value) noexcept {
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

constexpr int decimal_max_scale = 28;

// Rescale `mantissa` up by `scale_diff` decimal places (multiply by
// 10^scale_diff). Returns nullopt if the result would exceed 96 bits.
[[nodiscard]] std::optional<BigUInt> rescale_mantissa_up(
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

[[nodiscard]] std::optional<Decimal> decimal_add(const Decimal& left, const Decimal& right) noexcept {
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

[[nodiscard]] std::optional<Decimal> decimal_subtract(
    const Decimal& left,
    const Decimal& right) noexcept {
    Decimal negated_right = right;
    if (!is_zero_big(negated_right.mantissa)) {
        negated_right.negative = !negated_right.negative;
    }
    return decimal_add(left, negated_right);
}

[[nodiscard]] std::optional<Decimal> decimal_multiply(
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
[[nodiscard]] std::optional<Decimal> decimal_divide(
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

[[nodiscard]] double decimal_to_double(const Decimal& value) noexcept {
    double magnitude = 0.0;
    for (int i = 7; i >= 0; --i) {
        magnitude = magnitude * 4294967296.0 + static_cast<double>(value.mantissa.limb[static_cast<std::size_t>(i)]);
    }
    for (int i = 0; i < value.scale; ++i) {
        magnitude /= 10.0;
    }
    return value.negative ? -magnitude : magnitude;
}

[[nodiscard]] std::string render_decimal(const Decimal& value) {
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

[[nodiscard]] DecimalStringResult parse_decimal_string(const std::string_view text) {
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
[[nodiscard]] std::optional<Decimal> decimal_from_double(const double value) noexcept {
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

// Empty (an uninitialized Variant) and Null (a Variant explicitly holding no
// valid data) are states no other Value alternative can represent. Both are
// reachable only through a Variant-declared variable: Empty is a Variant's
// default value, and Null is produced only by the `Null` keyword or by
// propagation through an expression that already holds Null.
struct Empty {
    [[nodiscard]] friend bool operator==(const Empty&, const Empty&) noexcept { return true; }
};
struct Null {
    [[nodiscard]] friend bool operator==(const Null&, const Null&) noexcept { return true; }
};

// Nothing is the unset state of an object reference. This evaluator does not
// yet support class modules, New, or any other way to produce a non-Nothing
// object value (see REQ-0200's Scope), so Nothing is the only state an
// Object-typed or object-holding-Variant value can ever have.
struct Nothing {
    [[nodiscard]] friend bool operator==(const Nothing&, const Nothing&) noexcept { return true; }
};

// Forward-declared so Value (below) can name it as an alternative; its body,
// which needs Value to already be nameable (std::vector<Value> elements),
// is completed just after Value's declaration.
struct ArrayValue;

// Forward-declared so ObjectInstance (below) can hold a handle to it. Its
// full body (which embeds a Scope, and therefore needs Scope -- and Value --
// already complete) is defined further down, right after Scope itself.
// std::shared_ptr of an incomplete type is fine as a class member: unlike
// std::unique_ptr, its destructor does not require T to be complete at the
// point of declaration.
struct InstanceData;

// A live class-module instance, produced by `New ClassName`. Distinct from
// Nothing (REQ-0200's unset object-reference state): two ObjectInstance
// values are the same object (`Is`) exactly when they share the same
// underlying InstanceData, matching VB6 reference-type identity semantics.
struct ObjectInstance {
    std::shared_ptr<InstanceData> data;

    [[nodiscard]] friend bool operator==(
        const ObjectInstance& left, const ObjectInstance& right) noexcept {
        return left.data == right.data;
    }
};

// A Date value (REQ-0242): an OLE Automation date -- days since
// 1899-12-30 in the integer part, time of day in the fraction.
// An error-subtype Variant (REQ-0259), produced by `CVErr(n)`.
struct ErrorValue {
    std::int32_t code{};

    [[nodiscard]] friend bool operator==(const ErrorValue left, const ErrorValue right) noexcept {
        return left.code == right.code;
    }
};

struct DateValue {
    double serial{};

    [[nodiscard]] friend bool operator==(const DateValue left, const DateValue right) noexcept {
        return left.serial == right.serial;
    }
};

using Value = std::variant<
    Integer, std::string, bool, double, float, Currency, Decimal, Empty, Null, Int16, Nothing,
    ArrayValue, ObjectInstance, DateValue, Byte, ErrorValue>;

// A fixed-size or dynamic, one-dimensional or (fixed-size only) multi-
// dimensional array (`Dim arr(n)`, `Dim arr(lo To hi) As Type`, `Dim
// arr(n, m) As Type`, or `Dim arr() As Type`). For an ordinary 1-D array
// (`dimensions` empty), `lower_bound` is the first valid index and
// `elements.size()` gives the element count, so the last valid index is
// `lower_bound + elements.size() - 1`; for a multi-dimensional array, see
// `dimensions` below instead. All elements share the array's one declared
// element type, enforced at element-assignment time the same way a
// fixed-type scalar variable enforces its type.
struct ArrayValue {
    std::vector<Value> elements;
    Integer lower_bound{};
    // A dynamic array (`Dim arr()`, declared with no bound) can later be
    // `ReDim`/`ReDim Preserve`d; a fixed-size array (`Dim arr(n)`) cannot
    // (REQ-0207). `is_allocated` is false only for a dynamic array before
    // its first `ReDim` -- distinct from a merely zero-length array (for
    // example a `ParamArray` called with no extra arguments), which is
    // allocated and empty at the same time. `element_type_index` remembers
    // the array's declared element type (a `Value::index()`, not the value
    // itself -- storing a `Value` directly here, rather than through a
    // `std::vector`'s indirection, would make `ArrayValue` and `Value`
    // recursively complete-type-dependent on each other) so `ReDim` can
    // refill newly-created slots via `array_element_default`, and so
    // TypeName/VarType/element-type checks have an answer even when
    // `elements` is empty.
    bool is_dynamic{false};
    bool is_allocated{true};
    std::size_t element_type_index{};
    // Present (size >= 2) for a multi-dimensional array, fixed-size
    // (`Dim arr(b1, b2, ...)`, REQ-0210) or dynamic (`ReDim arr(b1, b2,
    // ...)`, REQ-0219); empty for an ordinary 1-D array, which continues
    // to use `lower_bound`/`elements.size()` alone. Each entry is one
    // dimension's [lower, upper] bound, outermost dimension first;
    // `elements` is laid out in row-major order (the last dimension
    // varies fastest), matching real VB6's `For Each` iteration order over
    // a multi-dimensional array.
    std::vector<std::pair<Integer, Integer>> dimensions{};
    // `Dim arr(...) As Variant`/`As Object` (REQ-0212): an element retypes
    // freely on plain assignment (`is_variant_element`, the per-element
    // analogue of a scalar Variant's `variant_variables` membership) or
    // must be an object reference assigned only via `Set`
    // (`is_object_element`, the per-element analogue of
    // `object_variables`). Mutually exclusive with each other; when either
    // is set, `element_type_index` is not a fixed per-element type and is
    // not consulted for element reads/writes (only `array_element_default`
    // still uses it, to seed new slots with `Empty`/`Nothing`).
    bool is_variant_element{false};
    bool is_object_element{false};
    // `Dim arr(...) As SomeClass` (REQ-0214): non-empty only when
    // `is_object_element` is also set, naming the specific class every
    // `Set arr(i) = ...` source must match exactly (`WFC0137` otherwise) --
    // the array-element analogue of a scalar `As ClassName` variable's
    // `Scope::object_class_names` entry. Empty means the generic `As
    // Object` form: any class (or `Nothing`) is accepted.
    std::string element_class_name{};
    // Only meaningful while `is_dynamic` and not yet `is_allocated`
    // (REQ-0219): the array's dimension count, fixed in advance by a
    // comma-only declaration (`Dim arr(,) As Type` is 2, `Dim arr(,,) As
    // Type` is 3, ...), or `0` if the plain `Dim arr()` form left the
    // count unconstrained until the first `ReDim` decides it. Once
    // allocated, the array's actual current dimension count
    // (`dimensions.empty() ? 1 : dimensions.size()`) is authoritative
    // instead, and every subsequent `ReDim` must match it exactly --
    // real VB6 never lets a later `ReDim` change how many dimensions an
    // array has, only their bounds. Appended as the struct's last field
    // (rather than nearer `dimensions`, which it conceptually belongs
    // beside) so every existing positional aggregate-init call site that
    // predates it keeps compiling unchanged.
    std::size_t dynamic_dimension_count{0};
    // True when `Dim a(,)` fixed the dimension count in the declaration (a later
    // ReDim may not change it); a plain `Dim a()` leaves it free.
    bool dimension_count_declared{false};
    // `Dim a(n) As String * k`: every element is padded/truncated to k characters.
    std::size_t element_fixed_length{0};

    [[nodiscard]] friend bool operator==(
        const ArrayValue& left, const ArrayValue& right) noexcept {
        return left.lower_bound == right.lower_bound && left.elements == right.elements &&
               left.dimensions == right.dimensions;
    }
};

// The default (zero) value for one of the fixed scalar types a Dim'd
// array's elements may hold, keyed by that type's `Value::index()`.
// Mirrors the same seven-type list `parse_declaration`'s array branch
// already accepts, plus `Empty`/`Nothing` for a Variant-/Object-element
// array's own `element_type_index` (REQ-0212; `Empty`/`Nothing` never
// collide with any fixed scalar array's element type, so this is
// unambiguous). Falls back to Integer (unreachable for a legitimately-
// typed array) rather than asserting, matching this codebase's general
// preference for a safe placeholder over a crash when a value is only
// needed for its type.
[[nodiscard]] inline Value array_element_default(const std::size_t type_index) noexcept {
    if (type_index == Value{std::string{}}.index()) {
        return Value{std::string{}};
    }
    if (type_index == Value{bool{}}.index()) {
        return Value{false};
    }
    if (type_index == Value{double{}}.index()) {
        return Value{0.0};
    }
    if (type_index == Value{float{}}.index()) {
        return Value{0.0f};
    }
    if (type_index == Value{Currency{}}.index()) {
        return Value{Currency{}};
    }
    if (type_index == Value{Int16{}}.index()) {
        return Value{Int16{}};
    }
    if (type_index == Value{DateValue{}}.index()) {
        return Value{DateValue{}};
    }
    if (type_index == Value{Byte{}}.index()) {
        return Value{Byte{}};
    }
    if (type_index == Value{Decimal{}}.index()) {
        return Value{Decimal{}};
    }
    if (type_index == Value{Empty{}}.index()) {
        return Value{Empty{}};
    }
    if (type_index == Value{Nothing{}}.index()) {
        return Value{Nothing{}};
    }
    return Value{Integer{}};
}

struct NumericStringResult {
    NumericStringStatus status{NumericStringStatus::malformed};
    double value{};
};

// Parse the strict, locale-independent numeric String form shared by the VBA
// conversion intrinsics. Surrounding ASCII whitespace and a leading plus are
// accepted; the entire remaining decimal/exponent spelling must be consumed.
[[nodiscard]] NumericStringResult parse_numeric_string(const std::string_view text) {
    std::size_t first{};
    std::size_t last = text.size();
    const auto is_ascii_whitespace = [](const char character) {
        return character == ' ' || character == '\t' || character == '\r' ||
               character == '\n';
    };
    while (first < last && is_ascii_whitespace(text[first])) {
        ++first;
    }
    while (last > first && is_ascii_whitespace(text[last - 1U])) {
        --last;
    }
    if (first < last && text[first] == '+') {
        ++first;
    }
    if (first == last) {
        return {};
    }

    // `&H1F` / `&O17` (optionally `&`-suffixed): VB converts these too.
    if (last - first > 2U && text[first] == '&' &&
        (text[first + 1U] == 'h' || text[first + 1U] == 'H' || text[first + 1U] == 'o' ||
         text[first + 1U] == 'O')) {
        const bool hex = text[first + 1U] == 'h' || text[first + 1U] == 'H';
        std::size_t end = last;
        if (text[end - 1U] == '&') --end;
        std::uint64_t magnitude = 0;
        for (std::size_t i = first + 2U; i < end; ++i) {
            const char c = text[i];
            int digit = -1;
            if (c >= '0' && c <= '9') digit = c - '0';
            else if (hex && c >= 'a' && c <= 'f') digit = c - 'a' + 10;
            else if (hex && c >= 'A' && c <= 'F') digit = c - 'A' + 10;
            if (digit < 0 || (!hex && digit > 7) || magnitude > 0xFFFFFFFFULL) return {};
            magnitude = magnitude * (hex ? 16U : 8U) + static_cast<std::uint64_t>(digit);
        }
        if (magnitude > 0xFFFFFFFFULL) return {};
        // 4 or fewer hex digits is an Integer (sign-extended from 16 bits).
        const bool short_form = text[end - 1U] != '&' && (end - first - 2U) <= (hex ? 4U : 6U) &&
            magnitude <= 0xFFFFULL && text[last - 1U] != '&';
        double result = short_form ? static_cast<double>(static_cast<std::int16_t>(magnitude))
                                   : static_cast<double>(static_cast<std::int32_t>(magnitude));
        return {NumericStringStatus::valid, result};
    }
    // Thousands separators between digits of the integer part: `1,000`.
    std::string stripped;
    if (text.substr(first, last - first).find(',') != std::string_view::npos) {
        bool in_integer_part = true;
        for (std::size_t i = first; i < last; ++i) {
            const char c = text[i];
            if (c == '.' || c == 'e' || c == 'E') in_integer_part = false;
            if (c == ',' && in_integer_part && i > first && i + 1U < last &&
                std::isdigit(static_cast<unsigned char>(text[i - 1U])) != 0 &&
                std::isdigit(static_cast<unsigned char>(text[i + 1U])) != 0) {
                continue;
            }
            stripped.push_back(c);
        }
        if (stripped.find(',') != std::string::npos) {
            return {};
        }
        return parse_numeric_string(stripped);
    }

    double value{};
    const auto conversion =
        std::from_chars(text.data() + first, text.data() + last, value);
    if (conversion.ec == std::errc::result_out_of_range) {
        return {NumericStringStatus::out_of_range, value};
    }
    if (conversion.ec != std::errc{} || conversion.ptr != text.data() + last ||
        !std::isfinite(value)) {
        return {};
    }
    return {NumericStringStatus::valid, value};
}

// A value participates in numeric operators when it is a Long, Currency,
// Single, Decimal, or Double. Empty and Null are deliberately excluded:
// they need their own coercion/propagation handling (Empty coerces to a
// context-dependent zero; Null propagates rather than widening) rather than
// being treated as plain numbers.
[[nodiscard]] inline bool is_number(const Value& value) noexcept {
    return std::holds_alternative<Integer>(value) ||
           std::holds_alternative<Int16>(value) ||
           std::holds_alternative<Byte>(value) ||
           std::holds_alternative<Currency>(value) ||
           std::holds_alternative<float>(value) ||
           std::holds_alternative<Decimal>(value) ||
           std::holds_alternative<double>(value);
}

// A whole-number value (Long, Integer or Byte) as a Long.
[[nodiscard]] inline std::optional<Integer> whole_value(const Value& value) noexcept {
    if (const auto* integer = std::get_if<Integer>(&value)) return *integer;
    if (const auto* short_integer = std::get_if<Int16>(&value)) return static_cast<Integer>(*short_integer);
    if (const auto* byte = std::get_if<Byte>(&value)) return static_cast<Integer>(*byte);
    return std::nullopt;
}

// An object reference: either Nothing (REQ-0200's unset state) or a live
// `New`-produced class instance (REQ-0203). Every place that used to check
// only for Nothing, back when Nothing was the only object-reference value
// this evaluator could produce, now checks for both.
[[nodiscard]] inline bool is_object_reference(const Value& value) noexcept {
    return std::holds_alternative<Nothing>(value) || std::holds_alternative<ObjectInstance>(value);
}

// Widen a Long, Currency, Single, Decimal, or Double value to double for
// mixed-type numeric evaluation. A Currency or Decimal value's exact
// magnitude can exceed what double represents exactly; callers that need
// exact results use the dedicated scaled-integer/big-integer arithmetic
// instead.
[[nodiscard]] inline double as_double(const Value& value) noexcept {
    if (const auto* integer = std::get_if<Integer>(&value)) {
        return static_cast<double>(*integer);
    }
    if (const auto* short_integer = std::get_if<Int16>(&value)) {
        return static_cast<double>(*short_integer);
    }
    if (const auto* byte = std::get_if<Byte>(&value)) {
        return static_cast<double>(*byte);
    }
    if (const auto* single = std::get_if<float>(&value)) {
        return static_cast<double>(*single);
    }
    if (const auto* currency = std::get_if<Currency>(&value)) {
        return static_cast<double>(currency->scaled) / 10000.0;
    }
    if (const auto* decimal = std::get_if<Decimal>(&value)) {
        return decimal_to_double(*decimal);
    }
    return std::get<double>(value);
}

// Widen a Long, Currency, or Single value to float for same-precision
// numeric evaluation. Callers must first establish that neither operand is
// Double or Decimal. A Currency operand widens through its exact decimal
// value (see as_double), matching VB6's Currency-to-Single promotion.
[[nodiscard]] inline float as_single(const Value& value) noexcept {
    if (const auto* integer = std::get_if<Integer>(&value)) {
        return static_cast<float>(*integer);
    }
    if (const auto* short_integer = std::get_if<Int16>(&value)) {
        return static_cast<float>(*short_integer);
    }
    if (const auto* byte = std::get_if<Byte>(&value)) {
        return static_cast<float>(*byte);
    }
    if (std::holds_alternative<Currency>(value)) {
        return static_cast<float>(as_double(value));
    }
    return std::get<float>(value);
}

// Widen a Long, Int16, or Currency value to a Currency scaled int64 for
// exact fixed-point evaluation. Callers must first establish that neither
// operand is Single or Double.
[[nodiscard]] inline std::int64_t as_currency_scaled(const Value& value) noexcept {
    if (const auto* integer = std::get_if<Integer>(&value)) {
        return static_cast<std::int64_t>(*integer) * 10000;
    }
    if (const auto* short_integer = std::get_if<Int16>(&value)) {
        return static_cast<std::int64_t>(*short_integer) * 10000;
    }
    if (const auto* byte = std::get_if<Byte>(&value)) {
        return static_cast<std::int64_t>(*byte) * 10000;
    }
    return std::get<Currency>(value).scaled;
}

// VB6's numeric promotion order for mixed-type arithmetic: Integer widens to
// Long, which widens to Currency, which widens to Single, which widens to
// Double, which widens to Decimal. Decimal dominates every other type,
// including Double, verified directly against a local VB6 6.00.8176
// reference probe: `CDec(1) + 1234567.89` (a Double literal) returns a
// Decimal `1234568.89`, and the same holds for Decimal vs Currency and
// Decimal vs Long. This corrects an earlier, unverified assumption that
// Double's larger representable magnitude made it dominant; real VB6 instead
// keeps the operation in Decimal's exact fixed-point representation whenever
// either operand is Decimal.
enum class NumericCategory {
    int16,
    integer,
    currency,
    single,
    double_precision,
    decimal_precision
};

[[nodiscard]] inline NumericCategory numeric_category(const Value& value) noexcept {
    if (std::holds_alternative<double>(value)) {
        return NumericCategory::double_precision;
    }
    if (std::holds_alternative<Decimal>(value)) {
        return NumericCategory::decimal_precision;
    }
    if (std::holds_alternative<float>(value)) {
        return NumericCategory::single;
    }
    if (std::holds_alternative<Currency>(value)) {
        return NumericCategory::currency;
    }
    if (std::holds_alternative<Int16>(value) || std::holds_alternative<Byte>(value)) {
        return NumericCategory::int16;
    }
    return NumericCategory::integer;
}

// The VB6 TypeName for a scalar array element (never Decimal/Empty/Null/
// Nothing/ArrayValue, since array element types in this evaluator's scope
// are restricted to Long/Integer/Double/Single/Currency/String/Boolean).
[[nodiscard]] inline std::string element_type_name(const Value& element) {
    if (std::holds_alternative<Integer>(element)) {
        return "Long";
    }
    if (std::holds_alternative<Int16>(element)) {
        return "Integer";
    }
    if (std::holds_alternative<double>(element)) {
        return "Double";
    }
    if (std::holds_alternative<float>(element)) {
        return "Single";
    }
    if (std::holds_alternative<Currency>(element)) {
        return "Currency";
    }
    if (std::holds_alternative<Byte>(element)) {
        return "Byte";
    }
    if (std::holds_alternative<bool>(element)) {
        return "Boolean";
    }
    return "String";
}

// The VarType code for a scalar array element, mirroring element_type_name.
[[nodiscard]] inline Integer element_vartype_code(const Value& element) {
    if (std::holds_alternative<Integer>(element)) {
        return 3;
    }
    if (std::holds_alternative<Int16>(element)) {
        return 2;
    }
    if (std::holds_alternative<double>(element)) {
        return 5;
    }
    if (std::holds_alternative<float>(element)) {
        return 4;
    }
    if (std::holds_alternative<Currency>(element)) {
        return 6;
    }
    if (std::holds_alternative<Byte>(element)) {
        return 17;
    }
    if (std::holds_alternative<bool>(element)) {
        return 11;
    }
    return 8;
}


// ---- Date support (REQ-0242) ----------------------------------------------

[[nodiscard]] inline std::int64_t days_from_civil(
    std::int64_t year, const std::int64_t month, const std::int64_t day) noexcept {
    year -= month <= 2 ? 1 : 0;
    const std::int64_t era = (year >= 0 ? year : year - 399) / 400;
    const std::int64_t year_of_era = year - era * 400;
    const std::int64_t day_of_year = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const std::int64_t day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
    return era * 146097 + day_of_era - 719468;
}

inline void civil_from_days(
    std::int64_t days, std::int64_t& year, std::int64_t& month, std::int64_t& day) noexcept {
    days += 719468;
    const std::int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    const std::int64_t day_of_era = days - era * 146097;
    const std::int64_t year_of_era =
        (day_of_era - day_of_era / 1460 + day_of_era / 36524 - day_of_era / 146096) / 365;
    year = year_of_era + era * 400;
    const std::int64_t day_of_year = day_of_era - (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
    const std::int64_t mp = (5 * day_of_year + 2) / 153;
    day = day_of_year - (153 * mp + 2) / 5 + 1;
    month = mp < 10 ? mp + 3 : mp - 9;
    year += month <= 2 ? 1 : 0;
}

constexpr std::int64_t kDateEpochOffset = 25569;  // 1970-01-01 as an OLE serial

[[nodiscard]] inline bool is_leap_year(const std::int64_t year) noexcept {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

[[nodiscard]] inline std::int64_t days_in_month(
    const std::int64_t year, const std::int64_t month) noexcept {
    constexpr std::int64_t lengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return month == 2 && is_leap_year(year) ? 29 : lengths[month - 1];
}

// Serial for a (possibly out-of-range, e.g. month 13 / day 0) y-m-d.
[[nodiscard]] inline double date_serial(
    std::int64_t year, std::int64_t month, const std::int64_t day) noexcept {
    year += (month - 1 >= 0 ? (month - 1) / 12 : -((12 - month) / 12));
    month = ((month - 1) % 12 + 12) % 12 + 1;
    return static_cast<double>(days_from_civil(year, month, 1) + (day - 1) + kDateEpochOffset);
}

struct DateParts {
    std::int64_t year{}, month{}, day{}, hour{}, minute{}, second{}, weekday{};  // weekday: 1 = Sunday
};

[[nodiscard]] inline DateParts split_date(const double serial) noexcept {
    double whole = std::floor(serial);
    std::int64_t seconds = static_cast<std::int64_t>(std::llround((serial - whole) * 86400.0));
    if (seconds >= 86400) {
        seconds -= 86400;
        whole += 1.0;
    }
    DateParts parts;
    const auto days = static_cast<std::int64_t>(whole);
    civil_from_days(days - kDateEpochOffset, parts.year, parts.month, parts.day);
    parts.hour = seconds / 3600;
    parts.minute = seconds % 3600 / 60;
    parts.second = seconds % 60;
    parts.weekday = ((days % 7) + 7 + 6) % 7 + 1;
    return parts;
}

// DatePart "ww": the week number of `serial` for a week starting on `first_day` (1 = Sunday)
// under `first_week` (1 = week containing Jan 1, 2 = first week with four days, 3 = first full week).
[[nodiscard]] inline std::int64_t vb_week_of_year(
    const double serial, const std::int64_t first_day, const std::int64_t first_week) noexcept {
    const double day = std::floor(serial);
    const auto parts = split_date(day);
    const auto week1_start = [&](const std::int64_t year) {
        const double jan1 = date_serial(year, 1, 1);
        const auto offset = (split_date(jan1).weekday - first_day + 7) % 7;
        const double week_start = jan1 - static_cast<double>(offset);
        if (first_week == 3) return offset == 0 ? jan1 : week_start + 7.0;
        if (first_week == 2) return (7 - offset) >= 4 ? week_start : week_start + 7.0;
        return week_start;
    };
    double start = week1_start(parts.year);
    if (first_week != 1 && day < start) {
        start = week1_start(parts.year - 1);
    } else if (first_week == 2 && day >= week1_start(parts.year + 1)) {
        return 1;
    }
    return static_cast<std::int64_t>(std::floor((day - start) / 7.0)) + 1;
}

[[nodiscard]] inline std::string render_time_part(const DateParts& parts) {
    const auto hour12 = parts.hour % 12 == 0 ? 12 : parts.hour % 12;
    char buffer[32];
    std::snprintf(
        buffer, sizeof(buffer), "%lld:%02lld:%02lld %s", static_cast<long long>(hour12),
        static_cast<long long>(parts.minute), static_cast<long long>(parts.second),
        parts.hour < 12 ? "AM" : "PM");
    return buffer;
}

[[nodiscard]] inline std::string render_date_part(const DateParts& parts) {
    return std::to_string(parts.month) + "/" + std::to_string(parts.day) + "/" +
           std::to_string(parts.year);
}

// VB6 "General Date" (US locale): date, time, or both.
[[nodiscard]] inline std::string render_date(const double serial) {
    const auto parts = split_date(serial);
    const bool has_time = parts.hour != 0 || parts.minute != 0 || parts.second != 0;
    const bool has_date = std::floor(serial) != 0.0;
    if (has_date && has_time) {
        return render_date_part(parts) + " " + render_time_part(parts);
    }
    if (has_time || !has_date) {
        return render_time_part(parts);
    }
    return render_date_part(parts);
}

// Parses "m/d/yyyy", "yyyy-mm-dd" or "yyyy/m/d" (2-digit years: 00-29 ->
// 2000s, 30-99 -> 1900s), each optionally followed by "h:mm[:ss][ AM|PM]",
// or a time alone. Returns the serial, or nullopt when malformed.
[[nodiscard]] inline std::optional<double> parse_date_text(const std::string_view text) {
    std::size_t i = 0;
    const auto skip_space = [&] {
        while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) ++i;
    };
    const auto read_number = [&](std::int64_t& value, std::size_t& digits) {
        digits = 0;
        value = 0;
        while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
            value = value * 10 + (text[i] - '0');
            ++i;
            ++digits;
        }
        return digits > 0;
    };
    skip_space();
    double result = 0.0;
    bool have_date = false;
    const auto start = i;
    std::int64_t a{}, b{}, c{};
    std::size_t da{}, db{}, dc{};
    // Month names: "March 5, 2024", "Fri, Mar 15 2024", "5 March 2024", "5-Mar-24".
    const auto read_word = [&]() {
        std::string word;
        while (i < text.size() && std::isalpha(static_cast<unsigned char>(text[i])) != 0) {
            word.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(text[i]))));
            ++i;
        }
        return word;
    };
    const auto month_from_word = [](const std::string& word) -> std::int64_t {
        static const char* const names[] = {"january", "february", "march", "april", "may", "june",
            "july", "august", "september", "october", "november", "december"};
        if (word.size() < 3U) return 0;
        for (std::int64_t m = 0; m < 12; ++m) {
            const std::string_view name = names[m];
            if (word == name || (word.size() == 3U && name.substr(0, 3) == word)) return m + 1;
        }
        return 0;
    };
    const auto skip_separators = [&] {
        while (i < text.size() && (text[i] == ' ' || text[i] == '\t' || text[i] == ',' ||
                                   text[i] == '-' || text[i] == '.')) {
            ++i;
        }
    };
    const auto finish_named = [&](std::int64_t month, std::int64_t day, std::int64_t year,
                                  const bool have_year) -> bool {
        if (!have_year) year = 2000;  // no year given: a fixed year keeps parsing deterministic
        if (year < 100) year += year < 30 ? 2000 : 1900;
        if (month < 1 || day < 1 || year < 100 || year > 9999 || day > days_in_month(year, month)) {
            return false;
        }
        result = date_serial(year, month, day);
        have_date = true;
        return true;
    };
    if (i < text.size() && std::isalpha(static_cast<unsigned char>(text[i])) != 0) {
        auto word = read_word();
        static const char* const weekdays[] = {"sunday", "monday", "tuesday", "wednesday",
            "thursday", "friday", "saturday"};
        for (const char* weekday : weekdays) {
            const std::string_view name = weekday;
            if (word == name || (word.size() == 3U && name.substr(0, 3) == word)) {
                skip_separators();
                word = read_word();
                break;
            }
        }
        const auto month = month_from_word(word);
        if (month == 0) return std::nullopt;
        skip_separators();
        std::int64_t first{}, second{};
        std::size_t first_digits{}, second_digits{};
        if (!read_number(first, first_digits)) {
            return std::nullopt;
        }
        std::int64_t day = first, year = 0;
        bool have_year = false;
        skip_separators();
        const auto before_next = i;
        if (read_number(second, second_digits) && !(i < text.size() && text[i] == ':')) {
            year = second;
            have_year = true;
        } else {
            i = before_next;
            if (first_digits > 2U) {  // "March 2024"
                year = first;
                day = 1;
                have_year = true;
            }
        }
        if (!finish_named(month, day, year, have_year)) return std::nullopt;
    } else if (read_number(a, da) && i < text.size() &&
               (text[i] == ' ' || text[i] == '-') && [&] {
                   const auto save = i;
                   skip_separators();
                   const bool alpha = i < text.size() &&
                                      std::isalpha(static_cast<unsigned char>(text[i])) != 0;
                   i = save;
                   return alpha;
               }()) {
        // "5 March 2024" / "5-Mar-24"
        skip_separators();
        const auto month = month_from_word(read_word());
        if (month == 0) return std::nullopt;
        skip_separators();
        std::int64_t year{};
        std::size_t year_digits{};
        const auto before_year = i;
        bool have_year = false;
        if (read_number(year, year_digits) && !(i < text.size() && text[i] == ':')) {
            have_year = true;
        } else {
            i = before_year;
        }
        if (!finish_named(month, a, year, have_year)) return std::nullopt;
    } else if (da > 0 && i < text.size() && (text[i] == '/' || text[i] == '-')) {
        const char separator = text[i++];
        if (!read_number(b, db) || i >= text.size() || text[i] != separator) {
            return std::nullopt;
        }
        ++i;
        if (!read_number(c, dc)) {
            return std::nullopt;
        }
        std::int64_t year{}, month{}, day{};
        if (da >= 3) {
            year = a; month = b; day = c;
        } else {
            month = a; day = b; year = c;
            if (dc <= 2) year += year < 30 ? 2000 : 1900;
        }
        if (month < 1 || month > 12 || day < 1 || year < 100 || year > 9999 ||
            day > days_in_month(year, month)) {
            return std::nullopt;
        }
        result = date_serial(year, month, day);
        have_date = true;
    } else {
        i = start;
    }
    skip_space();
    if (i < text.size()) {
        std::int64_t hour{}, minute{}, second{};
        std::size_t digits{};
        if (!read_number(hour, digits) || i >= text.size() || text[i] != ':') {
            return std::nullopt;
        }
        ++i;
        if (!read_number(minute, digits)) {
            return std::nullopt;
        }
        if (i < text.size() && text[i] == ':') {
            ++i;
            if (!read_number(second, digits)) {
                return std::nullopt;
            }
        }
        skip_space();
        if (i + 1 < text.size() + 0 && i + 2 <= text.size()) {
            const char m0 = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
            const char m1 = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i + 1])));
            if ((m0 == 'a' || m0 == 'p') && m1 == 'm') {
                if (hour < 1 || hour > 12) return std::nullopt;
                hour = hour % 12 + (m0 == 'p' ? 12 : 0);
                i += 2;
            }
        }
        skip_space();
        if (i != text.size() || hour > 23 || minute > 59 || second > 59) {
            return std::nullopt;
        }
        result += static_cast<double>(hour * 3600 + minute * 60 + second) / 86400.0;
    } else if (!have_date) {
        return std::nullopt;
    }
    return result;
}


// Custom/named date format for Format(date, style) (REQ-0242).
[[nodiscard]] inline std::string format_date_pattern(const double serial, const std::string& style) {
    static const char* const month_names[] = {"January", "February", "March", "April",
        "May", "June", "July", "August", "September", "October", "November", "December"};
    static const char* const day_names[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday"};
    const auto parts = split_date(serial);
    std::string lowered;
    for (const char c : style) lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    const auto pad2 = [](const std::int64_t v) {
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%02lld", static_cast<long long>(v));
        return std::string(buffer);
    };
    if (lowered == "general date" || style.empty()) return render_date(serial);
    if (lowered == "long date") {
        return std::string(day_names[parts.weekday - 1]) + ", " + month_names[parts.month - 1] +
               " " + std::to_string(parts.day) + ", " + std::to_string(parts.year);
    }
    if (lowered == "medium date") {
        return pad2(parts.day) + "-" + std::string(month_names[parts.month - 1]).substr(0, 3) +
               "-" + pad2(parts.year % 100);
    }
    if (lowered == "short date") return render_date_part(parts);
    if (lowered == "long time") return render_time_part(parts);
    if (lowered == "medium time") {
        const auto h12 = parts.hour % 12 == 0 ? 12 : parts.hour % 12;
        return pad2(h12) + ":" + pad2(parts.minute) + " " + (parts.hour < 12 ? "AM" : "PM");
    }
    if (lowered == "short time") return pad2(parts.hour) + ":" + pad2(parts.minute);
    const auto hour12 = parts.hour % 12 == 0 ? 12 : parts.hour % 12;
    const bool has_ampm = lowered.find("am/pm") != std::string::npos ||
                          lowered.find("ampm") != std::string::npos ||
                          lowered.find("a/p") != std::string::npos;
    std::string out;
    std::size_t i = 0;
    const auto run_length = [&](const char c) {
        std::size_t n = 0;
        while (i + n < lowered.size() && lowered[i + n] == c) ++n;
        return n;
    };
    // Whether a `m` token at lowered[pos] means minutes (follows h, precedes s).
    const auto is_minute = [&](const std::size_t pos, const std::size_t len) {
        std::size_t before = pos;
        while (before > 0 && (lowered[before - 1] == ' ' || lowered[before - 1] == ':')) --before;
        if (before > 0 && lowered[before - 1] == 'h') return true;
        std::size_t after = pos + len;
        while (after < lowered.size() && (lowered[after] == ' ' || lowered[after] == ':')) ++after;
        return after < lowered.size() && lowered[after] == 's';
    };
    while (i < style.size()) {
        const char c = lowered[i];
        if (c == '"') {
            ++i;
            while (i < style.size() && style[i] != '"') out.push_back(style[i++]);
            ++i;
        } else if (c == '\\' && i + 1 < style.size()) {
            out.push_back(style[i + 1]);
            i += 2;
        } else if (c == 'y') {
            const auto n = run_length('y');
            if (n == 1) {
                out += std::to_string(
                    static_cast<std::int64_t>(serial - date_serial(parts.year, 1, 1)) + 1);
            } else {
                out += n >= 3 ? std::to_string(parts.year) : pad2(parts.year % 100);
            }
            i += n;
        } else if (c == 'w') {
            const auto n = run_length('w');
            if (n == 1) {
                out += std::to_string(parts.weekday);
            } else {
                const auto doy = static_cast<std::int64_t>(std::floor(serial) - date_serial(parts.year, 1, 1)) + 1;
                const auto jan1_weekday = split_date(date_serial(parts.year, 1, 1)).weekday;
                out += std::to_string((doy - 1 + jan1_weekday - 1) / 7 + 1);
            }
            i += n;
        } else if (c == 'q') {
            out += std::to_string((parts.month - 1) / 3 + 1);
            ++i;
        } else if (c == 'm') {
            const auto n = run_length('m');
            if (is_minute(i, n)) {
                out += n >= 2 ? pad2(parts.minute) : std::to_string(parts.minute);
            } else if (n == 1) out += std::to_string(parts.month);
            else if (n == 2) out += pad2(parts.month);
            else if (n == 3) out += std::string(month_names[parts.month - 1]).substr(0, 3);
            else out += month_names[parts.month - 1];
            i += n;
        } else if (c == 'd' && lowered.compare(i, 6, "dddddd") == 0) {
            out += format_date_pattern(serial, "Long Date");
            i += 6;
        } else if (c == 'd' && lowered.compare(i, 5, "ddddd") == 0) {
            out += render_date_part(parts);
            i += 5;
        } else if (c == 'd') {
            const auto n = run_length('d');
            if (n == 1) out += std::to_string(parts.day);
            else if (n == 2) out += pad2(parts.day);
            else if (n == 3) out += std::string(day_names[parts.weekday - 1]).substr(0, 3);
            else out += day_names[parts.weekday - 1];
            i += n;
        } else if (c == 'h') {
            const auto n = run_length('h');
            const auto h = has_ampm ? hour12 : parts.hour;
            out += n >= 2 ? pad2(h) : std::to_string(h);
            i += n;
        } else if (c == 'n') {
            const auto n = run_length('n');
            out += n >= 2 ? pad2(parts.minute) : std::to_string(parts.minute);
            i += n;
        } else if (c == 's') {
            const auto n = run_length('s');
            out += n >= 2 ? pad2(parts.second) : std::to_string(parts.second);
            i += n;
        } else if (lowered.compare(i, 5, "am/pm") == 0) {
            out += parts.hour < 12 ? (style[i] == 'a' ? "am" : "AM") : (style[i] == 'a' ? "pm" : "PM");
            i += 5;
        } else if (lowered.compare(i, 4, "ampm") == 0) {
            out += parts.hour < 12 ? "AM" : "PM";
            i += 4;
        } else if (lowered.compare(i, 3, "a/p") == 0) {
            out += parts.hour < 12 ? (style[i] == 'a' ? "a" : "A") : (style[i] == 'a' ? "p" : "P");
            i += 3;
        } else if (lowered.compare(i, 5, "ttttt") == 0) {
            out += render_time_part(parts);
            i += 5;
        } else {
            out.push_back(style[i]);
            ++i;
        }
    }
    return out;
}

// Kleene three-valued logic for the logical operators: nullopt represents
// Null (unknown); a definite Boolean is represented as its own value. These
// tables are verified against the local VB6 6.00.8176 reference for And, Or,
// and Not; Xor/Eqv/Imp are derived algebraically from those verified
// primitives (Eqv = Not(Xor(a,b)), Imp = Not(a) Or b), which VBA's own
// documentation defines them as.
[[nodiscard]] inline std::optional<bool> ternary_and(
    const std::optional<bool> left,
    const std::optional<bool> right) noexcept {
    if (left == false || right == false) {
        return false;
    }
    if (left.has_value() && right.has_value()) {
        return true;
    }
    return std::nullopt;
}

[[nodiscard]] inline std::optional<bool> ternary_or(
    const std::optional<bool> left,
    const std::optional<bool> right) noexcept {
    if (left == true || right == true) {
        return true;
    }
    if (left.has_value() && right.has_value()) {
        return false;
    }
    return std::nullopt;
}

[[nodiscard]] inline std::optional<bool> ternary_not(const std::optional<bool> value) noexcept {
    if (!value.has_value()) {
        return std::nullopt;
    }
    return !*value;
}

[[nodiscard]] inline std::optional<bool> ternary_xor(
    const std::optional<bool> left,
    const std::optional<bool> right) noexcept {
    if (left.has_value() && right.has_value()) {
        return *left != *right;
    }
    return std::nullopt;
}

[[nodiscard]] inline std::optional<bool> ternary_eqv(
    const std::optional<bool> left,
    const std::optional<bool> right) noexcept {
    return ternary_not(ternary_xor(left, right));
}

[[nodiscard]] inline std::optional<bool> ternary_imp(
    const std::optional<bool> left,
    const std::optional<bool> right) noexcept {
    return ternary_or(ternary_not(left), right);
}

[[nodiscard]] char ascii_lower(const char character) noexcept {
    if (character >= 'A' && character <= 'Z') {
        return static_cast<char>(character + ('a' - 'A'));
    }
    return character;
}

[[nodiscard]] char ascii_upper(const char character) noexcept {
    if (character >= 'a' && character <= 'z') {
        return static_cast<char>(character - ('a' - 'A'));
    }
    return character;
}

// String model: a VB String is a sequence of UTF-16 code units. WFC stores it as
// UTF-8 (so I/O and source text need no conversion) and counts, slices and
// searches by UTF-16 unit. Bytes that are not valid UTF-8 count as one unit each
// (their Latin-1 value). ASCII-only text takes the plain byte paths.
[[nodiscard]] inline bool is_ascii_text(const std::string_view text) noexcept {
    for (const char character : text) {
        if (static_cast<unsigned char>(character) >= 0x80U) return false;
    }
    return true;
}

[[nodiscard]] inline std::u16string to_utf16_units(const std::string_view text) {
    std::u16string units;
    units.reserve(text.size());
    std::size_t i = 0;
    while (i < text.size()) {
        const auto lead = static_cast<unsigned char>(text[i]);
        std::size_t length = lead < 0x80U ? 1U : (lead >= 0xF0U && lead < 0xF8U) ? 4U
                             : (lead >= 0xE0U && lead < 0xF0U) ? 3U : (lead >= 0xC2U && lead < 0xE0U) ? 2U : 0U;
        bool valid = length != 0 && i + length <= text.size();
        std::uint32_t code = length == 1U ? lead : length == 2U ? (lead & 0x1FU)
                             : length == 3U ? (lead & 0x0FU) : (lead & 0x07U);
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

[[nodiscard]] inline std::string from_utf16_units(const std::u16string_view units) {
    std::string out;
    out.reserve(units.size());
    for (std::size_t i = 0; i < units.size(); ++i) {
        std::uint32_t code = units[i];
        if (code >= 0xD800U && code < 0xDC00U && i + 1U < units.size() &&
            units[i + 1U] >= 0xDC00U && units[i + 1U] < 0xE000U) {
            code = 0x10000U + ((code - 0xD800U) << 10U) + (units[i + 1U] - 0xDC00U);
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

// Pads with spaces or truncates `text` to exactly `units` UTF-16 units (a fixed-length String).
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
[[nodiscard]] inline std::uint32_t ansi_to_unicode(const unsigned code) noexcept {
    static constexpr std::uint16_t table[32] = {
        0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160,
        0x2039, 0x0152, 0x008D, 0x017D, 0x008F, 0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022,
        0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178};
    return code >= 0x80U && code <= 0x9FU ? table[code - 0x80U] : code;
}

// File text is ANSI (Windows-1252) on disk; Strings are Unicode in memory.
[[nodiscard]] inline std::string ansi_bytes_to_text(const std::string& bytes) {
    if (is_ascii_text(bytes)) return bytes;
    std::string text;
    text.reserve(bytes.size() + 8U);
    for (const char byte : bytes) {
        append_utf8_unit(text, ansi_to_unicode(static_cast<unsigned char>(byte)));
    }
    return text;
}

// The Windows-1252 byte for a character, or '?' when it has none.
[[nodiscard]] inline unsigned unicode_to_ansi(const std::uint32_t code) noexcept {
    if (code < 0x80U || (code >= 0xA0U && code <= 0xFFU)) return code;
    for (unsigned byte = 0x80U; byte <= 0x9FU; ++byte) {
        if (ansi_to_unicode(byte) == code) return byte;
    }
    return static_cast<unsigned>('?');
}

[[nodiscard]] inline std::string text_to_ansi_bytes(const std::string& text) {
    if (is_ascii_text(text)) return text;
    std::string bytes;
    for (const char16_t unit : to_utf16_units(text)) {
        bytes.push_back(static_cast<char>(unicode_to_ansi(unit)));
    }
    return bytes;
}

// Latin Extended-A letters pair upper/lower as even/odd, except two runs where
// the odd one is the capital.
[[nodiscard]] inline bool latin_ext_a_letter(const char16_t c) noexcept {
    return c >= 0x100U && c <= 0x17FU && c != 0x130U && c != 0x131U && c != 0x138U &&
           c != 0x149U && c != 0x178U && c != 0x17FU;
}

[[nodiscard]] inline bool latin_ext_a_odd_upper(const char16_t c) noexcept {
    return (c >= 0x139U && c <= 0x148U) || (c >= 0x179U && c <= 0x17EU);
}

[[nodiscard]] inline char16_t unit_to_lower(const char16_t c) noexcept {
    if (c >= u'A' && c <= u'Z') return static_cast<char16_t>(c + 32);
    if (c >= 0xC0U && c <= 0xDEU && c != 0xD7U) return static_cast<char16_t>(c + 32);
    if (c == 0x178U) return 0xFFU;
    if (c >= 0x391U && c <= 0x3A9U && c != 0x3A2U) return static_cast<char16_t>(c + 32);
    if (c >= 0x410U && c <= 0x42FU) return static_cast<char16_t>(c + 32);
    if (c >= 0x400U && c <= 0x40FU) return static_cast<char16_t>(c + 80);
    if (latin_ext_a_letter(c) && (c % 2U == (latin_ext_a_odd_upper(c) ? 1U : 0U))) {
        return static_cast<char16_t>(c + 1);
    }
    return c;
}

[[nodiscard]] inline char16_t unit_to_upper(const char16_t c) noexcept {
    if (c >= u'a' && c <= u'z') return static_cast<char16_t>(c - 32);
    if (c >= 0xE0U && c <= 0xFEU && c != 0xF7U) return static_cast<char16_t>(c - 32);
    if (c == 0xFFU) return 0x178U;
    if (c >= 0x3B1U && c <= 0x3C9U && c != 0x3C2U) return static_cast<char16_t>(c - 32);
    if (c >= 0x430U && c <= 0x44FU) return static_cast<char16_t>(c - 32);
    if (c >= 0x450U && c <= 0x45FU) return static_cast<char16_t>(c - 80);
    if (latin_ext_a_letter(c) && (c % 2U == (latin_ext_a_odd_upper(c) ? 0U : 1U))) {
        return static_cast<char16_t>(c - 1);
    }
    return c;
}

// Case-fold `text` for a text comparison (ASCII bytes, or full units when non-ASCII).
[[nodiscard]] inline std::string fold_case(const std::string& text) {
    if (is_ascii_text(text)) {
        std::string folded = text;
        for (char& character : folded) character = ascii_lower(character);
        return folded;
    }
    auto units = to_utf16_units(text);
    for (auto& unit : units) unit = unit_to_lower(unit);
    return from_utf16_units(units);
}

[[nodiscard]] bool is_identifier_start(const char character) noexcept {
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z');
}

[[nodiscard]] bool is_identifier_part(const char character) noexcept {
    return is_identifier_start(character) || (character >= '0' && character <= '9') ||
           character == '_';
}

// Source-visible VBA constant enumerations. Each name resolves to the value in
// its `REQ-008x`/`REQ-0089` type-library contract and is a reserved identifier
// so source cannot shadow it. Members are exposed for source compatibility;
// they carry no runtime behavior beyond their integer value.
[[nodiscard]] std::optional<Integer> vba_constant_value(
    const std::string_view identifier) {
    static const std::unordered_map<std::string_view, Integer> table = {
        // VbCompareMethod (REQ-0089)
        {"vbbinarycompare", 0}, {"vbtextcompare", 1}, {"vbdatabasecompare", 2},
        {"vbusecompareoption", -1},
        // ColorConstants (RGB values as Long)
        {"vbblack", 0}, {"vbred", 255}, {"vbgreen", 65280}, {"vbyellow", 65535},
        {"vbblue", 16711680}, {"vbmagenta", 16711935}, {"vbcyan", 16776960},
        {"vbwhite", 16777215},
        // Common VBRUN constants (key codes, shift masks, mouse buttons, Show modes)
        {"vbkeyreturn", 13}, {"vbkeyescape", 27}, {"vbkeyspace", 32}, {"vbkeyback", 8},
        {"vbkeytab", 9}, {"vbkeyleft", 37}, {"vbkeyup", 38}, {"vbkeyright", 39},
        {"vbkeydown", 40}, {"vbkeydelete", 46}, {"vbkeyinsert", 45}, {"vbkeyhome", 36},
        {"vbkeyend", 35}, {"vbkeypageup", 33}, {"vbkeypagedown", 34}, {"vbkeyshift", 16},
        {"vbkeycontrol", 17}, {"vbkeymenu", 18}, {"vbkeyf1", 112}, {"vbkeyf2", 113},
        {"vbkeyf3", 114}, {"vbkeyf4", 115}, {"vbkeyf5", 116}, {"vbkeyf6", 117},
        {"vbkeyf7", 118}, {"vbkeyf8", 119}, {"vbkeyf9", 120}, {"vbkeyf10", 121},
        {"vbkeyf11", 122}, {"vbkeyf12", 123},
        {"vbshiftmask", 1}, {"vbctrlmask", 2}, {"vbaltmask", 4},
        {"vbleftbutton", 1}, {"vbrightbutton", 2}, {"vbmiddlebutton", 4},
        {"vbmodeless", 0}, {"vbmodal", 1},
        // VbVarType (REQ-0080)
        {"vbempty", 0}, {"vbnull", 1}, {"vbinteger", 2}, {"vblong", 3},
        {"vbsingle", 4}, {"vbdouble", 5}, {"vbcurrency", 6}, {"vbdate", 7},
        {"vbstring", 8}, {"vbobject", 9}, {"vberror", 10}, {"vbboolean", 11},
        {"vbvariant", 12}, {"vbdataobject", 13}, {"vbdecimal", 14},
        {"vbbyte", 17}, {"vbuserdefinedtype", 36}, {"vbarray", 8192},
        // VbStrConv (REQ-0084)
        {"vbuppercase", 1}, {"vblowercase", 2}, {"vbpropercase", 3},
        {"vbwide", 4}, {"vbnarrow", 8}, {"vbkatakana", 16}, {"vbhiragana", 32},
        {"vbunicode", 64}, {"vbfromunicode", 128},
        // VbTriState (REQ-0092)
        {"vbusedefault", -2}, {"vbtrue", -1}, {"vbfalse", 0},
        // VbCallType (REQ-0093)
        {"vbmethod", 1}, {"vbget", 2}, {"vblet", 4}, {"vbset", 8},
        // VbFileAttribute (REQ-0083)
        {"vbnormal", 0}, {"vbreadonly", 1}, {"vbhidden", 2}, {"vbsystem", 4},
        {"vbvolume", 8}, {"vbdirectory", 16}, {"vbarchive", 32}, {"vbalias", 64},
        // VbMsgBoxResult (REQ-0082)
        {"vbok", 1}, {"vbcancel", 2}, {"vbabort", 3}, {"vbretry", 4},
        {"vbignore", 5}, {"vbyes", 6}, {"vbno", 7},
        // VbDayOfWeek (REQ-0085)
        {"vbusesystemdayofweek", 0}, {"vbsunday", 1}, {"vbmonday", 2},
        {"vbtuesday", 3}, {"vbwednesday", 4}, {"vbthursday", 5},
        {"vbfriday", 6}, {"vbsaturday", 7},
        // VbMsgBoxStyle (REQ-0081)
        {"vbokonly", 0}, {"vbokcancel", 1}, {"vbabortretryignore", 2},
        {"vbyesnocancel", 3}, {"vbyesno", 4}, {"vbretrycancel", 5},
        {"vbcritical", 16}, {"vbquestion", 32}, {"vbexclamation", 48},
        {"vbinformation", 64}, {"vbdefaultbutton1", 0}, {"vbdefaultbutton2", 256},
        {"vbdefaultbutton3", 512}, {"vbdefaultbutton4", 768},
        {"vbapplicationmodal", 0}, {"vbsystemmodal", 4096},
        {"vbmsgboxhelpbutton", 16384}, {"vbmsgboxright", 524288},
        {"vbmsgboxrtlreading", 1048576}, {"vbmsgboxsetforeground", 65536},
        // VbAppWinStyle (REQ-0088)
        {"vbhide", 0}, {"vbnormalfocus", 1}, {"vbminimizedfocus", 2},
        {"vbmaximizedfocus", 3}, {"vbnormalnofocus", 4}, {"vbminimizednofocus", 6},
        // VbFirstWeekOfYear (REQ-0086)
        {"vbusesystem", 0}, {"vbfirstjan1", 1}, {"vbfirstfourdays", 2},
        {"vbfirstfullweek", 3},
        // VbCalendar (REQ-0090)
        {"vbcalgreg", 0}, {"vbcalhijri", 1},
        // VbDateTimeFormat (REQ-0091)
        {"vbgeneraldate", 0}, {"vblongdate", 1}, {"vbshortdate", 2},
        {"vblongtime", 3}, {"vbshorttime", 4},
        // VbIMEStatus (REQ-0087)
        {"vbimenoop", 0}, {"vbimemodenocontrol", 0}, {"vbimeon", 1},
        {"vbimemodeon", 1}, {"vbimeoff", 2}, {"vbimemodeoff", 2},
        {"vbimedisable", 3}, {"vbimemodedisable", 3}, {"vbimehiragana", 4},
        {"vbimemodehiragana", 4}, {"vbimekatakanadbl", 5},
        {"vbimemodekatakana", 5}, {"vbimekatakanasng", 6},
        {"vbimemodekatakanahalf", 6}, {"vbimealphadbl", 7},
        {"vbimemodealphafull", 7}, {"vbimealphasng", 8}, {"vbimemodealpha", 8},
        {"vbimemodehangulfull", 9}, {"vbimemodehangul", 10},
        // General constants (REQ-0094), integer member
        {"vbobjecterror", -2147221504},
    };
    const auto entry = table.find(identifier);
    if (entry == table.end()) {
        // vbKeyA..vbKeyZ and vbKey0..vbKey9 are their ASCII codes.
        if (identifier.size() == 6U && identifier.substr(0, 5U) == "vbkey") {
            const char last = identifier[5];
            if (last >= 'a' && last <= 'z') return static_cast<Integer>(last - 'a' + 'A');
            if (last >= '0' && last <= '9') return static_cast<Integer>(last);
        }
        return std::nullopt;
    }
    return entry->second;
}

// Source-visible VBA string constants (REQ-0094). Each resolves to its exact
// stored bytes and is reserved so source cannot shadow it.
[[nodiscard]] std::optional<std::string> vba_string_constant(
    const std::string_view identifier) {
    static const std::unordered_map<std::string_view, std::string> table = {
        {"vbnullstring", std::string{}},
        {"vbnullchar", std::string(1, '\0')},
        {"vbcrlf", "\r\n"}, {"vbnewline", "\r\n"},
        {"vbcr", "\r"}, {"vblf", "\n"}, {"vbback", "\b"},
        {"vbformfeed", "\f"}, {"vbtab", "\t"}, {"vbverticaltab", "\v"},
    };
    const auto entry = table.find(identifier);
    if (entry == table.end()) {
        return std::nullopt;
    }
    return entry->second;
}

[[nodiscard]] bool is_reserved_identifier(const std::string_view identifier) noexcept {
    return identifier == "and" || identifier == "as" || identifier == "boolean" ||
           identifier == "dim" || identifier == "do" || identifier == "each" ||
           identifier == "eqv" ||
           identifier == "erase" ||
           identifier == "exit" || identifier == "false" || identifier == "for" ||
           identifier == "else" || identifier == "elseif" || identifier == "if" ||
           identifier == "imp" || identifier == "in" || identifier == "is" ||
           identifier == "let" ||
           identifier == "long" || identifier == "case" || identifier == "const" ||
           identifier == "loop" || identifier == "mod" ||
           identifier == "next" ||
           identifier == "not" ||
           identifier == "null" || identifier == "empty" ||
           identifier == "nothing" || identifier == "object" || identifier == "set" ||
           identifier == "sub" || identifier == "function" || identifier == "call" ||
           identifier == "new" || identifier == "property" || identifier == "get" ||
           identifier == "me" ||
           identifier == "byval" || identifier == "byref" ||
           identifier == "optional" || identifier == "paramarray" ||
           identifier == "static" || identifier == "private" ||
           identifier == "public" ||
           identifier == "option" || identifier == "or" ||
           identifier == "preserve" ||
           identifier == "print" || identifier == "randomize" || identifier == "redim" ||
           identifier == "rem" || identifier == "select" ||
           identifier == "string" || identifier == "then" ||
           identifier == "explicit" || identifier == "typeof" || identifier == "erl" || identifier == "type" || identifier == "err" || identifier == "goto" || identifier == "resume" || identifier == "enum" || identifier == "step" || identifier == "to" ||
           identifier == "true" ||
           identifier == "until" || identifier == "wend" ||
           identifier == "while" || identifier == "with" || identifier == "xor" ||
           vba_constant_value(identifier).has_value() ||
           vba_string_constant(identifier).has_value();
}

// Class members may reuse a few statement keywords (`Public Sub Print`).
[[nodiscard]] bool is_reserved_member_name(const std::string_view identifier) noexcept {
    return identifier != "print" && identifier != "randomize" && is_reserved_identifier(identifier);
}

[[nodiscard]] wfc::Evaluation failure(
    const std::string_view code,
    const std::string_view message,
    const std::size_t offset) {
    wfc::Evaluation result;
    result.diagnostic = std::string(code) + " at byte " +
                        std::to_string(offset + 1) + ": " + std::string(message);
    result.error_offset = offset;
    return result;
}

// One variable-declaration scope: either the single module-level scope, or
// one procedure call's local scope (its parameters and locally Dim'd
// variables). A procedure call's variable lookups see only its own scope
// and the module scope -- never an enclosing caller's locals -- matching
// VB6's own two-level (module/procedure) scoping, which has no nested
// block scope.
// REQ-0243: the built-in `Collection`, written in the evaluator's own VB
// dialect and registered on demand; its storage is the native `WfcStore`.

constexpr std::string_view kCollectionSource = R"VB(Private Sub Class_Initialize()
Dim r As Long
r = WfcStore(10, Me, 1)
End Sub
Public Sub Add(ByVal Item As Variant, Optional Key As String = "", Optional Before As Variant, Optional After As Variant)
Dim pos As Long
Dim k As Variant
If Not IsMissing(Before) And Not IsMissing(After) Then Err.Raise 5, , "Invalid procedure call or argument"
If Key <> "" Then
If WfcStore(2, Me, Key) > 0 Then Err.Raise 457, , "This key is already associated with an element of this collection"
k = Key
End If
pos = 0
If Not IsMissing(Before) Then pos = Locate(Before)
If Not IsMissing(After) Then pos = Locate(After) + 1
pos = WfcStore(3, Me, k, Item, pos)
End Sub
Public Property Get Count() As Long
Count = WfcStore(1, Me)
End Property
Public Function Item(Index As Variant) As Variant
Attribute Item.VB_UserMemId = 0
Dim p As Long
p = Locate(Index)
If WfcStore(11, Me, p) Then
Set Item = WfcStore(4, Me, p)
Else
Item = WfcStore(4, Me, p)
End If
End Function
Public Sub Remove(Index As Variant)
Dim p As Long
p = Locate(Index)
p = WfcStore(8, Me, p)
End Sub
Private Function Locate(Index As Variant) As Long
Dim p As Long
If VarType(Index) = 8 Then
p = WfcStore(2, Me, Index)
If p = 0 Then Err.Raise 5, , "Invalid procedure call or argument"
Locate = p
Exit Function
End If
If Index < 1 Or Index > WfcStore(1, Me) Then Err.Raise 9, , "Subscript out of range"
Locate = CLng(Index)
End Function
Public Function NewEnum() As Object
Set NewEnum = Me
End Function
Public Function WfcItems() As Variant
WfcItems = WfcStore(12, Me)
End Function
)VB";


// Scripting.Dictionary, provided as VB source like Collection (case-sensitive
// keys unless CompareMode = 1; reading a missing key adds an Empty entry, as
// the real object does).
constexpr std::string_view kDictionarySource = R"VB(Private mode As Long
Public Property Get CompareMode() As Long
CompareMode = mode
End Property
Public Property Let CompareMode(v As Long)
Dim r As Long
If WfcStore(1, Me) > 0 Then Err.Raise 5, , "Invalid procedure call or argument"
mode = v
r = WfcStore(10, Me, v)
End Property
Public Property Get Count() As Long
Count = WfcStore(1, Me)
End Property
Private Function IndexOf(Key As Variant) As Long
IndexOf = WfcStore(2, Me, Key)
End Function
Public Sub Add(Key As Variant, Item As Variant)
Dim r As Long
If IndexOf(Key) > 0 Then Err.Raise 457, "Scripting.Dictionary", "This key is already associated with an element of this collection"
r = WfcStore(3, Me, Key, Item, 0)
End Sub
Public Function Exists(Key As Variant) As Boolean
Exists = IndexOf(Key) > 0
End Function
Public Property Get Item(Key As Variant) As Variant
Attribute Item.VB_UserMemId = 0
Dim i As Long
Dim r As Long
i = IndexOf(Key)
If i = 0 Then
r = WfcStore(3, Me, Key, Empty, 0)
i = WfcStore(1, Me)
End If
If WfcStore(11, Me, i) Then
Set Item = WfcStore(4, Me, i)
Else
Item = WfcStore(4, Me, i)
End If
End Property
Public Property Let Item(Key As Variant, NewItem As Variant)
Dim i As Long
Dim r As Long
i = IndexOf(Key)
If i = 0 Then
r = WfcStore(3, Me, Key, NewItem, 0)
Else
r = WfcStore(6, Me, i, NewItem)
End If
End Property
Public Property Set Item(Key As Variant, NewItem As Object)
Dim i As Long
Dim r As Long
i = IndexOf(Key)
If i = 0 Then
r = WfcStore(3, Me, Key, NewItem, 0)
Else
r = WfcStore(6, Me, i, NewItem)
End If
End Property
Public Property Let Key(OldKey As Variant, NewKey As Variant)
Dim i As Long
Dim r As Long
i = IndexOf(OldKey)
If i = 0 Then Err.Raise 32811, "Scripting.Dictionary", "Element not found"
If IndexOf(NewKey) > 0 Then Err.Raise 457, "Scripting.Dictionary", "This key is already associated with an element of this collection"
r = WfcStore(7, Me, i, NewKey)
End Property
Public Sub Remove(Key As Variant)
Dim i As Long
i = IndexOf(Key)
If i = 0 Then Err.Raise 32811, "Scripting.Dictionary", "Element not found"
i = WfcStore(8, Me, i)
End Sub
Public Sub RemoveAll()
Dim r As Long
r = WfcStore(9, Me)
End Sub
Public Function Keys() As Variant
Keys = WfcStore(13, Me)
End Function
Public Function Items() As Variant
Items = WfcStore(12, Me)
End Function
Public Function WfcItems() As Variant
WfcItems = WfcStore(13, Me)
End Function
)VB";

// The global `App` object (the properties a console-style program reads).
constexpr std::string_view kAppSource = R"VB(Public Property Get Path() As String
Path = CurDir$
End Property
Public Property Get Title() As String
Title = ""
End Property
Public Property Get EXEName() As String
EXEName = "Project1"
End Property
Public Property Get PrevInstance() As Boolean
PrevInstance = False
End Property
Public Property Get Major() As Long
Major = 1
End Property
Public Property Get Minor() As Long
Minor = 0
End Property
Public Property Get Revision() As Long
Revision = 0
End Property
Public Property Get hInstance() As Long
hInstance = 0
End Property
Public Property Get ThreadID() As Long
ThreadID = 0
End Property
Public Property Get CompanyName() As String
CompanyName = ""
End Property
Public Property Get ProductName() As String
ProductName = ""
End Property
Public Property Get FileDescription() As String
FileDescription = ""
End Property
Public Property Get Comments() As String
Comments = ""
End Property
Public Property Get LegalCopyright() As String
LegalCopyright = ""
End Property
Public Sub LogEvent(LogBuffer As String, Optional EventType As Long = 1)
End Sub
)VB";

// Scripting.FileSystemObject and its TextStream / File objects, written in VB
// on top of the native file statements.
constexpr std::string_view kTextStreamSource = R"VB(Private fnum As Integer
Private mLine As Long
Private isOpen As Boolean
Public Sub Init(ByVal path As String, ByVal m As Long)
fnum = FreeFile
Select Case m
Case 1
Open path For Input As #fnum
Case 2
Open path For Output As #fnum
Case Else
Open path For Append As #fnum
End Select
isOpen = True
End Sub
Public Sub Write(ByVal s As String)
Print #fnum, s;
End Sub
Public Sub WriteLine(Optional ByVal s As String = "")
Print #fnum, s
End Sub
Public Sub WriteBlankLines(ByVal n As Long)
Dim i As Long
For i = 1 To n
Print #fnum, ""
Next i
End Sub
Public Function ReadLine() As String
Line Input #fnum, ReadLine
mLine = mLine + 1
End Function
Public Function ReadAll() As String
Dim t As String, first As Boolean
first = True
Do While Not EOF(fnum)
Line Input #fnum, t
If Not first Then ReadAll = ReadAll & vbCrLf
ReadAll = ReadAll & t
first = False
mLine = mLine + 1
Loop
End Function
Public Function Read(ByVal n As Long) As String
Read = Input$(n, #fnum)
End Function
Public Sub SkipLine()
Dim t As String
Line Input #fnum, t
mLine = mLine + 1
End Sub
Public Property Get AtEndOfStream() As Boolean
AtEndOfStream = EOF(fnum)
End Property
Public Property Get Line() As Long
Line = mLine + 1
End Property
Public Sub Close()
If isOpen Then Close #fnum
isOpen = False
End Sub
Private Sub Class_Terminate()
If isOpen Then Close #fnum
End Sub
)VB";

constexpr std::string_view kFileObjectSource = R"VB(Private mPath As String
Public Sub Init(ByVal path As String)
mPath = path
End Sub
Public Property Get Path() As String
Path = mPath
End Property
Public Property Get Name() As String
Dim i As Long
i = InStrRev(mPath, "\")
If InStrRev(mPath, "/") > i Then i = InStrRev(mPath, "/")
Name = Mid$(mPath, i + 1)
End Property
Public Property Get Size() As Long
Size = FileLen(mPath)
End Property
Public Property Get DateLastModified() As Date
DateLastModified = FileDateTime(mPath)
End Property
Public Sub Delete()
Kill mPath
End Sub
)VB";

constexpr std::string_view kFileSystemObjectSource = R"VB(Public Function FileExists(ByVal path As String) As Boolean
On Error Resume Next
Err.Clear
FileExists = ((GetAttr(path) And 16) = 0)
If Err.Number <> 0 Then FileExists = False
End Function
Public Function FolderExists(ByVal path As String) As Boolean
On Error Resume Next
Err.Clear
FolderExists = ((GetAttr(path) And 16) <> 0)
If Err.Number <> 0 Then FolderExists = False
End Function
Private Function LastSep(ByVal path As String) As Long
LastSep = InStrRev(path, "\")
If InStrRev(path, "/") > LastSep Then LastSep = InStrRev(path, "/")
End Function
Public Function GetFileName(ByVal path As String) As String
GetFileName = Mid$(path, LastSep(path) + 1)
End Function
Public Function GetExtensionName(ByVal path As String) As String
Dim n As String, i As Long
n = GetFileName(path)
i = InStrRev(n, ".")
If i > 0 Then GetExtensionName = Mid$(n, i + 1)
End Function
Public Function GetBaseName(ByVal path As String) As String
Dim n As String, i As Long
n = GetFileName(path)
i = InStrRev(n, ".")
If i > 0 Then GetBaseName = Left$(n, i - 1) Else GetBaseName = n
End Function
Public Function GetParentFolderName(ByVal path As String) As String
Dim i As Long
i = LastSep(path)
If i > 1 Then GetParentFolderName = Left$(path, i - 1) Else GetParentFolderName = ""
End Function
Public Function BuildPath(ByVal a As String, ByVal b As String) As String
If a = "" Then
BuildPath = b
ElseIf Right$(a, 1) = "\" Or Right$(a, 1) = "/" Then
BuildPath = a & b
Else
BuildPath = a & "\" & b
End If
End Function
Public Function GetAbsolutePathName(ByVal path As String) As String
If Mid$(path, 2, 1) = ":" Or Left$(path, 1) = "\" Or Left$(path, 1) = "/" Then
GetAbsolutePathName = path
Else
GetAbsolutePathName = BuildPath(CurDir$, path)
End If
End Function
Public Function GetTempName() As String
GetTempName = "rad" & Hex$(Int(Rnd * 65535)) & ".tmp"
End Function
Public Function CreateTextFile(ByVal path As String, Optional ByVal overwrite As Boolean = True) As Object
Dim t As New WfcTextStream
If Not overwrite And FileExists(path) Then Err.Raise 58, "FileSystemObject", "File already exists"
t.Init path, 2
Set CreateTextFile = t
End Function
Public Function OpenTextFile(ByVal path As String, Optional ByVal mode As Long = 1, Optional ByVal create As Boolean = False) As Object
Dim t As New WfcTextStream
If mode = 1 And Not FileExists(path) Then Err.Raise 53, "FileSystemObject", "File not found"
t.Init path, mode
Set OpenTextFile = t
End Function
Public Function GetFile(ByVal path As String) As Object
Dim f As New WfcFile
If Not FileExists(path) Then Err.Raise 53, "FileSystemObject", "File not found"
f.Init path
Set GetFile = f
End Function
Public Sub DeleteFile(ByVal path As String)
Kill path
End Sub
Public Sub CopyFile(ByVal src As String, ByVal dst As String, Optional ByVal overwrite As Boolean = True)
If Not overwrite And FileExists(dst) Then Err.Raise 58, "FileSystemObject", "File already exists"
FileCopy src, dst
End Sub
Public Sub MoveFile(ByVal src As String, ByVal dst As String)
Name src As dst
End Sub
Public Sub CreateFolder(ByVal path As String)
MkDir path
End Sub
Public Sub DeleteFolder(ByVal path As String)
RmDir path
End Sub
)VB";

// VBScript.RegExp, written in VB over two native helpers (WfcRegexMatches /
// WfcRegexReplace, std::regex ECMAScript flavor).
constexpr std::string_view kRegExpSource = R"VB(Public Pattern As String
Public Global As Boolean
Public IgnoreCase As Boolean
Public MultiLine As Boolean
Public Function Test(ByVal text As String) As Boolean
Dim m As Variant
m = WfcRegexMatches(Pattern, text, IgnoreCase, MultiLine, False)
Test = UBound(m) >= 0
End Function
Public Function Execute(ByVal text As String) As Object
Dim raw As Variant, i As Long, mc As New WfcMatchCollection
raw = WfcRegexMatches(Pattern, text, IgnoreCase, MultiLine, Global)
For i = 0 To UBound(raw)
mc.AddRaw raw(i)
Next i
Set Execute = mc
End Function
Public Function Replace(ByVal text As String, ByVal replacement As String) As String
Replace = WfcRegexReplace(Pattern, text, replacement, IgnoreCase, MultiLine, Global)
End Function
)VB";

constexpr std::string_view kMatchCollectionSource = R"VB(Private items() As Variant
Private n As Long
Public Sub AddRaw(raw As Variant)
Dim m As New WfcMatch
m.Init raw
n = n + 1
If n = 1 Then
ReDim items(0 To 3)
ElseIf n > UBound(items) + 1 Then
ReDim Preserve items(0 To UBound(items) * 2 + 1)
End If
Set items(n - 1) = m
End Sub
Public Property Get Count() As Long
Count = n
End Property
Public Function Item(ByVal Index As Long) As Object
Attribute Item.VB_UserMemId = 0
If Index < 0 Or Index >= n Then Err.Raise 9, , "Subscript out of range"
Set Item = items(Index)
End Function
Public Function WfcItems() As Variant
Dim r() As Variant, i As Long
If n = 0 Then
WfcItems = Array()
Else
ReDim r(1 To n)
For i = 1 To n
Set r(i) = items(i - 1)
Next i
WfcItems = r
End If
End Function
)VB";

constexpr std::string_view kMatchSource = R"VB(Private mIndex As Long
Private mLength As Long
Private mValue As String
Private subs As Variant
Public Sub Init(raw As Variant)
Dim sm As New WfcSubMatches, i As Long
mIndex = raw(0)
mLength = raw(1)
mValue = raw(2)
For i = 3 To UBound(raw)
sm.AddText raw(i)
Next i
Set subs = sm
End Sub
Public Property Get Value() As String
Attribute Value.VB_UserMemId = 0
Value = mValue
End Property
Public Property Get FirstIndex() As Long
FirstIndex = mIndex
End Property
Public Property Get Length() As Long
Length = mLength
End Property
Public Property Get SubMatches() As Object
Set SubMatches = subs
End Property
)VB";

constexpr std::string_view kSubMatchesSource = R"VB(Private items() As Variant
Private n As Long
Public Sub AddText(ByVal s As String)
n = n + 1
If n = 1 Then
ReDim items(0 To 3)
ElseIf n > UBound(items) + 1 Then
ReDim Preserve items(0 To UBound(items) * 2 + 1)
End If
items(n - 1) = s
End Sub
Public Property Get Count() As Long
Count = n
End Property
Public Function Item(ByVal Index As Long) As String
Attribute Item.VB_UserMemId = 0
If Index < 0 Or Index >= n Then Err.Raise 9, , "Subscript out of range"
Item = items(Index)
End Function
Public Function WfcItems() As Variant
Dim r() As Variant, i As Long
If n = 0 Then
WfcItems = Array()
Else
ReDim r(1 To n)
For i = 1 To n
r(i) = items(i - 1)
Next i
WfcItems = r
End If
End Function
)VB";

struct Scope {
    std::unordered_map<std::string, Value> variables;
    std::unordered_set<std::string> constants;
    std::unordered_set<std::string> variant_variables;
    std::unordered_set<std::string> object_variables;
    // `Dim x As New Cls` variables: created on first use (and again after
    // `Set x = Nothing`), as VB6 does.
    std::unordered_set<std::string> auto_new_variables;
    // Only populated for a variable declared `As ClassName` (as opposed to
    // the generic `As Object`, which accepts an instance of any class):
    // maps the variable's name to the lowercased class name Set must match.
    // A generic Object-typed variable (in object_variables but absent here)
    // accepts Nothing or any class's instance.
    std::unordered_map<std::string, std::string> object_class_names;
    // Only meaningful for a procedure-call scope (never the module scope):
    // distinguishes a Function call's frame (Exit Function is valid, and
    // the procedure's own name holds its return value) from a Sub's.
    bool is_function_frame{};
    // REQ-0206: names declared by a `Static` statement within the
    // procedure call this scope belongs to. invoke_definition copies each
    // one's final value back into the owning ProcedureDef's persistent
    // `statics` scope just before this frame is discarded.
    std::unordered_set<std::string> static_variable_names;
    // REQ-0224: names of this call's own Optional Variant parameters,
    // with no explicit default, that the caller did not supply an
    // argument for (bound to Empty instead). Real VB6's `IsMissing` is
    // documented specifically for this one case: a required parameter, a
    // non-Variant Optional parameter, and a Variant Optional parameter
    // that *does* have an explicit default (the default counts as
    // supplied) all bind a real value indistinguishable from a
    // caller-supplied one, so `IsMissing` on those stays the constant
    // `False` this evaluator already answered before this requirement.
    std::unordered_set<std::string> missing_parameter_names;
    // REQ-0238: this frame's `On Error` state. 0 = none, 1 = Resume Next,
    // 2 = GoTo label (on_error_label is the label's source offset).
    int on_error_mode{};
    std::size_t on_error_label{};
    bool in_error_handler{};
    // While an `On Error GoTo` handler runs in the context of the failing
    // statement: set by `Resume` / `Resume Next` (1 = next, 2 = retry).
    int handler_depth{};
    int resume_signal{};
    std::size_t error_resume_next{};
    std::size_t error_retry{};
    // REQ-0248: return offsets of active `GoSub` calls in this frame.
    std::vector<std::size_t> gosub_stack;
    // REQ-0248: `Dim s As String * n` -- name -> fixed length.
    std::unordered_map<std::string, std::size_t> fixed_string_lengths;
};

// The result of looking a variable name up across the (at most two) scopes
// a point of execution can see: the value slot itself, and which Scope
// owns it (so a caller can check that same scope's constants/
// variant_variables/object_variables membership). `value` is null when the
// name isn't declared in either visible scope.
struct VariableLookup {
    Value* value{};
    Scope* scope{};
};

// A parameter of a user-defined Sub/Function: `[ByVal|ByRef] name [As
// Type]`. `type_index` mirrors a `Dim`-declared variable's fixed-type slot
// (see `Interpreter::element_default_for_type`-style dispatch); `is_variant`
// marks a `ByVal`/`ByRef` Variant parameter, which -- like any
// Variant-declared variable -- accepts and retypes to any value.
struct ProcedureParameter {
    std::string name;
    std::size_t type_index{};
    bool is_variant{};
    bool by_val{};
    // `As Object` or `As SomeClassName` (REQ-0228, generalizing what was
    // previously only accepted for a `Property Set` member's own single
    // parameter): accepts an object reference (`Nothing` or an
    // `ObjectInstance`) rather than a fixed scalar type. `class_name` is
    // empty for the generic `As Object` form (any class, or `Nothing`,
    // accepted) or the required class name for `As SomeClassName` (a
    // `Set`-source class mismatch reports `WFC0137`, the same diagnostic
    // a class-typed field/return already reports for the same case).
    bool is_object_reference{};
    std::string class_name;
    // REQ-0206: `Optional [name [As Type] [= default]]`. An omitted
    // trailing argument at the call site binds `default_value` (if
    // `has_default`) or the type's own zero value otherwise. Every
    // parameter after the first Optional one must itself be Optional (or
    // the trailing ParamArray).
    bool is_optional{};
    bool has_default{};
    Value default_value;
    // REQ-0206: `ParamArray name()` -- must be the last parameter,
    // collects every call argument from this position onward into a fresh
    // zero-based array bound to `name`. Mutually exclusive with
    // `is_optional` (a ParamArray has no notion of a single default value).
    bool is_param_array{};
    // REQ-0211: a plain (non-ParamArray) array parameter, `name() As
    // Type`. `type_index` holds the required element type, not the
    // parameter's own type (there is no single `Value` alternative for
    // "array of Long"). Always effectively ByRef: `scan_procedure_
    // parameters` rejects an explicit `ByVal` on an array parameter, since
    // an array is a reference-like value in real VB6 and this evaluator
    // has no other way to alias the caller's array.
    bool is_array_parameter{};
    // REQ-0215: `name() As Variant`/`As Object` -- a Variant-/Object-
    // element array parameter (REQ-0212's element kinds, extended to
    // array parameters). Mutually exclusive with each other and with a
    // fixed `type_index`-checked element type; `type_index` is unused
    // (left at its default) when either is set, since a Variant/Object
    // element array carries its element kind on the `ArrayValue` itself,
    // not via a single fixed type index.
    bool is_variant_array_parameter{};
    bool is_object_array_parameter{};
};

// A `Sub`/`Function` declaration found by the module-level pre-scan
// (`Interpreter::scan_procedures`). `body_start`/`body_end` bound the
// statements between the header line and the matching `End Sub`/`End
// Function`; `declaration_end` is where the main top-to-bottom execution
// pass resumes after skipping the whole declaration (parameters aren't
// executed where they're written -- only when called).
struct ProcedureDef {
    std::vector<ProcedureParameter> parameters;
    bool is_function{};
    // `Static Sub|Function|Property`: every local variable keeps its value between calls.
    bool static_locals{};
    std::size_t return_type_index{};
    bool return_is_variant{};
    // A Function/Property Get declared `As Object` or `As SomeClass`
    // (REQ-0205): the call's own name slot holds Nothing or an
    // ObjectInstance, exactly like an Object-typed variable (assignable
    // only via Set). `return_class_name` is empty for the generic
    // `As Object` (any class's instance accepted), or the (lowercased)
    // required class name for `As SomeClass`.
    bool return_is_object{};
    std::string return_class_name;
    // REQ-0216: a `Function` declared `As Type()` returns an array of
    // `Type` (`return_type_index` holds the element type, the same
    // convention `REQ-0211`'s array parameters already use). The return
    // slot starts as an unallocated dynamic array (`Dim`-style), so the
    // body can either assign a whole array to its own name or `ReDim` it
    // directly, both through existing, unmodified array machinery.
    // `Property Get`, `Variant`/`Object`-element, and multi-dimensional
    // array return types remain unsupported; see REQ-0216's Scope.
    bool return_is_array{};
    // REQ-0266: a `Declare` d external routine; calling it raises error 453.
    bool is_external{};
    std::size_t body_start{};
    std::size_t body_end{};
    std::size_t declaration_end{};
    // REQ-0206: true for a class member (or, parsed but unenforced, a
    // module-level procedure -- see scan_procedures) declared `Private`, or
    // a class member declared with a bare `Dim`/no modifier at all (VB6's
    // own default); false for `Public` or a module-level procedure with no
    // modifier (nothing to restrict access from, since this evaluator has
    // only one standard module). A class member's dot-access sites check
    // this against current_class_def() (see member_accessible).
    bool is_private{};
    // REQ-0206: `Static name [As Type]` declarations inside this
    // procedure's own body persist their values across separate calls.
    // Storage lives here, on the ProcedureDef itself (found once by
    // scan_procedures/scan_class_body and never moved or erased
    // afterward), rather than in any per-call Scope; `mutable` because
    // every other member function receives its ProcedureDef by const
    // reference. See parse_static_declaration/current_procedure_def_.
    mutable Scope statics;
};

// One `Dim`/`Public`/`Private` field declared at a class module's top level
// (see ClassDef below). `Public` and `Private` fields differ only in
// `is_private` (REQ-0206); a bare `Dim` is implicitly `Private`, matching
// real VB6's own module-level default.
struct ClassFieldDef {
    std::size_t type_index{};
    bool is_variant{};
    bool is_private{};
    // A field declared `As Object` or `As SomeClass` (REQ-0205): the field
    // holds Nothing or an ObjectInstance. `class_name` is empty for the
    // generic `As Object` (any class's instance accepted), or the
    // (lowercased) required class name for `As SomeClass`.
    bool is_object{};
    std::string class_name;
    // `Private WithEvents x As Source`: handlers named `x_Event` run when
    // the referenced object raises that event.
    bool with_events{};
    // `String * n` field: its fixed length (0 for an ordinary String).
    std::size_t fixed_length{};
    // `Private x As New Cls`: created together with the owning instance.
    bool auto_new{};
    // REQ-0251: an array field (`Private items() As Variant`,
    // `Public grid(1 To 3) As Long`). `dimensions` is empty for a dynamic
    // (bound-less) array, which starts unallocated until `ReDim`.
    bool is_array{};
    std::vector<std::pair<Integer, Integer>> dimensions;
};

// A class module's declarations, found by `Interpreter::scan_class_body`
// (mirroring `scan_procedures`' role for the standard module): its own
// source text, field declarations, `Sub`/`Function` methods, and `Property
// Get`/`Let`/`Set` accessors. `source` is a real VB6 class's separate .cls
// file, supplied via `wfc::ClassModuleSource`; a class's members only ever
// see their own instance's fields and their own locals/parameters, never the
// standard module's variables, matching VB6's own module-to-module
// isolation (see REQ-0203's Scope).
struct ClassDef {
    std::string_view source;
    // The original, as-supplied spelling (for TypeName rendering); lookups
    // themselves are keyed by the lowercased name, like every other
    // identifier in this evaluator.
    std::string display_name;
    std::unordered_map<std::string, ClassFieldDef> fields;
    // Field names in declaration order (UDT serialization, Len).
    std::vector<std::string> field_order;
    std::unordered_map<std::string, ProcedureDef> methods;
    std::unordered_map<std::string, ProcedureDef> property_get;
    std::unordered_map<std::string, ProcedureDef> property_let;
    std::unordered_map<std::string, ProcedureDef> property_set;
    // REQ-0233: lowercased names of every class this one `Implements`.
    // An interface is just an ordinary class in real VB6 (there is no
    // separate `Interface` keyword) -- this class must define its own
    // `InterfaceName_MemberName`-named method/property for each member a
    // caller reaches through an interface-typed reference (see
    // `interface_prefixed_member_name`); this evaluator does not verify
    // completeness (that every interface member actually has a matching
    // `InterfaceName_MemberName` counterpart here) at scan time.
    std::vector<std::string> implements;
    // REQ-0241: a `Type ... End Type` user-defined type, modeled as a
    // class whose instances have value semantics.
    bool is_udt{};
    // REQ-0257: lowercased name of the member marked
    // `Attribute Name.VB_UserMemId = 0` (the class's default member).
    std::string default_member;
    // REQ-0260: `Const` and `Enum` members declared in the class module.
    std::unordered_map<std::string, Value> constants;
    // `Public Event Name(params)` declarations; only `parameters` is used.
    std::unordered_map<std::string, ProcedureDef> events;
};

// The storage backing one live `New ClassName` instance. Reuses Scope for
// field storage (a class instance's fields need exactly the same
// name/value/constants-are-irrelevant/variant-retyping shape a procedure's
// local scope already provides). Held behind a shared_ptr so every
// ObjectInstance Value copy referring to the same instance shares one
// identity, matching VB6 reference-type semantics (`Is`, and mutating a
// field through one reference is visible through another).
//
// Inherits enable_shared_from_this so the `Me` keyword can hand out a new
// ObjectInstance sharing this same instance's ownership (ObjectInstance
// identity, and `Is`, depend on every reference sharing one control block --
// wrapping the raw InstanceData* instance_scopes_ tracks in a *new*
// shared_ptr would create a second, independent control block, leading to a
// double-free once both reached zero).
// Native key/value storage behind the built-in Collection and Dictionary classes (reached
// through the hidden `WfcStore` function): ordered entries plus a hash index of the keys.
struct NativeStore {
    std::vector<Value> keys;
    std::vector<Value> values;
    std::unordered_map<std::string, std::size_t> index;  // canonical key -> 0-based position
    bool dirty{true};
    bool text_compare{false};
};

struct InstanceData : std::enable_shared_from_this<InstanceData> {
    std::string class_name;
    std::unique_ptr<NativeStore> store;
    Scope fields;
    // Objects handling this instance's events: the sink instance (weak, so
    // a handler does not keep itself alive through its source) and the
    // WithEvents field name that holds this instance.
    std::vector<std::pair<std::weak_ptr<InstanceData>, std::string>> event_sinks;
    // `Static` locals of this instance's methods (VB6 keeps them per object).
    std::map<const void*, Scope> static_scopes;
};

// One evaluated call argument. `byref_target` is non-null only when the
// argument's source text was a single bare variable name (nothing else),
// pointing at that variable's slot in whichever scope it was found; a
// ByRef parameter's final value is copied back through this pointer after
// the call returns, matching VB6's default ByRef parameter-passing. Any
// other argument form (a literal, an expression, an array-element access)
// behaves as ByVal even for a ByRef parameter, since there is no
// caller-visible variable for the mutation to reach -- the same outcome a
// real VB6 compiler produces via a discarded temporary.
struct CallArgument {
    Value value;
    Value* byref_target{};
    // REQ-0269: `F(, 7)` -- an omitted argument slot.
    bool omitted{};
    // `name:=value` (lowercased); empty for a positional argument.
    std::string name{};
};

class Interpreter final {
public:
    ~Interpreter() {
        for (auto& [number, file] : files_) {
            if (file.handle != nullptr) {
                std::fclose(file.handle);
            }
        }
    }
    Interpreter(const Interpreter&) = delete;
    Interpreter& operator=(const Interpreter&) = delete;

    explicit Interpreter(const std::string_view source, const bool allow_identifiers = true)
        : source_(source), allow_identifiers_(allow_identifiers), main_source_data_(source.data()) {}

    Interpreter(
        const std::string_view source,
        std::vector<wfc::ClassModuleSource> classes,
        const bool allow_identifiers = true)
        : source_(source), allow_identifiers_(allow_identifiers),
          main_source_data_(source.data()), class_sources_(std::move(classes)) {}

    [[nodiscard]] Scope& current_scope() noexcept { return scopes_.back(); }
    [[nodiscard]] Scope& module_scope() noexcept { return scopes_.front(); }
    [[nodiscard]] bool in_procedure() const noexcept { return scopes_.size() > 1U; }

    // Looks `name` up in the current procedure's local scope (if any),
    // falling back to the module scope -- or, while executing inside a
    // class member (`instance_scopes_` non-empty), that instance's own
    // field scope instead. A class member never falls through to the real
    // module scope: each class is its own isolated module, matching VB6's
    // module-to-module isolation (a class does not implicitly see a
    // standard module's variables, and vice versa). Neither level ever sees
    // an enclosing caller's locals, matching VB6's module/procedure
    // two-level scoping.
    [[nodiscard]] VariableLookup find_variable(const std::string& name) {
        auto found = find_variable_raw(name);
        if (found.value != nullptr && execute_ && !found.scope->auto_new_variables.empty() &&
            found.scope->auto_new_variables.contains(name) &&
            std::holds_alternative<Nothing>(*found.value)) {
            const auto declared = found.scope->object_class_names.find(name);
            if (declared != found.scope->object_class_names.end()) {
                const auto saved_offset = offset_;
                auto instance = instantiate_class(declared->second, offset_);
                offset_ = saved_offset;
                if (instance.has_value()) {
                    *found.value = std::move(*instance);
                }
            }
        }
        return found;
    }

    [[nodiscard]] VariableLookup find_variable_raw(const std::string& name) {
        if (name.starts_with("with.")) {
            const auto slot = with_slots_.find(name);
            if (slot != with_slots_.end()) {
                return {&slot->second, &with_scope_};
            }
            return {};
        }
        if (in_procedure()) {
            auto& local = current_scope();
            const auto entry = local.variables.find(name);
            if (entry != local.variables.end()) {
                return {&entry->second, &local};
            }
        }
        if (!instance_scopes_.empty()) {
            auto& fields = instance_scopes_.back()->fields;
            const auto entry = fields.variables.find(name);
            if (entry != fields.variables.end()) {
                return {&entry->second, &fields};
            }
            return {};
        }
        auto& module = module_scope();
        const auto entry = module.variables.find(name);
        if (entry != module.variables.end()) {
            return {&entry->second, &module};
        }
        return {};
    }

    // REQ-0237: collects `[Public|Private] Enum Name` type names up front so
    // `As Name` resolves even in procedures scanned before the Enum runs.
    [[nodiscard]] static bool in_with_identifier(const std::string& name) {
        return name.starts_with("with.");
    }

    // REQ-0265: `Option Explicit` anywhere in the program (main source or a
    // class module) makes undeclared names an error everywhere.
    void scan_option_explicit() {
        const auto mentions = [](const std::string_view text) {
            std::size_t position = 0;
            while (position < text.size()) {
                auto end = text.find('\n', position);
                if (end == std::string_view::npos) end = text.size();
                std::string line;
                for (const char c : text.substr(position, end - position)) {
                    if (c != ' ' && c != '\t' && c != '\r') line.push_back(ascii_lower(c));
                }
                position = end + 1;
                if (line == "optionexplicit" || line.rfind("optionexplicit'", 0) == 0) {
                    return true;
                }
            }
            return false;
        };
        strict_declarations_ = mentions(source_);
        for (const auto& module : class_sources_) {
            strict_declarations_ = strict_declarations_ || mentions(module.source);
        }
    }

    // `DefInt A-C, X` ... `DefVar`: the default type for undeclared names by
    // first letter. Scanned up front from every module (one program-wide
    // table; a per-module table is not modeled).
    static constexpr std::size_t no_default_type = static_cast<std::size_t>(-1);
    std::array<std::size_t, 26> default_types_ = [] {
        std::array<std::size_t, 26> table{};
        table.fill(static_cast<std::size_t>(-1));
        return table;
    }();

    [[nodiscard]] std::optional<std::size_t> default_type_for(const std::string& name) const {
        if (name.empty()) return std::nullopt;
        const char c = ascii_lower(name.front());
        if (c < 'a' || c > 'z') return std::nullopt;
        const auto index = default_types_[static_cast<std::size_t>(c - 'a')];
        if (index == no_default_type) return std::nullopt;
        return index;
    }

    // A Function/Property Get declared without `As Type`: Variant, or the
    // DefXxx type for its first letter.
    void apply_implicit_return_type(
        ProcedureDef& definition, const std::string& name, const char suffix = '\0') const {
        if (suffix != '\0') {
            definition.return_type_index = type_character_index(suffix);
            definition.return_is_variant = false;
        } else if (const auto default_type = default_type_for(name)) {
            definition.return_type_index = *default_type;
            definition.return_is_variant = false;
        } else {
            definition.return_type_index = Value{Empty{}}.index();
            definition.return_is_variant = true;
        }
    }

    // Parses one `DefXxx letters` line starting at `text[position]`; returns
    // false if the line is not a Def statement.
    bool apply_deftype_line(const std::string_view line) {
        std::size_t i = 0;
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
        if (line.size() < i + 6U || ascii_lower(line[i]) != 'd' || ascii_lower(line[i + 1U]) != 'e' ||
            ascii_lower(line[i + 2U]) != 'f') {
            return false;
        }
        std::size_t j = i + 3U;
        std::string word;
        while (j < line.size() && std::isalpha(static_cast<unsigned char>(line[j])) != 0) {
            word.push_back(ascii_lower(line[j++]));
        }
        std::size_t type_index = no_default_type;
        if (word == "int") type_index = Value{Int16{}}.index();
        else if (word == "lng") type_index = Value{Integer{}}.index();
        else if (word == "sng") type_index = Value{0.0f}.index();
        else if (word == "dbl") type_index = Value{0.0}.index();
        else if (word == "cur") type_index = Value{Currency{}}.index();
        else if (word == "str") type_index = Value{std::string{}}.index();
        else if (word == "bool") type_index = Value{false}.index();
        else if (word == "byte") type_index = Value{Byte{}}.index();
        else if (word == "dec") type_index = Value{Decimal{}}.index();
        else if (word == "date") type_index = Value{DateValue{}}.index();
        else if (word != "var" && word != "obj") return false;
        if (j >= line.size() || (line[j] != ' ' && line[j] != '\t')) return false;
        while (j < line.size()) {
            while (j < line.size() && (line[j] == ' ' || line[j] == '\t' || line[j] == ',')) ++j;
            if (j >= line.size() || std::isalpha(static_cast<unsigned char>(line[j])) == 0) break;
            const char first = ascii_lower(line[j++]);
            char last = first;
            std::size_t k = j;
            while (k < line.size() && line[k] == ' ') ++k;
            if (k < line.size() && line[k] == '-') {
                ++k;
                while (k < line.size() && line[k] == ' ') ++k;
                if (k < line.size() && std::isalpha(static_cast<unsigned char>(line[k])) != 0) {
                    last = ascii_lower(line[k]);
                    j = k + 1U;
                }
            }
            for (char c = first; c <= last; ++c) {
                default_types_[static_cast<std::size_t>(c - 'a')] = type_index;
            }
        }
        return true;
    }

    void scan_deftypes() {
        const auto scan = [this](const std::string_view text) {
            std::size_t position = 0;
            while (position < text.size()) {
                auto end = text.find('\n', position);
                if (end == std::string_view::npos) end = text.size();
                static_cast<void>(apply_deftype_line(text.substr(position, end - position)));
                position = end + 1;
            }
        };
        scan(source_);
        for (const auto& module : class_sources_) scan(module.source);
    }

    void scan_module_names() {
        const std::string_view text = source_;
        std::size_t position = 0;
        while (position < text.size()) {
            auto end = text.find('\n', position);
            if (end == std::string_view::npos) end = text.size();
            std::string line;
            for (const char c : text.substr(position, end - position)) {
                if (c != '\r') line.push_back(c);
            }
            position = end + 1;
            std::string lowered;
            for (const char c : line) lowered.push_back(ascii_lower(c));
            const auto marker = lowered.find("attribute vb_name");
            if (marker != 0) continue;
            const auto first = line.find('"');
            const auto last = line.rfind('"');
            if (first != std::string::npos && last > first) {
                std::string name;
                for (const char c : line.substr(first + 1, last - first - 1)) {
                    name.push_back(ascii_lower(c));
                }
                module_names_.insert(std::move(name));
            }
        }
    }

    void scan_enum_names() {
        scan_enum_names_in(source_);
        for (const auto& module : class_sources_) {
            scan_enum_names_in(module.source);
        }
    }

    void scan_enum_names_in(const std::string_view text) {
        std::size_t position = 0;
        while (position < text.size()) {
            auto end = text.find('\n', position);
            if (end == std::string_view::npos) {
                end = text.size();
            }
            std::string line;
            for (const char c : text.substr(position, end - position)) {
                line.push_back(ascii_lower(c));
            }
            position = end + 1;
            std::size_t i = 0;
            const auto skip_space = [&] {
                while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
            };
            const auto word = [&](const std::string_view w) {
                if (line.compare(i, w.size(), w) == 0 &&
                    (i + w.size() == line.size() || !is_identifier_part(line[i + w.size()]))) {
                    i += w.size();
                    return true;
                }
                return false;
            };
            skip_space();
            if (word("public") || word("private")) {
                skip_space();
            }
            if (!word("enum")) {
                continue;
            }
            skip_space();
            std::string name;
            while (i < line.size() && is_identifier_part(line[i])) {
                name.push_back(line[i++]);
            }
            if (!name.empty()) {
                module_names_.insert(name);  // `EnumName.Member`
                enum_names_.push_back(std::move(name));
            }
        }
    }

    void set_max_procedure_depth(const std::size_t depth) noexcept { max_procedure_depth_ = depth; }

    // Native stack accounting for deep recursion: `base` is an address near
    // the top of the (downward-growing) stack, `budget` the bytes that may be
    // used before a call reports "Out of stack space".
    void set_stack_budget(const char* const base, const std::size_t budget) noexcept {
        stack_base_ = base;
        stack_budget_ = budget;
    }

    [[nodiscard]] bool stack_nearly_exhausted() const noexcept {
        if (stack_budget_ == 0U) return false;
        char marker = 0;
        const char* const here = &marker;
        return stack_base_ > here && static_cast<std::size_t>(stack_base_ - here) > stack_budget_;
    }

    [[nodiscard]] wfc::Evaluation evaluate() {
        auto result = evaluate_program_text();
        result.debug_output = debug_output_;
        if (!result.success) {
            result.partial_output = output_;  // what the program printed before it failed
            const auto saved_error = std::move(error_);
            error_ = result;
            const Integer number = runtime_error_number();
            if (number != 0) {
                result.vb_error_number = number;
                result.vb_error_description =
                    std::string_view(result.diagnostic).substr(0, 7) == "WFC0300"
                        ? err_description_
                        : vb_error_description(number);
            }
            error_ = saved_error;
        }
        return result;
    }

    // The class module (display name) whose source was executing when the last error was
    // raised; empty for the standard module(s).
    void set_vb_number_spacing(const bool enabled) noexcept { vb_number_spacing_ = enabled; }

    [[nodiscard]] std::string failing_module_name() const {
        if (error_source_data_ == nullptr || error_source_data_ == main_source_data_) {
            return {};
        }
        for (const auto& [name, definition] : class_definitions_) {
            if (definition.source.data() == error_source_data_) {
                return definition.display_name;
            }
        }
        return {};
    }

    [[nodiscard]] wfc::Evaluation evaluate_program_text() {
        scan_option_explicit();
        scan_deftypes();
        scan_module_names();
        scan_enum_names();
        scan_udt_types();
        scan_builtin_classes();
        if (!scan_classes()) {
            return std::move(error_);
        }
        if (!scan_procedures()) {
            return std::move(error_);
        }
        skip_program_leading_trivia();
        if (at_end()) {
            return failure("WFC0001", "expected statement", offset_);
        }
        while (!at_end()) {
            if (!parse_statement()) {
                if (take_pending_jump()) {
                    skip_program_leading_trivia();
                    continue;
                }
                if (end_requested_) {
                    error_ = wfc::Evaluation{};
                    break;
                }
                return std::move(error_);
            }
            if (!consume_statement_end()) {
                return std::move(error_);
            }
            skip_program_leading_trivia();
        }

        // Terminate any module-level variable still holding the last
        // reference to an instance, the same way a call frame's locals are
        // drained at the end of a call -- but only on this successful
        // completion path; the interpreter is already erroring out on every
        // other return in this function, and running more class-member code
        // during that unwind is more likely to compound the failure than
        // clean up after it.
        if (!drain_scope_instances(module_scope())) {
            return std::move(error_);
        }
        // REQ-0230: a `Static` Variant local (REQ-0206) can already hold an
        // object reference via `Set` (REQ-0200), persisted on its own
        // procedure's/class-member's `ProcedureDef::statics` -- storage
        // this evaluator never previously drained at all, unlike the
        // module scope just above, so `Class_Terminate` would never run
        // for an instance only ever reachable through one at program end.
        for (auto& [name, definition] : procedures_) {
            if (!drain_scope_instances(definition.statics)) {
                return std::move(error_);
            }
        }
        for (auto& [class_name, class_def] : class_definitions_) {
            for (auto& [member_name, definition] : class_def.methods) {
                if (!drain_scope_instances(definition.statics)) {
                    return std::move(error_);
                }
            }
            for (auto& [member_name, definition] : class_def.property_get) {
                if (!drain_scope_instances(definition.statics)) {
                    return std::move(error_);
                }
            }
            for (auto& [member_name, definition] : class_def.property_let) {
                if (!drain_scope_instances(definition.statics)) {
                    return std::move(error_);
                }
            }
            for (auto& [member_name, definition] : class_def.property_set) {
                if (!drain_scope_instances(definition.statics)) {
                    return std::move(error_);
                }
            }
        }

        wfc::Evaluation result;
        result.success = true;
        result.output = std::move(output_);
        return result;
    }

private:
    [[nodiscard]] bool at_end() const noexcept { return offset_ == source_.size(); }
    [[nodiscard]] char current() const noexcept { return source_[offset_]; }
    [[nodiscard]] char peek(const std::size_t ahead) const noexcept {
        const auto index = offset_ + ahead;
        return index < source_.size() ? source_[index] : '\0';
    }
    void advance() noexcept { ++offset_; }

    void skip_horizontal_whitespace() noexcept {
        while (!at_end() && (current() == ' ' || current() == '\t' || current() == '\f' ||
                             current() == '\v')) {
            advance();
        }
    }

    [[nodiscard]] bool consume_line_break() noexcept {
        if (at_end()) {
            return false;
        }
        if (current() == '\r') {
            advance();
            if (!at_end() && current() == '\n') {
                advance();
            }
            return true;
        }
        if (current() == '\n') {
            advance();
            return true;
        }
        return false;
    }

    // Skips to the end of the current statement (a ':' or line end outside a string).
    void skip_comment_free_statement_text() noexcept {
        bool in_string = false;
        while (!at_end() && current() != '\r' && current() != '\n') {
            if (current() == '"') {
                in_string = !in_string;
            } else if (!in_string && (current() == ':' || current() == '\'')) {
                break;
            }
            advance();
        }
    }

    void skip_comment() noexcept {
        while (!at_end() && current() != '\r' && current() != '\n') {
            advance();
        }
    }

    void skip_program_leading_trivia() noexcept {
        while (true) {
            skip_horizontal_whitespace();
            if (!at_end() && current() == '\'') {
                skip_comment();
            }
            if (!consume_line_break()) {
                return;
            }
        }
    }

    // Advances past the rest of the current logical line (its content is
    // not this call's concern) and the line break that ends it.
    void skip_rest_of_line() noexcept {
        skip_comment();  // skip_comment already just means "to end of line"
        static_cast<void>(consume_line_break());
    }

    // Scans forward from the current position (immediately after a
    // procedure's parameter list/return type and its terminating line
    // break) for a line consisting of `End <keyword>` (`keyword` is "sub"
    // or "function"), which is guaranteed to be this procedure's own
    // terminator: VB6 does not allow a Sub/Function to nest another one, so
    // no block-depth tracking is needed, only recognizing "End sub"/"End
    // function" pairs distinct from unrelated "End If"/"End Select"/etc.
    // On success, `body_end` is set to the offset where "End" begins (after
    // any leading blank lines/comments) and the cursor is left right after
    // consuming the terminator's own line break.
    [[nodiscard]] bool skip_to_matching_end(
        const std::string_view keyword, std::size_t& body_end) {
        while (true) {
            skip_program_leading_trivia();
            if (at_end()) {
                return false;
            }
            const auto line_offset = offset_;
            if (consume_keyword("end")) {
                skip_horizontal_whitespace();
                if (consume_keyword(keyword)) {
                    body_end = line_offset;
                    // Leave the cursor right after "End <keyword>", not
                    // past its line break: the caller (parse_statement,
                    // when skipping a declaration during the main pass;
                    // scan_procedures' own loop, when continuing to scan)
                    // is responsible for consuming that statement
                    // separator itself, matching every other statement
                    // handler in this evaluator.
                    return true;
                }
                offset_ = line_offset;
            }
            // `Sub F(): Print 1: End Sub` -- look for a `: End <keyword>` on this line.
            bool in_string = false;
            while (!at_end() && current() != '\r' && current() != '\n') {
                const char ch = current();
                if (ch == '"') {
                    in_string = !in_string;
                } else if (!in_string && ch == '\'') {
                    break;
                } else if (!in_string && ch == ':') {
                    advance();
                    skip_horizontal_whitespace();
                    const auto end_offset = offset_;
                    if (consume_keyword("end")) {
                        skip_horizontal_whitespace();
                        if (consume_keyword(keyword)) {
                            body_end = end_offset;
                            return true;
                        }
                    }
                    offset_ = end_offset;
                    continue;
                }
                advance();
            }
            skip_rest_of_line();
        }
    }

    // Parses `(` [`ByVal`|`ByRef`] name [`As` Type] {`,` ...} `)` into
    // `definition.parameters`, used by `scan_procedures` for both `Sub` and
    // `Function` declarations.
    [[nodiscard]] bool scan_procedure_parameters(ProcedureDef& definition) {
        skip_horizontal_whitespace();
        if (!consume('(')) {
            set_error(
                "WFC0005", "expected opening parenthesis after procedure name", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        if (consume(')')) {
            return true;
        }
        // REQ-0206: once an Optional parameter has been seen, every
        // parameter after it must itself be Optional or the trailing
        // ParamArray -- a required parameter cannot follow.
        bool seen_optional = false;
        while (true) {
            skip_horizontal_whitespace();
            const auto modifier_offset = offset_;
            ProcedureParameter parameter;
            bool is_param_array = false;
            bool is_optional = false;
            if (consume_keyword("paramarray")) {
                is_param_array = true;
                skip_horizontal_whitespace();
            } else if (consume_keyword("optional")) {
                is_optional = true;
                skip_horizontal_whitespace();
            }
            if (is_param_array) {
                // ParamArray is always effectively ByVal: each call builds
                // it a fresh array, so there is no caller variable for a
                // ByRef write-back to reach.
                parameter.by_val = true;
            } else if (consume_keyword("byval")) {
                parameter.by_val = true;
                skip_horizontal_whitespace();
            } else if (consume_keyword("byref")) {
                skip_horizontal_whitespace();
            }
            if (!is_optional && !is_param_array && seen_optional) {
                set_error(
                    "WFC0140",
                    "a required parameter cannot follow an Optional parameter",
                    modifier_offset);
                return false;
            }
            const auto parameter_name_offset = offset_;
            char type_character{};
            auto name = parse_identifier(&type_character);
            if (!name.has_value()) {
                set_error("WFC0011", "expected parameter name", parameter_name_offset);
                return false;
            }
            if (is_reserved_identifier(*name)) {
                set_error(
                    "WFC0017",
                    "reserved keyword cannot be a parameter name",
                    parameter_name_offset);
                return false;
            }
            parameter.name = std::move(*name);
            skip_horizontal_whitespace();
            if (is_param_array) {
                if (type_character != '\0' || !consume('(')) {
                    set_error(
                        "WFC0141",
                        "ParamArray parameter must be declared as an array: name()",
                        offset_);
                    return false;
                }
                skip_horizontal_whitespace();
                if (!consume(')')) {
                    set_error(
                        "WFC0141",
                        "ParamArray parameter must be declared as an array: name()",
                        offset_);
                    return false;
                }
                skip_horizontal_whitespace();
                const auto element_type_offset = offset_;
                const auto matched_as = consume_keyword("as");
                if (matched_as) {
                    skip_horizontal_whitespace();
                }
                const auto type_result = matched_as ? parse_type_keyword() : std::nullopt;
                if (matched_as && !type_result.has_value()) {
                    set_error(
                        "WFC0141",
                        "ParamArray requires an element type: As Integer, As Long, "
                        "As Double, As Single, As Currency, As String, As Boolean, or As Variant",
                        element_type_offset);
                    return false;
                }
                if (!matched_as || type_result->is_variant) {
                    // REQ-0248: an untyped / `As Variant` ParamArray is a
                    // Variant array.
                    parameter.is_variant = true;
                    parameter.type_index = Value{Empty{}}.index();
                } else {
                    parameter.type_index = type_result->default_value.index();
                }
                parameter.is_param_array = true;
            } else if (type_character == '\0' && !at_end() && current() == '(') {
                // A plain (non-ParamArray) array parameter, `name() As
                // Type` (REQ-0211): always effectively ByRef -- an array is
                // a reference-like value in real VB6, and this evaluator
                // has no other way to alias the caller's array -- so an
                // explicit `ByVal` here is rejected rather than silently
                // ignored.
                advance();
                skip_horizontal_whitespace();
                if (!consume(')')) {
                    set_error(
                        "WFC0149",
                        "array parameter must be declared as an array: name()",
                        offset_);
                    return false;
                }
                skip_horizontal_whitespace();
                const auto element_type_offset = offset_;
                const auto matched_as = consume_keyword("as");
                if (matched_as) {
                    skip_horizontal_whitespace();
                }
                // REQ-0215: `name() As Object` is not one of
                // parse_type_keyword's own scalars (it only knows the
                // eight `Dim`-style keywords, `Variant` included), so it
                // is checked separately first, mirroring the same
                // separate check a generic `As Object` scalar parameter
                // already uses.
                bool matched_object = matched_as && consume_keyword("object");
                std::string array_class_name;
                if (matched_as && !matched_object) {
                    // REQ-0267: `name() As ClassOrUdt`.
                    const auto class_probe = offset_;
                    char class_type_character{};
                    auto class_name = parse_identifier(&class_type_character);
                    if (class_name.has_value() && class_type_character == '\0' &&
                        class_definitions_.contains(*class_name)) {
                        matched_object = true;
                        array_class_name = std::move(*class_name);
                    } else {
                        offset_ = class_probe;
                    }
                }
                const auto type_result =
                    (matched_as && !matched_object) ? parse_type_keyword() : std::nullopt;
                if (!matched_as || (!matched_object && !type_result.has_value())) {
                    set_error(
                        "WFC0149",
                        "array parameter requires an explicit element type: As Integer, As "
                        "Long, As Double, As Single, As Currency, As String, As Boolean, As "
                        "Object, or As Variant",
                        element_type_offset);
                    return false;
                }
                if (parameter.by_val) {
                    set_error(
                        "WFC0149", "array parameters must be passed ByRef", modifier_offset);
                    return false;
                }
                if (is_optional) {
                    set_error(
                        "WFC0149", "array parameters cannot be Optional", modifier_offset);
                    return false;
                }
                parameter.is_array_parameter = true;
                if (matched_object) {
                    parameter.is_object_array_parameter = true;
                    parameter.class_name = array_class_name;
                } else if (type_result->is_variant) {
                    parameter.is_variant_array_parameter = true;
                } else {
                    parameter.type_index = type_result->default_value.index();
                }
            } else if (type_character != '\0') {
                if (!validate_type_character(type_character, parameter_name_offset)) {
                    return false;
                }
                parameter.type_index = type_character_index(type_character);
            } else if (consume_keyword("as")) {
                skip_horizontal_whitespace();
                const auto type_offset = offset_;
                // REQ-0228: `As Object`/`As SomeClassName` are now accepted
                // for any Sub/Function/Property parameter (not just
                // Property Set's own single value parameter, previously
                // the only caller that allowed `As Object` here), via the
                // same resolver a class-typed field/return type already
                // uses.
                const auto type_result = parse_scalar_object_or_class_type();
                if (!type_result.has_value()) {
                    set_error(
                        "WFC0012",
                        "expected As Integer, As Long, As Double, As Single, As Currency, As "
                        "String, As Boolean, As Object, or As Variant",
                        type_offset);
                    return false;
                }
                if (type_result->is_object) {
                    parameter.is_object_reference = true;
                    parameter.type_index = type_result->type_index;
                    parameter.class_name = std::move(type_result->class_name);
                } else {
                    parameter.type_index = type_result->type_index;
                    parameter.is_variant = type_result->is_variant;
                }
            } else {
                // A bare parameter with no As clause and no type character
                // is implicitly Variant, matching real VB6.
                if (const auto default_type = default_type_for(parameter.name)) {
                    parameter.type_index = *default_type;
                } else {
                    parameter.type_index = Value{Empty{}}.index();
                    parameter.is_variant = true;
                }
            }

            if (is_optional) {
                parameter.is_optional = true;
                seen_optional = true;
                skip_horizontal_whitespace();
                if (consume('=')) {
                    skip_horizontal_whitespace();
                    const auto default_offset = offset_;
                    const bool enclosing_constant_expression = constant_expression_;
                    constant_expression_ = true;
                    auto default_value = parse_expression();
                    constant_expression_ = enclosing_constant_expression;
                    if (!default_value.has_value()) {
                        return false;
                    }
                    if (!parameter.is_variant) {
                        if (!coerce_numeric_value(
                                *default_value, parameter.type_index, default_offset)) {
                            return false;
                        }
                        if (default_value->index() != parameter.type_index) {
                            set_error(
                                "WFC0016", "default value type mismatch", default_offset);
                            return false;
                        }
                    }
                    parameter.has_default = true;
                    parameter.default_value = std::move(*default_value);
                }
            }

            definition.parameters.push_back(std::move(parameter));
            skip_horizontal_whitespace();
            if (consume(')')) {
                break;
            }
            if (is_param_array) {
                set_error("WFC0141", "ParamArray must be the last parameter", offset_);
                return false;
            }
            if (!consume(',')) {
                set_error("WFC0005", "expected closing parenthesis", offset_);
                return false;
            }
        }
        return true;
    }

    // Returns whether `name` is already used anywhere in `class_def` (as a
    // field, a method, or any Property accessor), regardless of kind --
    // used to reject a field or method name that collides with any existing
    // member. A Property accessor's own duplicate check is narrower (see
    // scan_class_body): Get/Let/Set of the *same* property name are meant
    // to coexist.
    [[nodiscard]] static bool class_member_name_used(const ClassDef& class_def, const std::string& name) {
        return class_def.fields.contains(name) || class_def.methods.contains(name) ||
               class_def.property_get.contains(name) || class_def.property_let.contains(name) ||
               class_def.property_set.contains(name);
    }

    // Parses one already-installed class module source (see scan_classes)
    // top-to-bottom, finding its field declarations, `Sub`/`Function`
    // methods, and `Property Get`/`Let`/`Set` accessors -- mirroring
    // scan_procedures' pre-pass role (locating declarations and body
    // ranges, never executing anything), extended with the field and
    // Property forms a class body can also contain. Unlike a standard
    // module, a class body has no other top-level content at all: no
    // executable statements, since nothing calls a class module's own body
    // directly (see REQ-0203's Scope).
    //
    // A line beginning `Dim` always introduces a field, implicitly
    // `Private` (matching real VB6's own module-level default for a bare
    // `Dim`). A line beginning `Public`/`Private` is ambiguous on its own --
    // `Public x As Long` is a field, `Public Sub Name()` is a method -- so
    // the modifier is consumed first and then either `Property`/`Sub`/
    // `Function` or a field name is looked for (REQ-0206; visibility was
    // entirely unmodeled before it, with every member reachable via `.`
    // regardless of the `Dim`/`Public` keyword written).
    [[nodiscard]] bool scan_class_body(ClassDef& class_def) {
        while (true) {
            skip_program_leading_trivia();
            if (at_end()) {
                return true;
            }
            const auto line_offset = offset_;

            if (consume_keyword("implements")) {
                skip_horizontal_whitespace();
                const auto interface_name_offset = offset_;
                char interface_type_character{};
                auto interface_name = parse_identifier(&interface_type_character);
                if (!interface_name.has_value() || interface_type_character != '\0') {
                    set_error("WFC0011", "expected interface name after Implements",
                              interface_name_offset);
                    return false;
                }
                if (!class_definitions_.contains(*interface_name)) {
                    set_error("WFC0134", "unknown class name", interface_name_offset);
                    return false;
                }
                class_def.implements.push_back(std::move(*interface_name));
                continue;
            }

            if (consume_keyword("dim")) {
                if (!scan_class_field_declaration(class_def, /*is_private=*/true)) {
                    return false;
                }
                continue;
            }

            if (consume_keyword("option")) {
                // `Option Explicit` (and friends) at the top of a class
                // module file; accepted and ignored.
                skip_rest_of_line();
                continue;
            }
            {
                // REQ-0260: class-level `[Public|Private|Friend] Const|Enum`.
                const auto before_constant = offset_;
                const bool constant_is_private = consume_keyword("private");
                if (!constant_is_private) {
                    static_cast<void>(consume_keyword("public") || consume_keyword("friend"));
                }
                skip_horizontal_whitespace();
                const bool is_const = consume_keyword("const");
                const bool is_enum = !is_const && consume_keyword("enum");
                if (is_const || is_enum) {
                    scopes_.emplace_back();
                    const bool parsed_ok =
                        is_const ? parse_constant_declaration() : parse_enum_statement(line_offset);
                    Scope captured = std::move(scopes_.back());
                    scopes_.pop_back();
                    if (!parsed_ok) {
                        return false;
                    }
                    for (auto& [constant_name, constant_value] : captured.variables) {
                        // A Public Enum/Const in a class module is visible program-wide.
                        if (!constant_is_private) {
                            global_class_constants_.emplace(constant_name, constant_value);
                            module_names_.insert(current_class_scan_name_);  // `Cls.Member`
                        }
                        class_def.constants[constant_name] = std::move(constant_value);
                    }
                    if (!consume_statement_end() && !is_enum) {
                        return false;
                    }
                    continue;
                }
                offset_ = before_constant;
            }
            bool is_private = false;
            bool has_visibility_keyword = false;
            if (consume_keyword("private")) {
                is_private = true;
                has_visibility_keyword = true;
                skip_horizontal_whitespace();
            } else if (consume_keyword("public") || consume_keyword("friend")) {
                has_visibility_keyword = true;
                skip_horizontal_whitespace();
            }
            if (consume_keyword("static")) {
                pending_static_procedure_ = true;
                skip_horizontal_whitespace();
            }

            if (consume_keyword("event")) {
                skip_horizontal_whitespace();
                const auto event_name_offset = offset_;
                char event_type_character{};
                auto event_name = parse_identifier(&event_type_character);
                if (!event_name.has_value() || event_type_character != '\0') {
                    set_error("WFC0011", "expected event name", event_name_offset);
                    return false;
                }
                if (class_def.events.contains(*event_name) ||
                    class_member_name_used(class_def, *event_name)) {
                    set_error("WFC0128", "duplicate or reserved class member name",
                              event_name_offset);
                    return false;
                }
                ProcedureDef definition;
                if (!scan_procedure_parameters(definition) || !consume_statement_end()) {
                    return false;
                }
                class_def.events.emplace(std::move(*event_name), std::move(definition));
                continue;
            }
            if (consume_keyword("property")) {
                if (!scan_class_property_declaration(class_def, is_private, line_offset)) {
                    return false;
                }
                continue;
            }
            bool is_function = false;
            bool matched_sub_or_function = true;
            if (consume_keyword("sub")) {
                is_function = false;
            } else if (consume_keyword("function")) {
                is_function = true;
            } else {
                matched_sub_or_function = false;
            }
            if (matched_sub_or_function) {
                if (!scan_class_procedure_declaration(
                        class_def, is_function, is_private, line_offset)) {
                    return false;
                }
                continue;
            }
            if (has_visibility_keyword) {
                if (!scan_class_field_declaration(class_def, is_private)) {
                    return false;
                }
                continue;
            }
            set_error(
                "WFC0127",
                "expected a class member declaration (Dim, Public, Private, Sub, Function, "
                "or Property)",
                line_offset);
            return false;
        }
    }

    // `[Dim|Public|Private] name [As Type]` -- the keyword itself has
    // already been consumed by scan_class_body, which also determined
    // `is_private`.
    // `Private a As Long, b As String` declares several fields on one line.
    [[nodiscard]] bool scan_class_field_declaration(ClassDef& class_def, const bool is_private) {
        bool more = true;
        while (more) {
            if (!scan_class_field_declarator(class_def, is_private, more)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool scan_class_field_declarator(
        ClassDef& class_def, const bool is_private, bool& more) {
        skip_horizontal_whitespace();
        const bool with_events = consume_keyword("withevents");
        skip_horizontal_whitespace();
        const auto name_offset = offset_;
        char type_character{};
        auto name = parse_identifier(&type_character);
        if (!name.has_value() || type_character != '\0') {
            set_error("WFC0011", "expected field name", name_offset);
            return false;
        }
        if (is_reserved_member_name(*name) || class_member_name_used(class_def, *name)) {
            set_error("WFC0128", "duplicate or reserved class member name", name_offset);
            return false;
        }
        skip_horizontal_whitespace();
        ClassFieldDef field;
        field.is_private = is_private;
        field.with_events = with_events;
        if (!at_end() && current() == '(') {
            advance();
            skip_horizontal_whitespace();
            field.is_array = true;
            if (!at_end() && current() == ')') {
                advance();
            } else {
                auto bounds = parse_fixed_array_bounds(name_offset);
                if (!bounds.has_value()) {
                    return false;
                }
                field.dimensions = std::move(*bounds);
            }
            skip_horizontal_whitespace();
        }
        if (consume_keyword("as")) {
            skip_horizontal_whitespace();
            field.auto_new = consume_keyword("new");
            skip_horizontal_whitespace();
            const auto type_offset = offset_;
            const auto type_result = parse_scalar_object_or_class_type();
            if (!type_result.has_value() || (field.auto_new && !type_result->is_object)) {
                set_error(
                    "WFC0012",
                    "expected As Integer, As Long, As Double, As Single, As Currency, "
                    "As String, As Boolean, As Object, As Variant, or a known class "
                    "name",
                    type_offset);
                return false;
            }
            field.type_index = type_result->type_index;
            field.is_variant = type_result->is_variant;
            field.is_object = type_result->is_object;
            field.class_name = type_result->class_name;
            skip_horizontal_whitespace();
            if (!field.is_array && !field.is_variant && !field.is_object &&
                field.type_index == Value{std::string{}}.index() && consume('*')) {
                skip_horizontal_whitespace();
                std::size_t length = 0;
                std::size_t digits = 0;
                while (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0) {
                    length = length * 10U + static_cast<std::size_t>(current() - '0');
                    advance();
                    ++digits;
                }
                if (digits == 0 || length == 0 || length > 65535U) {
                    set_error("WFC0012", "expected a fixed string length", offset_);
                    return false;
                }
                field.fixed_length = length;
            }
        } else {
            // A bare field declaration with no As clause is implicitly
            // Variant, matching a bare module-level Dim.
            field.type_index = Value{Empty{}}.index();
            field.is_variant = true;
        }
        // A field declaration is a plain statement line, not a block
        // opener -- unlike Sub/Function/Property (which always need a body
        // to follow, so consume_block_line_end's "a line break is
        // mandatory" requirement is right for them), a class's last field
        // can legally be its source's very last line with no trailing line
        // break.
        skip_horizontal_whitespace();
        more = consume(',');
        if (!more && !consume_statement_end()) {
            return false;
        }
        class_def.field_order.push_back(*name);
        class_def.fields.emplace(std::move(*name), std::move(field));
        return true;
    }

    // `Property Get|Let|Set name(...) [As Type] ... End Property` -- the
    // `Property` keyword has already been consumed by scan_class_body,
    // which also determined `is_private`.
    [[nodiscard]] bool scan_class_property_declaration(
        ClassDef& class_def, const bool is_private, const std::size_t line_offset) {
        skip_horizontal_whitespace();
        enum class Accessor { get, let, set };
        Accessor accessor;
        if (consume_keyword("get")) {
            accessor = Accessor::get;
        } else if (consume_keyword("let")) {
            accessor = Accessor::let;
        } else if (consume_keyword("set")) {
            accessor = Accessor::set;
        } else {
            set_error("WFC0129", "expected Get, Let, or Set after Property", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        const auto name_offset = offset_;
        char type_character{};
        auto name = parse_identifier(&type_character);
        if (!name.has_value() || type_character != '\0') {
            set_error("WFC0118", "expected property name", name_offset);
            return false;
        }
        if (is_reserved_member_name(*name) || class_def.fields.contains(*name) ||
            class_def.methods.contains(*name)) {
            set_error(
                "WFC0128", "duplicate or reserved class member name", name_offset);
            return false;
        }
        auto& accessor_table = accessor == Accessor::get   ? class_def.property_get
                                : accessor == Accessor::let ? class_def.property_let
                                                             : class_def.property_set;
        if (accessor_table.contains(*name)) {
            set_error(
                "WFC0128", "duplicate Property accessor for this name", name_offset);
            return false;
        }

        ProcedureDef definition;
        definition.is_function = accessor == Accessor::get;
        definition.is_private = is_private;
        definition.static_locals = std::exchange(pending_static_procedure_, false);
        if (!scan_procedure_parameters(definition)) {
            return false;
        }
        if (accessor == Accessor::get) {
            // Property Get may take zero or more index parameters
            // (REQ-0205's indexed properties -- `Property Get
            // Name(index [, ...]) As Type`, accessed as
            // `obj.Name(args)` exactly like a method call); zero
            // parameters is the plain, non-indexed form accessed as
            // `obj.Name` with no parentheses at all.
            skip_horizontal_whitespace();
            if (!consume_keyword("as")) {
                apply_implicit_return_type(definition, *name);
            } else {
            const auto type_offset = offset_;
            const auto type_result = parse_scalar_object_or_class_type();
            if (!type_result.has_value()) {
                set_error(
                    "WFC0012",
                    "expected As Integer, As Long, As Double, As Single, As Currency, "
                    "As String, As Boolean, As Object, As Variant, or a known class "
                    "name",
                    type_offset);
                return false;
            }
            definition.return_type_index = type_result->type_index;
            definition.return_is_variant = type_result->is_variant;
            definition.return_is_object = type_result->is_object;
            definition.return_class_name = type_result->class_name;
            }
        } else {
            // Property Let/Set's last parameter is always the value
            // being assigned; any parameters before it are index
            // parameters (REQ-0205), so at least one parameter (the
            // value alone, for the plain non-indexed form) is
            // required, but there is no upper bound.
            if (definition.parameters.empty()) {
                set_error(
                    "WFC0131",
                    "Property Let/Set requires at least one parameter",
                    name_offset);
                return false;
            }
            if (accessor == Accessor::set &&
                !definition.parameters.back().is_object_reference) {
                set_error(
                    "WFC0132",
                    "Property Set's last parameter (the value) must be declared As "
                    "Object",
                    name_offset);
                return false;
            }
        }
        if (!consume_loop_header_end()) {
            return false;
        }
        definition.body_start = offset_;
        if (!skip_to_matching_end("property", definition.body_end)) {
            set_error("WFC0133", "expected End Property", line_offset);
            return false;
        }
        definition.declaration_end = offset_;
        accessor_table.emplace(std::move(*name), std::move(definition));
        return true;
    }

    // `Sub|Function name(...) [As Type] ... End Sub|Function` -- the
    // `Sub`/`Function` keyword has already been consumed by scan_class_body,
    // which also determined `is_private`.
    [[nodiscard]] bool scan_class_procedure_declaration(
        ClassDef& class_def, const bool is_function, const bool is_private,
        const std::size_t line_offset) {
        skip_horizontal_whitespace();
        const auto name_offset = offset_;
        char type_character{};
        auto name = parse_identifier(&type_character);
        if (!name.has_value() || (type_character != '\0' && !is_function)) {
            set_error("WFC0118", "expected procedure name", name_offset);
            return false;
        }
        if (is_reserved_member_name(*name) || class_member_name_used(class_def, *name)) {
            set_error("WFC0128", "duplicate or reserved class member name", name_offset);
            return false;
        }
        ProcedureDef definition;
        definition.is_function = is_function;
        definition.is_private = is_private;
        definition.static_locals = std::exchange(pending_static_procedure_, false);
        if (!scan_procedure_parameters(definition)) {
            return false;
        }
        if (is_function) {
            skip_horizontal_whitespace();
            if (!consume_keyword("as")) {
                apply_implicit_return_type(definition, *name, type_character);
            } else {
            const auto type_offset = offset_;
            const auto type_result = parse_scalar_object_or_class_type();
            if (!type_result.has_value()) {
                set_error(
                    "WFC0012",
                    "expected As Integer, As Long, As Double, As Single, As Currency, "
                    "As String, As Boolean, As Object, As Variant, or a known class name",
                    type_offset);
                return false;
            }
            definition.return_type_index = type_result->type_index;
            definition.return_is_variant = type_result->is_variant;
            definition.return_is_object = type_result->is_object;
            definition.return_class_name = type_result->class_name;
            const auto array_marker = parse_function_array_return_marker(*type_result, type_offset);
            if (!array_marker.has_value()) {
                return false;
            }
            definition.return_is_array = *array_marker;
            }
        }
        if (!consume_loop_header_end()) {
            return false;
        }
        definition.body_start = offset_;
        if (!skip_to_matching_end(is_function ? "function" : "sub", definition.body_end)) {
            set_error(
                is_function ? "WFC0120" : "WFC0121",
                is_function ? "expected End Function" : "expected End Sub",
                line_offset);
            return false;
        }
        definition.declaration_end = offset_;
        // Class_Initialize/Class_Terminate are the lifecycle hooks New
        // (instantiate_class) and drain_scope_instances/
        // terminate_if_last_reference invoke automatically -- both
        // always call with zero arguments, so a declaration with
        // parameters (or written as a Function) could never actually
        // run correctly and is rejected up front instead.
        if ((*name == "class_initialize" || *name == "class_terminate") &&
            (definition.is_function || !definition.parameters.empty())) {
            set_error(
                "WFC0139",
                "Class_Initialize/Class_Terminate must be a parameterless Sub",
                name_offset);
            return false;
        }
        class_def.methods.emplace(std::move(*name), std::move(definition));
        return true;
    }

    // Scans every class module source supplied alongside the standard
    // module (see wfc::ClassModuleSource), registering each one's
    // definition in class_definitions_ before the standard module's own
    // scan_procedures/execution begins, so `New ClassName` and `As
    // ClassName` can resolve regardless of textual order between the
    // classes and the module. Temporarily installs each class's own source
    // into source_/offset_ (restored afterward) since ProcedureDef's
    // body_start/body_end offsets are only meaningful against the source
    // they were scanned from -- the same reason call_procedure later swaps
    // source_ back in before running a method/property body.
    //
    // Two passes: the first registers every class's name (and its own
    // source, needed by the second pass) with no member scanning at all, so
    // the second pass -- which actually scans each class's fields/methods/
    // properties, and therefore needs to recognize `As SomeClass`
    // field/return types (REQ-0205) -- sees every class name up front,
    // regardless of which `--class` argument came first. Without this, a
    // class referencing another declared *after* it on the command line
    // would see an unresolved name where a real VB6 project (compiled as a
    // whole) would not.
    // A UDT stored into a Variant (or passed to a Variant parameter) is copied.
    [[nodiscard]] Value copy_if_udt(Value value) {
        if (const auto* instance = std::get_if<ObjectInstance>(&value)) {
            if (is_udt_class(instance->data->class_name)) {
                return Value{ObjectInstance{clone_udt(*instance->data)}};
            }
        } else if (std::holds_alternative<ArrayValue>(value)) {
            deep_copy_udt_values(value);  // an array of UDTs copies its elements
        }
        return value;
    }

    [[nodiscard]] bool is_udt_class(const std::string& class_name) const {
        const auto found = class_definitions_.find(class_name);
        return found != class_definitions_.end() && found->second.is_udt;
    }

    // Deep-copies a UDT instance's fields (nested UDT fields get their own
    // fresh copies, matching value semantics).
    [[nodiscard]] std::shared_ptr<InstanceData> clone_udt(const InstanceData& source) {
        auto copy = std::make_shared<InstanceData>();
        copy->class_name = source.class_name;
        copy->fields = source.fields;
        for (auto& [name, value] : copy->fields.variables) {
            deep_copy_udt_values(value);
        }
        return copy;
    }

    // Gives every UDT instance inside `value` (itself, or the elements of an
    // array) its own copy, so value semantics hold for arrays of UDTs.
    void deep_copy_udt_values(Value& value) {
        if (const auto* nested = std::get_if<ObjectInstance>(&value)) {
            if (is_udt_class(nested->data->class_name)) {
                value = Value{ObjectInstance{clone_udt(*nested->data)}};
            }
        } else if (auto* array = std::get_if<ArrayValue>(&value)) {
            for (auto& element : array->elements) {
                deep_copy_udt_values(element);
            }
        }
    }

    // `target = source` where both are UDT instances of the same type.
    [[nodiscard]] bool assign_udt(Value& target, const Value& source, const std::size_t offset) {
        if (!execute_) {
            return true;  // dry run: operands are placeholders
        }
        const auto* destination = std::get_if<ObjectInstance>(&target);
        const auto* origin = std::get_if<ObjectInstance>(&source);
        if (destination == nullptr || origin == nullptr ||
            destination->data->class_name != origin->data->class_name) {
            set_error("WFC0016", "assignment type mismatch", offset);
            return false;
        }
        if (execute_ && destination->data != origin->data) {
            destination->data->fields = clone_udt(*origin->data)->fields;
        }
        return true;
    }

    // Finds `[Public|Private] Type Name ... End Type` blocks in the main
    // source and registers each as a value-semantics class (REQ-0241).
    void scan_udt_types() {
        const std::string_view text = source_;
        std::size_t position = 0;
        std::string current_name;
        std::string body;
        while (position < text.size()) {
            auto end = text.find('\n', position);
            if (end == std::string_view::npos) {
                end = text.size();
            }
            std::string line;
            for (const char c : text.substr(position, end - position)) {
                if (c == '\'') break;
                if (c != '\r') line.push_back(c);
            }
            position = end + 1;
            std::size_t i = 0;
            const auto skip_space = [&] {
                while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
            };
            const auto word = [&](const std::string_view w) {
                if (line.size() - i < w.size()) return false;
                for (std::size_t k = 0; k < w.size(); ++k)
                    if (ascii_lower(line[i + k]) != w[k]) return false;
                if (i + w.size() < line.size() && is_identifier_part(line[i + w.size()])) return false;
                i += w.size();
                return true;
            };
            skip_space();
            if (current_name.empty()) {
                if (word("public") || word("private")) skip_space();
                if (!word("type")) continue;
                skip_space();
                while (i < line.size() && is_identifier_part(line[i])) current_name.push_back(line[i++]);
                body.clear();
                continue;
            }
            if (word("end")) {
                skip_space();
                if (word("type")) {
                    udt_sources_.push_back(std::move(body));
                    class_sources_.push_back({current_name, udt_sources_.back()});
                    udt_names_.push_back(current_name);
                    current_name.clear();
                    continue;
                }
            }
            if (i >= line.size()) continue;
            body += "Public " + line.substr(i) + "\n";
        }
    }

    // REQ-0243: registers the built-in Collection when the program mentions
    // it and does not define its own class of that name.
    void scan_builtin_classes() {
        const auto mentions = [](const std::string_view text) {
            constexpr std::string_view word = "collection";
            for (std::size_t i = 0; i + word.size() <= text.size(); ++i) {
                std::size_t k = 0;
                while (k < word.size() && ascii_lower(text[i + k]) == word[k]) ++k;
                if (k == word.size() && (i == 0 || !is_identifier_part(text[i - 1])) &&
                    (i + k == text.size() || !is_identifier_part(text[i + k]))) {
                    return true;
                }
            }
            return false;
        };
        bool needed = mentions(source_);
        for (const auto& module : class_sources_) {
            if (module.name.size() == 10 && mentions(module.name)) {
                return;  // user-defined Collection
            }
            needed = needed || mentions(module.source);
        }
        if (needed) {
            class_sources_.push_back({"Collection", kCollectionSource});
        }
        const auto text_mentions = [](const std::string_view text, const std::string_view needle,
                                      const bool whole_word) {
            for (std::size_t i = 0; i + needle.size() <= text.size(); ++i) {
                std::size_t k = 0;
                while (k < needle.size() && ascii_lower(text[i + k]) == needle[k]) ++k;
                if (k == needle.size() &&
                    (!whole_word ||
                     ((i == 0 || !is_identifier_part(text[i - 1])) &&
                      (i + k == text.size() || !is_identifier_part(text[i + k]))))) {
                    return true;
                }
            }
            return false;
        };
        const auto any_source_mentions = [&](const std::string_view needle, const bool whole_word) {
            bool found = text_mentions(source_, needle, whole_word);
            for (const auto& module : class_sources_) {
                found = found || text_mentions(module.source, needle, whole_word);
            }
            return found;
        };
        const auto user_defines = [&](const std::string_view name) {
            for (const auto& module : class_sources_) {
                if (module.name.size() == name.size()) {
                    bool same = true;
                    for (std::size_t i = 0; i < name.size(); ++i) {
                        same = same && ascii_lower(module.name[i]) == name[i];
                    }
                    if (same) return true;
                }
            }
            return false;
        };
        // Each built-in is registered under an internal `Wfc` name (what CreateObject builds)
        // and, unless the program defines its own class of that name, the public type name too
        // (`Dim d As New Dictionary`, `As Scripting.Dictionary`).
        if (any_source_mentions("scripting.dictionary", false) ||
            any_source_mentions("dictionary", true)) {
            const bool alias = !user_defines("dictionary");
            class_sources_.push_back({"WfcDictionary", kDictionarySource});
            if (alias) class_sources_.push_back({"Dictionary", kDictionarySource});
        }
        if (any_source_mentions("scripting.filesystemobject", false) ||
            any_source_mentions("filesystemobject", true)) {
            const bool alias = !user_defines("filesystemobject");
            class_sources_.push_back({"WfcTextStream", kTextStreamSource});
            class_sources_.push_back({"WfcFile", kFileObjectSource});
            class_sources_.push_back({"WfcFileSystemObject", kFileSystemObjectSource});
            if (alias) class_sources_.push_back({"FileSystemObject", kFileSystemObjectSource});
        }
        if (any_source_mentions("vbscript.regexp", false) ||
            any_source_mentions("vbscript_regexp_55.regexp", false) ||
            any_source_mentions("regexp", true)) {
            const bool alias = !user_defines("regexp");
            class_sources_.push_back({"WfcRegExp", kRegExpSource});
            class_sources_.push_back({"WfcMatchCollection", kMatchCollectionSource});
            class_sources_.push_back({"WfcMatch", kMatchSource});
            class_sources_.push_back({"WfcSubMatches", kSubMatchesSource});
            if (alias) class_sources_.push_back({"RegExp", kRegExpSource});
        }
        const auto mentions_app = [](const std::string_view text) {
            for (std::size_t i = 0; i + 4U <= text.size(); ++i) {
                if (ascii_lower(text[i]) == 'a' && ascii_lower(text[i + 1U]) == 'p' &&
                    ascii_lower(text[i + 2U]) == 'p' && text[i + 3U] == '.' &&
                    (i == 0 || !is_identifier_part(text[i - 1U]))) {
                    return true;
                }
            }
            return false;
        };
        bool needs_app = mentions_app(source_);
        for (const auto& module : class_sources_) {
            needs_app = needs_app || mentions_app(module.source);
        }
        if (needs_app) {
            class_sources_.push_back({"WfcApp", kAppSource});
        }
    }

    // REQ-0257: finds `Attribute Name.VB_UserMemId = 0` in a class source.
    static void scan_default_member(ClassDef& class_def) {
        const std::string_view text = class_def.source;
        std::size_t position = 0;
        while (position < text.size()) {
            auto end = text.find('\n', position);
            if (end == std::string_view::npos) end = text.size();
            std::string line;
            for (const char c : text.substr(position, end - position)) {
                if (c != ' ' && c != '\t' && c != '\r') line.push_back(ascii_lower(c));
            }
            position = end + 1;
            constexpr std::string_view prefix = "attribute";
            constexpr std::string_view suffix = ".vb_usermemid=0";
            if (line.size() > prefix.size() + suffix.size() &&
                line.compare(0, prefix.size(), prefix) == 0 &&
                line.compare(line.size() - suffix.size(), suffix.size(), suffix) == 0) {
                class_def.default_member = line.substr(
                    prefix.size(), line.size() - prefix.size() - suffix.size());
                return;
            }
        }
    }

    [[nodiscard]] bool scan_classes() {
        for (const auto& class_source : class_sources_) {
            std::string lowered_name;
            lowered_name.reserve(class_source.name.size());
            for (const char character : class_source.name) {
                lowered_name.push_back(ascii_lower(character));
            }
            if (lowered_name.empty() || is_reserved_identifier(lowered_name) ||
                class_definitions_.contains(lowered_name)) {
                set_error("WFC0126", "duplicate or reserved class name", offset_);
                return false;
            }
            ClassDef class_def;
            class_def.source = class_source.source;
            class_def.display_name = class_source.name;
            class_definitions_.emplace(std::move(lowered_name), std::move(class_def));
        }
        for (const auto& udt_name : udt_names_) {
            std::string lowered;
            for (const char c : udt_name) lowered.push_back(ascii_lower(c));
            class_definitions_.at(lowered).is_udt = true;
        }
        for (auto& [lowered_name, class_def] : class_definitions_) {
            const auto saved_source = source_;
            const auto saved_offset = offset_;
            source_ = class_def.source;
            offset_ = 0;
            current_class_scan_name_ = lowered_name;
            const bool scanned_ok = scan_class_body(class_def);
            source_ = saved_source;
            offset_ = saved_offset;
            if (!scanned_ok) {
                return false;
            }
            scan_default_member(class_def);
        }
        return true;
    }

    // A lightweight pre-pass, run once before the main top-to-bottom
    // execution begins, that finds every module-level `Sub`/`Function`
    // declaration and registers its signature and body range in
    // `procedures_`. This is required because VB6 procedures are callable
    // from anywhere in the module -- including textually before their own
    // declaration -- unlike ordinary statements, which only take effect
    // when execution reaches them. The scan does not parse procedure
    // bodies at all (only searches for the matching "End Sub"/"End
    // Function" line), so a syntax error inside a procedure that is never
    // called is only discovered if/when that procedure is eventually
    // called (a disclosed scope simplification; see REQ-0202's Scope).
    [[nodiscard]] bool scan_procedures() {
        const auto saved_offset = offset_;
        offset_ = 0;
        while (true) {
            skip_program_leading_trivia();
            if (at_end()) {
                break;
            }
            const auto line_offset = offset_;
            // A leading `Public`/`Private` is accepted (real VB6 source
            // commonly writes one) but not enforced -- REQ-0206 makes
            // Private meaningful for a *class* member's dot-accessibility,
            // but this evaluator has only one standard module, so nothing
            // exists for a module-level Private procedure to be hidden
            // from. If neither the modifier nor a bare Sub/Function follows,
            // this is some other statement; restore and skip the line as
            // scan_procedures already does for anything it doesn't
            // recognize.
            const auto pre_modifier_offset = offset_;
            if (consume_keyword("public") || consume_keyword("private") ||
                consume_keyword("friend")) {
                skip_horizontal_whitespace();
            }
            bool static_procedure = false;
            if (consume_keyword("static")) {
                static_procedure = true;
                skip_horizontal_whitespace();
            }
            bool is_function = false;
            bool is_declare = false;
            if (consume_keyword("declare")) {
                is_declare = true;
                skip_horizontal_whitespace();
                static_cast<void>(consume_keyword("ptrsafe"));
                skip_horizontal_whitespace();
            }
            std::string property_prefix;  // "" for Sub/Function, else "wfclet_" / "wfcset_"
            bool is_property = false;
            if (!is_declare && consume_keyword("property")) {
                skip_horizontal_whitespace();
                is_property = true;
                if (consume_keyword("get")) {
                    is_function = true;
                } else if (consume_keyword("let")) {
                    property_prefix = "wfclet_";
                } else if (consume_keyword("set")) {
                    property_prefix = "wfcset_";
                } else {
                    offset_ = pre_modifier_offset;
                    skip_rest_of_line();
                    continue;
                }
            } else if (consume_keyword("sub")) {
                is_function = false;
            } else if (consume_keyword("function")) {
                is_function = true;
            } else {
                offset_ = pre_modifier_offset;
                skip_rest_of_line();
                continue;
            }
            if (is_declare) {
                // REQ-0266: `Declare Function|Sub Name Lib "dll" ...` has no
                // body; register the name so a call reports error 453.
                skip_horizontal_whitespace();
                char declare_type_character{};
                auto declared = parse_identifier(&declare_type_character);
                if (declared.has_value() && !is_reserved_identifier(*declared) &&
                    !procedures_.contains(*declared)) {
                    ProcedureDef external;
                    external.is_function = is_function;
                    external.is_external = true;
                    external.return_type_index = Value{Integer{}}.index();
                    external.return_is_variant = true;
                    procedures_.emplace(*declared, std::move(external));
                }
                skip_rest_of_line();
                continue;
            }

            skip_horizontal_whitespace();
            const auto name_offset = offset_;
            char type_character{};
            auto name = parse_identifier(&type_character);
            if (!name.has_value() || (type_character != '\0' && !is_function)) {
                offset_ = saved_offset;
                set_error("WFC0118", "expected procedure name", name_offset);
                return false;
            }
            const std::string procedure_key = property_prefix + *name;
            if (is_reserved_identifier(*name) || procedures_.contains(procedure_key)) {
                offset_ = saved_offset;
                set_error(
                    "WFC0119", "duplicate or reserved procedure name", name_offset);
                return false;
            }

            ProcedureDef definition;
            definition.is_function = is_function;
            definition.static_locals = static_procedure;
            if (!scan_procedure_parameters(definition)) {
                offset_ = saved_offset;
                return false;
            }
            if (is_function) {
                skip_horizontal_whitespace();
                if (!consume_keyword("as")) {
                    apply_implicit_return_type(definition, *name, type_character);
                } else {
                const auto type_offset = offset_;
                const auto type_result = parse_scalar_object_or_class_type();
                if (!type_result.has_value()) {
                    offset_ = saved_offset;
                    set_error(
                        "WFC0012",
                        "expected As Integer, As Long, As Double, As Single, As Currency, "
                        "As String, As Boolean, As Object, As Variant, or a known class name",
                        type_offset);
                    return false;
                }
                definition.return_type_index = type_result->type_index;
                definition.return_is_variant = type_result->is_variant;
                definition.return_is_object = type_result->is_object;
                definition.return_class_name = type_result->class_name;
                const auto array_marker =
                    parse_function_array_return_marker(*type_result, type_offset);
                if (!array_marker.has_value()) {
                    offset_ = saved_offset;
                    return false;
                }
                definition.return_is_array = *array_marker;
                }
            }
            if (!consume_loop_header_end()) {
                offset_ = saved_offset;
                return false;
            }
            definition.body_start = offset_;

            if (!skip_to_matching_end(
                    is_property ? "property" : is_function ? "function" : "sub",
                    definition.body_end)) {
                offset_ = saved_offset;
                set_error(
                    is_function ? "WFC0120" : "WFC0121",
                    is_property ? "expected End Property"
                    : is_function ? "expected End Function" : "expected End Sub",
                    line_offset);
                return false;
            }
            definition.declaration_end = offset_;
            procedures_.emplace(procedure_key, std::move(definition));
        }
        offset_ = saved_offset;
        return true;
    }

    [[nodiscard]] bool consume_statement_end() {
        skip_horizontal_whitespace();
        if (pending_next_comma_ && !at_end() && current() == ',') {
            return true;  // `Next j, i`: the enclosing For consumes `, i`.
        }
        if (!at_end() && current() == '\'') {
            skip_comment();
        }
        if (at_end()) {
            return true;
        }
        if (current() == ':') {
            advance();
            return true;
        }
        if (consume_line_break()) {
            return true;
        }
        set_error("WFC0004", "unexpected trailing input", offset_);
        return false;
    }

    // `Case x: statement` -- a colon may end the Case line (REQ-0252).
    [[nodiscard]] bool consume_case_line_end() {
        skip_horizontal_whitespace();
        if (!at_end() && current() == ':') {
            advance();
            return true;
        }
        return consume_block_line_end();
    }

    // A loop header may be followed by `:` and the first body statement on
    // the same line (`For i = 1 To 3: Print i: Next`).
    [[nodiscard]] bool consume_loop_header_end() {
        skip_horizontal_whitespace();
        if (!at_end() && current() == ':') {
            advance();
            return true;
        }
        return consume_block_line_end();
    }

    [[nodiscard]] bool consume_block_line_end() {
        skip_horizontal_whitespace();
        if (!at_end() && current() == '\'') {
            skip_comment();
        }
        if (consume_line_break()) {
            return true;
        }
        set_error("WFC0004", "expected line break", offset_);
        return false;
    }

    [[nodiscard]] bool consume(const char character) noexcept {
        if (at_end() || current() != character) {
            return false;
        }
        advance();
        return true;
    }

    [[nodiscard]] bool consume_keyword(const std::string_view keyword) {
        // Fast reject: most calls are made where some other token starts.
        if (keyword.front() != 'l' || enum_names_.empty()) {
            if (offset_ >= source_.size() || ascii_lower(source_[offset_]) != keyword.front()) {
                return false;
            }
        }
        return consume_keyword_slow(keyword);
    }

    [[nodiscard]] bool consume_keyword_slow(const std::string_view keyword) {
        const auto start = offset_;
        if (!enum_names_.empty() && keyword == "long") {
            // REQ-0237: an Enum's name is a Long-typed type name.
            for (const auto& enum_name : enum_names_) {
                std::size_t i = 0;
                while (i < enum_name.size() && offset_ + i < source_.size() &&
                       ascii_lower(source_[offset_ + i]) == enum_name[i]) {
                    ++i;
                }
                if (i == enum_name.size() &&
                    (offset_ + i == source_.size() || !is_identifier_part(source_[offset_ + i]))) {
                    offset_ += i;
                    return true;
                }
            }
        }
        for (const char expected : keyword) {
            if (at_end() || ascii_lower(current()) != expected) {
                offset_ = start;
                return false;
            }
            advance();
        }

        if (!at_end() && is_identifier_part(current())) {
            offset_ = start;
            return false;
        }
        return true;
    }

    [[nodiscard]] bool at_with_member() const noexcept {
        return !with_names_.empty() && !at_end() && current() == '.' &&
            offset_ + 1 < source_.size() && is_identifier_start(source_[offset_ + 1]);
    }

    [[nodiscard]] std::optional<std::string> parse_identifier(
        char* const type_character = nullptr) {
        // REQ-0236: a leading `.member` inside a With block reads as the
        // With object's hidden variable followed by the member access.
        if (at_with_member()) {
            if (type_character != nullptr) {
                *type_character = '\0';
            }
            return with_names_.back();
        }
        if (at_end() || !is_identifier_start(current())) {
            return std::nullopt;
        }

        std::string identifier;
        do {
            identifier.push_back(ascii_lower(current()));
            advance();
        } while (!at_end() && is_identifier_part(current()));
        // `VBA.Left$(...)`, `Strings.Left`, `Conversion.Int`, ...: the library qualifier is
        // implied; drop it (and a second one: `VBA.Strings.Left`).
        for (int qualifiers = 0; qualifiers < 2; ++qualifiers) {
            static const std::set<std::string, std::less<>> libraries = {
                "vba", "strings", "math", "conversion", "datetime", "interaction", "filesystem",
                "information", "fileio", "financial", "constants", "globals", "vbruntime", "scripting",
                "vbscript_regexp_55", "vbscript_regexp_10"};
            if (!at_end() && current() == '.' && offset_ + 1 < source_.size() &&
                is_identifier_start(source_[offset_ + 1]) && libraries.contains(identifier) &&
                find_variable(identifier).value == nullptr &&
                !(module_names_.contains(identifier)) && !class_definitions_.contains(identifier) &&
                std::find(enum_names_.begin(), enum_names_.end(), identifier) == enum_names_.end()) {
                advance();
                identifier.clear();
                do {
                    identifier.push_back(ascii_lower(current()));
                    advance();
                } while (!at_end() && is_identifier_part(current()));
            } else {
                break;
            }
        }
        // REQ-0258: `Module1.Name` -- drop the module qualifier (standard
        // modules share one namespace), unless a variable shadows it.
        if (!module_names_.empty() && !at_end() && current() == '.' &&
            offset_ + 1 < source_.size() && is_identifier_start(source_[offset_ + 1]) &&
            module_names_.contains(identifier) && find_variable(identifier).value == nullptr) {
            advance();
            identifier.clear();
            do {
                identifier.push_back(ascii_lower(current()));
                advance();
            } while (!at_end() && is_identifier_part(current()));
        }
        if (type_character != nullptr) {
            *type_character = '\0';
            if (!at_end() && strchr("$%&!#@", current()) != nullptr) {
                *type_character = current();
                advance();
            }
        }
        return identifier;
    }

    [[nodiscard]] bool validate_type_character(
        const char type_character,
        const std::size_t identifier_offset) {
        if (type_character == '\0' || type_character == '$' ||
            type_character == '&' || type_character == '#' ||
            type_character == '!' || type_character == '@' ||
            type_character == '%') {
            return true;
        }
        set_error(
            "WFC0097",
            "type-declaration character requires an unsupported value type",
            identifier_offset);
        return false;
    }

    [[nodiscard]] std::size_t type_character_index(const char type_character) const {
        if (type_character == '$') {
            return Value{std::string{}}.index();
        }
        if (type_character == '#') {
            return Value{0.0}.index();
        }
        if (type_character == '!') {
            return Value{0.0f}.index();
        }
        if (type_character == '@') {
            return Value{Currency{}}.index();
        }
        if (type_character == '%') {
            return Value{Int16{}}.index();
        }
        return Value{Integer{}}.index();
    }

    // Recognizes one `As Type` keyword (Integer/Long/Double/Single/
    // Currency/String/Boolean/Variant) for a procedure parameter or
    // function return type, mirroring the type keywords `Dim` already
    // accepts. `Object` and array element types are deliberately not
    // included here (see REQ-0202's Scope).
    struct TypeKeywordResult {
        Value default_value;
        bool is_variant{};
    };
    [[nodiscard]] std::optional<TypeKeywordResult> parse_type_keyword() {
        if (consume_keyword("long")) {
            return TypeKeywordResult{Value{Integer{}}, false};
        }
        if (consume_keyword("integer")) {
            return TypeKeywordResult{Value{Int16{}}, false};
        }
        if (consume_keyword("double")) {
            return TypeKeywordResult{Value{0.0}, false};
        }
        if (consume_keyword("single")) {
            return TypeKeywordResult{Value{0.0f}, false};
        }
        if (consume_keyword("currency")) {
            return TypeKeywordResult{Value{Currency{}}, false};
        }
        if (consume_keyword("date")) {
            return TypeKeywordResult{Value{DateValue{}}, false};
        }
        if (consume_keyword("byte")) {
            return TypeKeywordResult{Value{Byte{}}, false};
        }
        if (consume_keyword("decimal")) {
            return TypeKeywordResult{Value{Decimal{}}, false};
        }
        if (consume_keyword("string")) {
            return TypeKeywordResult{Value{std::string{}}, false};
        }
        if (consume_keyword("boolean")) {
            return TypeKeywordResult{Value{false}, false};
        }
        if (consume_keyword("variant")) {
            return TypeKeywordResult{Value{Empty{}}, true};
        }
        return std::nullopt;
    }

    // A field's or a Function/Property Get return's resolved type (REQ-0205):
    // one of parse_type_keyword's eight scalars, the generic `Object`
    // (`is_object` true, `class_name` empty), or a known class name
    // (`is_object` true, `class_name` set). Exactly one of `is_variant`/
    // `is_object` is ever true, or neither (a plain scalar).
    struct ResolvedType {
        std::size_t type_index{};
        bool is_variant{};
        bool is_object{};
        std::string class_name;
    };

    // Resolves the `Type` keyword sequence immediately following an already-
    // consumed `As` for a class field or a Function/Property Get return
    // type. Returns nullopt without reporting an error when nothing matches
    // (leaving offset_ at the unconsumed type token), so each caller can
    // report its own "expected ..." message listing exactly the forms it
    // accepts.
    [[nodiscard]] std::optional<ResolvedType> parse_scalar_object_or_class_type() {
        skip_horizontal_whitespace();
        if (consume_keyword("object")) {
            return ResolvedType{Value{Nothing{}}.index(), false, true, {}};
        }
        if (const auto type_result = parse_type_keyword()) {
            return ResolvedType{type_result->default_value.index(), type_result->is_variant, false, {}};
        }
        const auto saved_offset = offset_;
        char class_type_character{};
        auto class_name = parse_identifier(&class_type_character);
        if (class_name.has_value() && class_type_character == '\0' &&
            class_definitions_.contains(*class_name)) {
            return ResolvedType{Value{Nothing{}}.index(), false, true, std::move(*class_name)};
        }
        offset_ = saved_offset;
        return std::nullopt;
    }

    // Checks for a trailing `()` immediately after a `Function`'s own
    // return type (REQ-0216: `As Type()` returns an array of `Type`),
    // shared by the module-level and class-method Function return-type
    // parsing sites (not `Property Get`, which does not support an array
    // return type). Returns `false` with `offset_` unchanged when no `(`
    // follows (an ordinary scalar return type); returns `nullopt` (a
    // reported error) for a `Variant`/`Object`/class-typed `type_result`,
    // or a malformed `(...)` -- an array return type must be a fixed
    // scalar, matching an array-typed parameter's own restriction
    // (REQ-0211).
    [[nodiscard]] std::optional<bool> parse_function_array_return_marker(
        const ResolvedType& type_result, const std::size_t type_offset) {
        skip_horizontal_whitespace();
        if (at_end() || current() != '(') {
            return false;
        }
        if (type_result.is_variant || type_result.is_object) {
            set_error(
                "WFC0150", "an array return type must be a fixed scalar type", type_offset);
            return std::nullopt;
        }
        advance();
        skip_horizontal_whitespace();
        if (!consume(')')) {
            set_error("WFC0150", "expected closing parenthesis", offset_);
            return std::nullopt;
        }
        return true;
    }

    [[nodiscard]] bool type_character_matches(
        const Value& value,
        const char type_character,
        const std::size_t identifier_offset) {
        if (type_character == '\0') {
            return true;
        }
        if (!validate_type_character(type_character, identifier_offset)) {
            return false;
        }
        if (value.index() == type_character_index(type_character)) {
            return true;
        }
        set_error("WFC0016", "identifier type-declaration character mismatch", identifier_offset);
        return false;
    }

    // An object reference used where a value is expected stands for its class's
    // default member (`Attribute Name.VB_UserMemId = 0`): `s = obj`, `"x" & obj`,
    // `Print obj`. Replaces `value` with that member's result; leaves it unchanged
    // (returning true) when it is not an instance of a class with a default
    // member. Returns false only after the default member itself failed.
    [[nodiscard]] bool resolve_default_value(Value& value, const std::size_t offset) {
        const auto* holder = std::get_if<ObjectInstance>(&value);
        if (holder == nullptr || !execute_ || holder->data == nullptr) {
            return true;
        }
        const auto class_iterator = class_definitions_.find(holder->data->class_name);
        if (class_iterator == class_definitions_.end() || class_iterator->second.is_udt ||
            class_iterator->second.default_member.empty()) {
            return true;
        }
        const auto& class_def = class_iterator->second;
        const ProcedureDef* definition = nullptr;
        if (const auto method = class_def.methods.find(class_def.default_member);
            method != class_def.methods.end()) {
            definition = &method->second;
        } else if (const auto getter = class_def.property_get.find(class_def.default_member);
                   getter != class_def.property_get.end()) {
            definition = &getter->second;
        }
        if (definition == nullptr) {
            return true;
        }
        const auto instance = holder->data;  // keep the object alive across the call
        auto result = invoke_definition(
            *definition, class_def.default_member, {}, offset, class_def.source, instance.get());
        if (!result.has_value()) {
            return false;
        }
        value = std::move(*result);
        return true;
    }

    // Attempt Integer/Long/Single/Double/Currency widening or checked
    // narrowing so `value` matches `target_index`. Leaves `value` unchanged, and returns
    // true, when no numeric conversion applies (including when it already
    // matches) -- the caller still compares `value->index()` against
    // `target_index` afterward, since a non-numeric mismatch (e.g. a String
    // assigned to a Double target) is not this function's concern. Returns
    // false only after reporting WFC0009 for a narrowing conversion whose
    // result does not fit the target type.
    // REQ-0270: VB6's implicit scalar assignment conversions -- numeric <->
    // numeric (round half to even, overflow is error 6), Boolean <-> number,
    // numeric String -> number (non-numeric is error 13), number/Boolean/Date
    // -> String, Empty -> the target's zero. Leaves `value` untouched when the
    // pair is not a scalar conversion (the caller reports the mismatch).
    [[nodiscard]] bool implicit_scalar_conversion(
        Value& value, const std::size_t target_index, const std::size_t offset) {
        const std::size_t object_index = Value{ObjectInstance{}}.index();
        if (value.index() == object_index && target_index != object_index &&
            !resolve_default_value(value, offset)) {
            return false;
        }
        const std::size_t long_index = Value{Integer{}}.index();
        const std::size_t string_index = Value{std::string{}}.index();
        const std::size_t bool_index = Value{false}.index();
        const bool target_numeric = target_index == long_index ||
            target_index == Value{Int16{}}.index() || target_index == Value{Byte{}}.index() ||
            target_index == Value{0.0}.index() || target_index == Value{0.0f}.index() ||
            target_index == Value{Currency{}}.index() || target_index == Value{Decimal{}}.index();
        const bool target_scalar = target_numeric || target_index == string_index ||
            target_index == bool_index || target_index == Value{DateValue{}}.index();
        if (!target_scalar) {
            return true;
        }
        const bool source_scalar = is_number(value) || std::holds_alternative<bool>(value) ||
            std::holds_alternative<std::string>(value) || std::holds_alternative<Empty>(value) ||
            std::holds_alternative<DateValue>(value) || std::holds_alternative<Null>(value);
        if (!source_scalar) {
            return true;
        }
        if (std::holds_alternative<Null>(value)) {
            if (execute_) {
                set_error("WFC0104", "Invalid use of Null", offset);
                return false;
            }
            value = zero_value_for_index(target_index);
            return true;
        }
        if (std::holds_alternative<Empty>(value)) {
            value = zero_value_for_index(target_index);
            return true;
        }
        if (!execute_) {
            // Dry run: the operand is a placeholder; only the type matters.
            if (target_index == bool_index || target_index == string_index || target_numeric) {
                value = zero_value_for_index(target_index);
            }
            return true;
        }
        const auto mismatch = [&]() {
            set_error("WFC0016", "assignment type mismatch", offset);
            return false;
        };
        // To String.
        if (target_index == string_index) {
            if (std::holds_alternative<std::string>(value)) {
                return true;
            }
            value = render(value);
            return true;
        }
        // From String.
        if (const auto* text = std::get_if<std::string>(&value)) {
            if (target_index == string_index) {
                return true;
            }
            if (target_index == bool_index) {
                std::string lowered;
                for (const char c : *text) lowered.push_back(ascii_lower(c));
                if (lowered == "true") { value = true; return true; }
                if (lowered == "false") { value = false; return true; }
            }
            if (target_index == Value{DateValue{}}.index()) {
                return true;  // handled by the Date branch below
            }
            const auto parsed = parse_numeric_string(*text);
            if (parsed.status == NumericStringStatus::out_of_range) {
                set_error("WFC0009", "numeric overflow", offset);
                return false;
            }
            if (parsed.status != NumericStringStatus::valid) {
                return mismatch();
            }
            value = parsed.value;
            if (target_index == bool_index) {
                value = parsed.value != 0.0;
                return true;
            }
            return coerce_numeric_value(value, target_index, offset);
        }
        // From Boolean.
        if (const auto* flag = std::get_if<bool>(&value)) {
            if (target_index == bool_index) {
                return true;
            }
            value = Integer{*flag ? -1 : 0};
            return coerce_numeric_value(value, target_index, offset);
        }
        // To Boolean from a number.
        if (target_index == bool_index) {
            if (std::holds_alternative<DateValue>(value)) {
                value = std::get<DateValue>(value).serial != 0.0;
            } else if (std::holds_alternative<Decimal>(value)) {
                value = as_double(value) != 0.0;
            } else {
                value = as_double(value) != 0.0;
            }
            return true;
        }
        // To Long from the other numerics (round half to even).
        if (target_index == long_index && !std::holds_alternative<Integer>(value)) {
            double number{};
            if (const auto* date = std::get_if<DateValue>(&value)) {
                number = date->serial;
            } else if (std::holds_alternative<Decimal>(value)) {
                number = decimal_to_double(std::get<Decimal>(value));
            } else {
                number = as_double(value);
            }
            const double rounded = std::nearbyint(number);
            if (!(rounded >= -2147483648.0 && rounded <= 2147483647.0)) {
                set_error("WFC0009", "integer overflow", offset);
                return false;
            }
            value = static_cast<Integer>(rounded);
            return true;
        }
        return true;
    }

    [[nodiscard]] bool coerce_numeric_value(
        Value& value,
        const std::size_t target_index,
        const std::size_t offset) {
        if (value.index() == target_index) {
            return true;
        }
        if (!implicit_scalar_conversion(value, target_index, offset)) {
            return false;
        }
        if (value.index() == target_index) {
            return true;
        }
        // REQ-0254: Decimal target from any exact or floating numeric.
        if (target_index == Value{Decimal{}}.index()) {
            if (!is_number(value)) {
                return true;  // caller reports the mismatch
            }
            Decimal result;
            if (const auto* integer = std::get_if<Integer>(&value)) {
                result.negative = *integer < 0;
                result.mantissa = big_from_u32(static_cast<std::uint32_t>(
                    *integer < 0 ? -static_cast<std::int64_t>(*integer) : *integer));
            } else if (const auto* short_integer = std::get_if<Int16>(&value)) {
                result.negative = *short_integer < 0;
                result.mantissa = big_from_u32(static_cast<std::uint32_t>(
                    *short_integer < 0 ? -static_cast<std::int32_t>(*short_integer)
                                       : *short_integer));
            } else if (const auto* byte = std::get_if<Byte>(&value)) {
                result.mantissa = big_from_u32(static_cast<std::uint32_t>(*byte));
            } else if (const auto* currency = std::get_if<Currency>(&value)) {
                result.negative = currency->scaled < 0;
                const auto magnitude = currency->scaled < 0
                    ? (~static_cast<std::uint64_t>(currency->scaled) + 1ULL)
                    : static_cast<std::uint64_t>(currency->scaled);
                result.mantissa.limb[0] = static_cast<std::uint32_t>(magnitude);
                result.mantissa.limb[1] = static_cast<std::uint32_t>(magnitude >> 32U);
                result.scale = 4U;
            } else {
                const auto converted = decimal_from_double(as_double(value));
                if (!converted.has_value()) {
                    set_error("WFC0009", "numeric overflow", offset);
                    return false;
                }
                result = *converted;
            }
            value = result;
            return true;
        }
        // REQ-0247: Byte target (range-checked) and Byte source (widens).
        if (target_index == Value{Byte{}}.index()) {
            double source_value{};
            if (std::holds_alternative<Decimal>(value) || !is_number(value)) {
                return true;  // caller reports the type mismatch
            }
            source_value = std::nearbyint(as_double(value));
            if (!(source_value >= 0.0 && source_value <= 255.0)) {
                set_error("WFC0009", "integer overflow", offset);
                return false;
            }
            value = static_cast<Byte>(source_value);
            return true;
        }
        if (const auto* byte_source = std::get_if<Byte>(&value)) {
            value = static_cast<Integer>(*byte_source);
            if (target_index == Value{Integer{}}.index()) {
                return true;
            }
            return coerce_numeric_value(value, target_index, offset);
        }
        // REQ-0242: Date <-> numeric/String implicit conversions.
        if (target_index == Value{DateValue{}}.index()) {
            if (is_number(value) && !std::holds_alternative<Decimal>(value)) {
                value = DateValue{as_double(value)};
            } else if (const auto* text = std::get_if<std::string>(&value)) {
                if (const auto parsed = parse_date_text(*text)) {
                    value = DateValue{*parsed};
                }
            }
            return true;
        }
        if (const auto* date = std::get_if<DateValue>(&value)) {
            if (target_index == Value{std::string{}}.index()) {
                value = render_date(date->serial);
                return true;
            }
            if (target_index != Value{Empty{}}.index() && target_index != Value{Null{}}.index() &&
                target_index != Value{Nothing{}}.index() && target_index != Value{false}.index()) {
                value = date->serial;
                return coerce_numeric_value(value, target_index, offset);
            }
            return true;
        }
        if (target_index == Value{0.0}.index()) {
            if (const auto* integer = std::get_if<Integer>(&value)) {
                value = static_cast<double>(*integer);
            } else if (const auto* short_integer = std::get_if<Int16>(&value)) {
                value = static_cast<double>(*short_integer);
            } else if (const auto* single = std::get_if<float>(&value)) {
                value = static_cast<double>(*single);
            } else if (std::holds_alternative<Currency>(value)) {
                value = as_double(value);
            }
            return true;
        }
        if (target_index == Value{0.0f}.index()) {
            if (const auto* integer = std::get_if<Integer>(&value)) {
                value = static_cast<float>(*integer);
                return true;
            }
            if (const auto* short_integer = std::get_if<Int16>(&value)) {
                value = static_cast<float>(*short_integer);
                return true;
            }
            if (const auto* number = std::get_if<double>(&value)) {
                const auto narrowed = static_cast<float>(*number);
                if (!std::isfinite(narrowed)) {
                    set_error("WFC0009", "numeric overflow", offset);
                    return false;
                }
                value = narrowed;
                return true;
            }
            if (std::holds_alternative<Currency>(value)) {
                const auto narrowed = static_cast<float>(as_double(value));
                if (!std::isfinite(narrowed)) {
                    set_error("WFC0009", "numeric overflow", offset);
                    return false;
                }
                value = narrowed;
                return true;
            }
            return true;
        }
        if (target_index == Value{Currency{}}.index()) {
            if (const auto* integer = std::get_if<Integer>(&value)) {
                value = Currency{static_cast<std::int64_t>(*integer) * 10000};
                return true;
            }
            if (const auto* short_integer = std::get_if<Int16>(&value)) {
                value = Currency{static_cast<std::int64_t>(*short_integer) * 10000};
                return true;
            }
            if (const auto* number = std::get_if<double>(&value)) {
                const auto scaled = currency_from_double(*number);
                if (!scaled.has_value()) {
                    set_error("WFC0009", "numeric overflow", offset);
                    return false;
                }
                value = Currency{*scaled};
                return true;
            }
            if (const auto* single = std::get_if<float>(&value)) {
                const auto scaled = currency_from_double(static_cast<double>(*single));
                if (!scaled.has_value()) {
                    set_error("WFC0009", "numeric overflow", offset);
                    return false;
                }
                value = Currency{*scaled};
                return true;
            }
            return true;
        }
        if (target_index == Value{Int16{}}.index()) {
            // Widening from Long, or narrowing from Double/Single/Currency
            // with a checked range (VB6 Integer is -32768 through 32767); an
            // already-integral Long source needs a plain range check, not
            // rounding.
            if (const auto* integer = std::get_if<Integer>(&value)) {
                if (*integer < std::numeric_limits<Int16>::min() ||
                    *integer > std::numeric_limits<Int16>::max()) {
                    set_error("WFC0009", "integer overflow", offset);
                    return false;
                }
                value = static_cast<Int16>(*integer);
                return true;
            }
            if (const auto* number = std::get_if<double>(&value)) {
                const double rounded = std::nearbyint(*number);
                if (!(rounded >= static_cast<double>(std::numeric_limits<Int16>::min()) &&
                      rounded <= static_cast<double>(std::numeric_limits<Int16>::max()))) {
                    set_error("WFC0009", "integer overflow", offset);
                    return false;
                }
                value = static_cast<Int16>(rounded);
                return true;
            }
            if (const auto* single = std::get_if<float>(&value)) {
                const double rounded = std::nearbyint(static_cast<double>(*single));
                if (!(rounded >= static_cast<double>(std::numeric_limits<Int16>::min()) &&
                      rounded <= static_cast<double>(std::numeric_limits<Int16>::max()))) {
                    set_error("WFC0009", "integer overflow", offset);
                    return false;
                }
                value = static_cast<Int16>(rounded);
                return true;
            }
            if (std::holds_alternative<Currency>(value)) {
                const double rounded = std::nearbyint(as_double(value));
                if (!(rounded >= static_cast<double>(std::numeric_limits<Int16>::min()) &&
                      rounded <= static_cast<double>(std::numeric_limits<Int16>::max()))) {
                    set_error("WFC0009", "integer overflow", offset);
                    return false;
                }
                value = static_cast<Int16>(rounded);
                return true;
            }
            return true;
        }
        return true;
    }

    [[nodiscard]] static std::string vb_error_description(const Integer number) {
        switch (number) {
        case 0: return {};
        case 3: return "Return without GoSub";
        case 5: return "Invalid procedure call or argument";
        case 6: return "Overflow";
        case 7: return "Out of memory";
        case 9: return "Subscript out of range";
        case 10: return "This array is fixed or temporarily locked";
        case 11: return "Division by zero";
        case 13: return "Type mismatch";
        case 14: return "Out of string space";
        case 16: return "Expression too complex";
        case 17: return "Can't perform requested operation";
        case 18: return "User interrupt occurred";
        case 20: return "Resume without error";
        case 28: return "Out of stack space";
        case 35: return "Sub, Function, or Property not defined";
        case 47: return "Too many DLL application clients";
        case 48: return "Error in loading DLL";
        case 49: return "Bad DLL calling convention";
        case 51: return "Internal error";
        case 52: return "Bad file name or number";
        case 53: return "File not found";
        case 54: return "Bad file mode";
        case 55: return "File already open";
        case 57: return "Device I/O error";
        case 58: return "File already exists";
        case 59: return "Bad record length";
        case 61: return "Disk full";
        case 62: return "Input past end of file";
        case 63: return "Bad record number";
        case 67: return "Too many files";
        case 68: return "Device unavailable";
        case 70: return "Permission denied";
        case 71: return "Disk not ready";
        case 74: return "Can't rename with different drive";
        case 75: return "Path/File access error";
        case 76: return "Path not found";
        case 91: return "Object variable or With block variable not set";
        case 92: return "For loop not initialized";
        case 93: return "Invalid pattern string";
        case 94: return "Invalid use of Null";
        case 321: return "Invalid file format";
        case 322: return "Can't create necessary temporary file";
        case 325: return "Invalid format in resource file";
        case 380: return "Invalid property value";
        case 381: return "Invalid property array index";
        case 382: return "Set not supported at runtime";
        case 383: return "Set not supported (read-only property)";
        case 385: return "Need property array index";
        case 387: return "Set not permitted";
        case 393: return "Get not supported at runtime";
        case 394: return "Get not supported (write-only property)";
        case 422: return "Property not found";
        case 423: return "Property or method not found";
        case 424: return "Object required";
        case 429: return "ActiveX component can't create object";
        case 430: return "Class does not support Automation or does not support expected interface";
        case 432: return "File name or class name not found during Automation operation";
        case 438: return "Object doesn't support this property or method";
        case 440: return "Automation error";
        case 443: return "Automation object does not have a default value";
        case 445: return "Object doesn't support this action";
        case 446: return "Object doesn't support named arguments";
        case 447: return "Object doesn't support current locale setting";
        case 448: return "Named argument not found";
        case 449: return "Argument not optional";
        case 450: return "Wrong number of arguments or invalid property assignment";
        case 451: return "Property let procedure not defined and property get procedure did not return an object";
        case 452: return "Invalid ordinal";
        case 453: return "Specified DLL function not found";
        case 454: return "Code resource not found";
        case 455: return "Code resource lock error";
        case 457: return "This key is already associated with an element of this collection";
        case 458: return "Variable uses an Automation type not supported in Visual Basic";
        case 459: return "Object or class does not support the set of events";
        case 460: return "Invalid clipboard format";
        case 461: return "Method or data member not found";
        case 462: return "The remote server machine does not exist or is unavailable";
        case 463: return "Class not registered on local machine";
        case 481: return "Invalid picture";
        case 482: return "Printer error";
        case 483: return "Printer driver does not support specified property";
        case 485: return "Invalid picture type";
        case 486: return "Can't print form image to this type of printer";
        case 520: return "Can't empty Clipboard";
        case 521: return "Can't open Clipboard";
        case 735: return "Can't save file to TEMP";
        case 744: return "Search text not found";
        case 746: return "Replacements too long";
        case 31001: return "Out of memory";
        default: return "Application-defined or object-defined error";
        }
    }

    // The VB error number a failed statement's diagnostic stands for, or 0
    // when it is not a catchable runtime error (a syntax/semantic error).
    [[nodiscard]] Integer runtime_error_number() const {
        const std::string_view code = std::string_view(error_.diagnostic).substr(0, 7);
        if (code == "WFC0300") return err_number_;
        if (code == "WFC0016" || code == "WFC0007" || code == "WFC0018" || code == "WFC0019" ||
            code == "WFC0020" || code == "WFC0073" || code == "WFC0086" || code == "WFC0087" ||
            code == "WFC0088" || code == "WFC0095" || code == "WFC0098" || code == "WFC0103" ||
            code == "WFC0105" || code == "WFC0060" || code == "WFC0053") {
            return 13;  // Type mismatch
        }
        if (code == "WFC0136" || code == "WFC0107") return 424;  // Object required
        if (code == "WFC0008") return 11;
        if (code == "WFC0009") return 6;
        if (code == "WFC0111") return 9;
        if (code == "WFC0106") return 91;
        if (code == "WFC0104") return 94;
        if (code == "WFC0089" || code == "WFC0101" || code == "WFC0075" || code == "WFC0078" ||
            code == "WFC0076" || code == "WFC0077" || code == "WFC0079" ||
            code == "WFC0080" || code == "WFC0082" || code == "WFC0083" ||
            code == "WFC0091" || code == "WFC0092" || code == "WFC0094" ||
            code == "WFC0096") {
            return 5;
        }
        if (code == "WFC0148" || code == "WFC0151") return 9;
        if (code == "WFC0123") return 28;
        return 0;
    }

    // Moves offset_ to the end of the current statement (a ':' or line
    // break outside a string literal), without consuming it.
    void skip_to_statement_end() noexcept {
        bool in_string = false;
        while (!at_end()) {
            const char c = current();
            if (c == '"') {
                in_string = !in_string;
            } else if (!in_string && (c == ':' || c == '\r' || c == '\n')) {
                return;
            } else if (!in_string && c == '\'') {
                skip_comment();
                return;
            }
            advance();
        }
    }

    // REQ-0238: applies the frame's `On Error` mode to a statement that just
    // failed with a runtime error. Returns true when the error was absorbed
    // (Resume Next) or converted into a pending jump to the handler label.
    [[nodiscard]] bool recover_runtime_error(const std::size_t statement_start) {
        const Integer number = runtime_error_number();
        if (number == 0) {
            return false;
        }
        Scope& frame = scopes_.back();
        if (frame.on_error_mode == 0 || frame.in_error_handler) {
            return false;
        }
        if (std::string_view(error_.diagnostic).substr(0, 7) != "WFC0300") {
            err_number_ = number;
            err_source_.clear();
            err_description_ = vb_error_description(number);
        }
        skip_to_statement_end();
        frame.error_resume_next = offset_;
        frame.error_retry = statement_start;
        error_ = wfc::Evaluation{};
        if (frame.on_error_mode == 1) {
            execute_ = true;
            return true;
        }
        frame.in_error_handler = true;
        const bool in_procedure_context =
            in_procedure_body() && current_procedure_def_ != nullptr && frame.handler_depth == 0;
        if (!in_procedure_context) {
            jump_pending_ = true;
            jump_target_ = frame.on_error_label;
            set_error("WFC0999", "internal jump", statement_start);
            return false;
        }
        // Run the handler right here, in the failing statement's context, so
        // `Resume Next` / `Resume` continue inside any enclosing loop.
        const auto resume_point = offset_;
        const auto end_limit = current_procedure_def_->body_end;
        offset_ = frame.on_error_label;
        ++frame.handler_depth;
        frame.resume_signal = 0;
        bool completed = true;
        while (true) {
            skip_program_leading_trivia();
            if (offset_ >= end_limit || at_end()) {
                exit_sub_requested_ = true;
                exit_function_requested_ = true;
                offset_ = resume_point;
                break;
            }
            if (!parse_statement() || !consume_statement_end()) {
                completed = false;
                break;
            }
            if (frame.resume_signal != 0 || exit_sub_requested_ || exit_function_requested_) {
                offset_ = resume_point;
                break;
            }
        }
        --frame.handler_depth;
        if (!completed) {
            return false;
        }
        if (frame.resume_signal == 2) {
            retry_statement_ = true;
        }
        frame.resume_signal = 0;
        execute_ = true;
        return true;
    }

    [[nodiscard]] bool parse_statement() {
        while (true) {
            const auto start = offset_;
            if (parse_statement_once()) {
                if (!retry_statement_) return true;
                retry_statement_ = false;
                offset_ = start;
                continue;
            }
            return false;
        }
    }

    [[nodiscard]] bool parse_statement_once() {
        const auto start = offset_;
        const bool entry_execute = execute_;
        if (parse_statement_core()) {
            return true;
        }
        if (!entry_execute || jump_pending_) {
            return false;
        }
        // Block statements (If/For/While/Do/Select/With) are not recovered
        // as a whole: their nested statements recover individually.
        {
            const auto saved = offset_;
            offset_ = start;
            skip_horizontal_whitespace();
            const bool is_block = consume_keyword("if") || consume_keyword("for") ||
                consume_keyword("while") || consume_keyword("do") ||
                consume_keyword("select") || consume_keyword("with");
            offset_ = saved;
            if (is_block) {
                return false;
            }
        }
        execute_ = entry_execute;
        return recover_runtime_error(start);
    }

    // After a statement sequence failed with a pending jump, transfers
    // control to the jump target. Returns false when no jump is pending.
    [[nodiscard]] bool take_pending_jump() {
        if (!jump_pending_) {
            return false;
        }
        jump_pending_ = false;
        error_ = wfc::Evaluation{};
        offset_ = jump_target_;
        execute_ = true;
        exit_sub_requested_ = false;
        exit_function_requested_ = false;
        return true;
    }

    // Offset of the `label:` line inside the current procedure (or the
    // whole program at module level), or npos.
    [[nodiscard]] std::size_t find_label(const std::string& label) const {
        std::size_t begin = 0;
        std::size_t end = source_.size();
        if (in_procedure_body() && current_procedure_def_ != nullptr) {
            begin = current_procedure_def_->body_start;
            end = std::min(end, current_procedure_def_->body_end);
        }
        std::size_t position = begin;
        while (position < end) {
            std::size_t line_end = source_.find('\n', position);
            if (line_end == std::string_view::npos || line_end > end) {
                line_end = end;
            }
            std::size_t i = position;
            while (i < line_end && (source_[i] == ' ' || source_[i] == '\t')) ++i;
            if (!label.empty() && std::isdigit(static_cast<unsigned char>(label[0])) != 0) {
                std::size_t k = i;
                while (k < line_end && std::isdigit(static_cast<unsigned char>(source_[k])) != 0) ++k;
                if (k > i && source_.substr(i, k - i) == label) return i;
                position = line_end + 1;
                continue;
            }
            std::size_t j = 0;
            while (j < label.size() && i + j < line_end &&
                   ascii_lower(source_[i + j]) == label[j]) {
                ++j;
            }
            if (j == label.size() && i + j < line_end && source_[i + j] == ':' &&
                (i + j + 1 >= line_end || source_[i + j + 1] != '=')) {
                return i;
            }
            position = line_end + 1;
        }
        return std::string::npos;
    }

    // A label operand: an identifier or a line number.
    [[nodiscard]] std::optional<std::string> parse_label_name() {
        skip_horizontal_whitespace();
        if (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0) {
            std::string digits;
            while (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0) {
                digits.push_back(current());
                advance();
            }
            return digits;
        }
        return parse_identifier();
    }

    [[nodiscard]] bool in_procedure_body() const noexcept { return scopes_.size() > 1U; }

    // `On Error ...`, `Resume ...`, `GoTo label`, `label:` and `Err.Raise`/
    // `Err.Clear` (REQ-0238). Returns nullopt when the statement is none of
    // these (offset_ unchanged).
    [[nodiscard]] std::optional<bool> parse_error_handling_statement(
        const std::size_t statement_offset, const bool allow_label = true) {
        const auto start = offset_;
        if (consume_keyword("end") || consume_keyword("stop")) {
            skip_horizontal_whitespace();
            if (!at_statement_end()) {
                offset_ = start;
                return std::nullopt;
            }
            if (execute_) {
                end_requested_ = true;
                set_error("WFC0998", "program ended", statement_offset);
                return false;
            }
            return true;
        }
        if (consume_keyword("error")) {
            skip_horizontal_whitespace();
            if (at_statement_end()) {
                offset_ = start;
                return std::nullopt;
            }
            const auto number_offset = offset_;
            auto number = parse_expression();
            if (!number.has_value()) {
                return false;
            }
            if (!coerce_numeric_value(*number, Value{Integer{}}.index(), number_offset) ||
                !std::holds_alternative<Integer>(*number)) {
                set_error("WFC0073", "Error requires a Long number", number_offset);
                return false;
            }
            if (!execute_) {
                return true;
            }
            const Integer raised = std::get<Integer>(*number);
            if (raised < 1 || raised > 65535) {
                return raise_runtime(5, "Invalid procedure call or argument", statement_offset);
            }
            return raise_runtime(raised, vb_error_description(raised), statement_offset);
        }
        if (consume_keyword("lset") || consume_keyword("rset")) {
            const bool right = ascii_lower(source_[start + 0]) == 'r';
            skip_horizontal_whitespace();
            const auto variable_offset = offset_;
            auto name = parse_identifier();
            if (!name.has_value()) {
                set_error("WFC0011", "expected variable name", variable_offset);
                return false;
            }
            const auto variable = find_variable(*name);
            if (variable.value == nullptr) {
                set_error("WFC0015", "undeclared variable", variable_offset);
                return false;
            }
            skip_horizontal_whitespace();
            if (!consume('=')) {
                set_error("WFC0014", "expected assignment operator", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            auto value = parse_expression();
            if (!value.has_value()) {
                return false;
            }
            if (!execute_) {
                return true;
            }
            auto* const target = std::get_if<std::string>(variable.value);
            const auto* text = std::get_if<std::string>(&*value);
            if (target == nullptr || text == nullptr) {
                set_error("WFC0016", "LSet/RSet require String operands", variable_offset);
                return false;
            }
            const std::size_t width = utf16_length(*target);
            std::string aligned = *text;
            if (!is_ascii_text(aligned)) {
                auto wide = to_utf16_units(aligned);
                wide.resize(std::min(wide.size(), width));
                aligned = from_utf16_units(wide);
            } else {
                aligned.resize(std::min(aligned.size(), width));
            }
            const std::size_t used = utf16_length(aligned);
            if (used < width) {
                const std::string padding(width - used, ' ');
                aligned = right ? padding + aligned : aligned + padding;
            }
            *target = std::move(aligned);
            return true;
        }
        if (consume_keyword("return")) {
            skip_horizontal_whitespace();
            if (!at_statement_end()) {
                offset_ = start;
                return std::nullopt;
            }
            if (!execute_) {
                return true;
            }
            Scope& frame = scopes_.back();
            if (frame.gosub_stack.empty()) {
                return raise_runtime(3, "Return without GoSub", statement_offset);
            }
            jump_pending_ = true;
            jump_target_ = frame.gosub_stack.back();
            frame.gosub_stack.pop_back();
            set_error("WFC0999", "internal jump", statement_offset);
            return false;
        }
        if (consume_keyword("gosub")) {
            skip_horizontal_whitespace();
            const auto label_offset = offset_;
            auto label = parse_label_name();
            if (!label.has_value()) {
                set_error("WFC0011", "expected label after GoSub", label_offset);
                return false;
            }
            if (!execute_) {
                return true;
            }
            const auto target = find_label(*label);
            if (target == std::string::npos) {
                set_error("WFC0301", "label not defined", label_offset);
                return false;
            }
            skip_to_statement_end();
            scopes_.back().gosub_stack.push_back(offset_);
            jump_pending_ = true;
            jump_target_ = target;
            set_error("WFC0999", "internal jump", statement_offset);
            return false;
        }
        if (consume_keyword("on")) {
            skip_horizontal_whitespace();
            if (!consume_keyword("error")) {
                // `On expr GoTo|GoSub label1, label2, ...`
                auto selector = parse_expression();
                if (!selector.has_value()) {
                    return false;
                }
                skip_horizontal_whitespace();
                const bool is_gosub = consume_keyword("gosub");
                if (!is_gosub && !consume_keyword("goto")) {
                    set_error("WFC0010", "expected GoTo or GoSub", offset_);
                    return false;
                }
                if (!coerce_numeric_value(*selector, Value{Integer{}}.index(), statement_offset) ||
                    !std::holds_alternative<Integer>(*selector)) {
                    set_error("WFC0073", "On ... GoTo selector must be numeric", statement_offset);
                    return false;
                }
                const Integer choice = std::get<Integer>(*selector);
                std::vector<std::pair<std::string, std::size_t>> labels;
                while (true) {
                    skip_horizontal_whitespace();
                    const auto label_offset = offset_;
                    auto label = parse_label_name();
                    if (!label.has_value()) {
                        set_error("WFC0011", "expected label", label_offset);
                        return false;
                    }
                    labels.emplace_back(std::move(*label), label_offset);
                    skip_horizontal_whitespace();
                    if (!consume(',')) {
                        break;
                    }
                }
                if (!execute_) {
                    return true;
                }
                if (choice < 0 || choice > 255) {
                    return raise_runtime(5, "Invalid procedure call or argument", statement_offset);
                }
                if (choice == 0 || static_cast<std::size_t>(choice) > labels.size()) {
                    return true;
                }
                const auto& chosen = labels[static_cast<std::size_t>(choice) - 1U];
                const auto target = find_label(chosen.first);
                if (target == std::string::npos) {
                    set_error("WFC0301", "label not defined", chosen.second);
                    return false;
                }
                if (is_gosub) {
                    skip_to_statement_end();
                    scopes_.back().gosub_stack.push_back(offset_);
                }
                jump_pending_ = true;
                jump_target_ = target;
                set_error("WFC0999", "internal jump", statement_offset);
                return false;
            }
            skip_horizontal_whitespace();
            Scope& frame = scopes_.back();
            if (consume_keyword("resume")) {
                skip_horizontal_whitespace();
                if (!consume_keyword("next")) {
                    set_error("WFC0010", "expected Next after On Error Resume", offset_);
                    return false;
                }
                if (execute_) {
                    frame.on_error_mode = 1;
                    frame.in_error_handler = false;
                }
                return true;
            }
            if (!consume_keyword("goto")) {
                set_error("WFC0010", "expected GoTo or Resume after On Error", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            if (!at_end() && (current() == '0' || current() == '-')) {
                skip_to_statement_end();
                if (execute_) {
                    frame.on_error_mode = 0;
                    frame.in_error_handler = false;
                }
                return true;
            }
            const auto label_offset = offset_;
            auto label = parse_label_name();
            if (!label.has_value()) {
                set_error("WFC0011", "expected label after GoTo", label_offset);
                return false;
            }
            if (execute_) {
                const auto target = find_label(*label);
                if (target == std::string::npos) {
                    set_error("WFC0301", "label not defined", label_offset);
                    return false;
                }
                frame.on_error_mode = 2;
                frame.on_error_label = target;
                frame.in_error_handler = false;
            }
            return true;
        }
        if (consume_keyword("resume")) {
            skip_horizontal_whitespace();
            Scope& frame = scopes_.back();
            std::size_t target{};
            if (consume_keyword("next")) {
                target = frame.error_resume_next;
            } else if (at_end() || current() == '\r' || current() == '\n' || current() == ':' ||
                       current() == '\'') {
                target = frame.error_retry;
            } else {
                const auto label_offset = offset_;
                auto label = parse_label_name();
                if (!label.has_value()) {
                    set_error("WFC0011", "expected label after Resume", label_offset);
                    return false;
                }
                target = execute_ ? find_label(*label) : 0;
                if (execute_ && target == std::string::npos) {
                    set_error("WFC0301", "label not defined", label_offset);
                    return false;
                }
            }
            if (execute_) {
                if (!frame.in_error_handler) {
                    return raise_runtime(20, "Resume without error", statement_offset);
                }
                frame.in_error_handler = false;
                if (frame.handler_depth > 0 &&
                    (target == frame.error_resume_next || target == frame.error_retry)) {
                    frame.resume_signal = target == frame.error_retry &&
                            target != frame.error_resume_next ? 2 : 1;
                    skip_to_statement_end();
                    return true;
                }
                jump_pending_ = true;
                jump_target_ = target;
                set_error("WFC0999", "internal jump", statement_offset);
                return false;
            }
            return true;
        }
        if (consume_keyword("goto")) {
            skip_horizontal_whitespace();
            const auto label_offset = offset_;
            auto label = parse_label_name();
            if (!label.has_value()) {
                set_error("WFC0011", "expected label after GoTo", label_offset);
                return false;
            }
            if (execute_) {
                const auto target = find_label(*label);
                if (target == std::string::npos) {
                    set_error("WFC0301", "label not defined", label_offset);
                    return false;
                }
                jump_pending_ = true;
                jump_target_ = target;
                set_error("WFC0999", "internal jump", statement_offset);
                return false;
            }
            return true;
        }
        if (consume_keyword("err")) {
            skip_horizontal_whitespace();
            if (!consume('.')) {
                offset_ = start;
                return std::nullopt;
            }
            const auto member_offset = offset_;
            auto member = parse_identifier();
            if (member == "clear") {
                if (execute_) {
                    err_number_ = 0;
                    err_description_.clear();
                    err_source_.clear();
                }
                return true;
            }
            if (member == "raise") {
                skip_horizontal_whitespace();
                const bool parenthesized = consume('(');
                skip_horizontal_whitespace();
                auto number = parse_expression();
                if (!number.has_value()) {
                    return false;
                }
                if (!coerce_numeric_value(*number, Value{Integer{}}.index(), member_offset) ||
                    !std::holds_alternative<Integer>(*number)) {
                    set_error("WFC0073", "Err.Raise requires a Long number", member_offset);
                    return false;
                }
                std::string description;
                std::string source_text;
                bool has_description = false;
                for (int argument = 0; argument < 4; ++argument) {
                    skip_horizontal_whitespace();
                    if (!consume(',')) {
                        break;
                    }
                    skip_horizontal_whitespace();
                    if (!at_end() && (current() == ',' || current() == ')')) {
                        continue;  // omitted argument
                    }
                    auto value = parse_expression();
                    if (!value.has_value()) {
                        return false;
                    }
                    if (argument == 0) {
                        if (const auto* text = std::get_if<std::string>(&*value)) {
                            source_text = *text;
                        }
                    }
                    if (argument == 1) {
                        if (const auto* text = std::get_if<std::string>(&*value)) {
                            description = *text;
                            has_description = true;
                        }
                    }
                }
                if (parenthesized) {
                    skip_horizontal_whitespace();
                    if (!consume(')')) {
                        set_error("WFC0005", "expected closing parenthesis", offset_);
                        return false;
                    }
                }
                if (execute_) {
                    const Integer raised = std::get<Integer>(*number);
                    if (raised == 0) {
                        return raise_runtime(5, "Invalid procedure call or argument", member_offset);
                    }
                    err_number_ = raised;
                    err_source_ = source_text;
                    err_description_ = has_description ? description : vb_error_description(raised);
                    set_error("WFC0300", err_description_, statement_offset);
                    return false;
                }
                return true;
            }
            offset_ = start;
            return std::nullopt;
        }
        // `label:` (an identifier immediately followed by ':' that is not `:=`).
        {
            auto label = parse_identifier();
            if (allow_label && label.has_value() && !at_end() && current() == ':' &&
                !(offset_ + 1 < source_.size() && source_[offset_ + 1] == '=') &&
                !is_reserved_identifier(*label) && *label != "else") {
                return true;  // the ':' is left for the statement separator
            }
            offset_ = start;
        }
        return std::nullopt;
    }

    [[nodiscard]] bool parse_statement_core() {
        variant_operand_seen_ = false;
        variant_string_seen_ = false;
        variant_number_seen_ = false;
        skip_horizontal_whitespace();
        if (!at_end() && current() == ':') {
            return true;  // an empty statement (`a = 1 : : b = 2`)
        }
        // A leading line number (`10  x = 1`) is a label that also feeds Erl.
        if (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0) {
            std::size_t line_start = offset_;
            while (line_start > 0 && (source_[line_start - 1] == ' ' || source_[line_start - 1] == '\t')) {
                --line_start;
            }
            if (line_start == 0 || source_[line_start - 1] == '\n') {
                Integer number{};
                while (!at_end() && std::isdigit(static_cast<unsigned char>(current())) != 0) {
                    number = static_cast<Integer>(
                        std::min<long long>(number * 10LL + (current() - '0'), 2147483647LL));
                    advance();
                }
                if (execute_) erl_ = number;
                if (!at_end() && current() == ':' &&
                    !(offset_ + 1 < source_.size() && source_[offset_ + 1] == '=')) {
                    advance();
                }
                skip_horizontal_whitespace();
                if (at_statement_end()) return true;
            }
        }
        const auto statement_offset = offset_;
        // A statement that once fell through every keyword test is an
        // assignment or call; skip straight to that tail next time (the loop
        // bodies re-parse the same text over and over).
        if (identifier_statements_.contains(source_.data() + statement_offset)) {
            return parse_identifier_statement(statement_offset);
        }
        if (const auto handled = parse_error_handling_statement(statement_offset)) {
            return *handled;
        }
        if (const auto handled = parse_file_statement(statement_offset)) {
            return *handled;
        }
        if (const auto handled = parse_mid_statement(statement_offset)) {
            return *handled;
        }
        if (const auto handled = parse_binary_statement(statement_offset)) {
            return *handled;
        }
        if (consume_keyword("doevents")) {
            return true;
        }
        {
            // DefXxx statements were applied by scan_deftypes.
            const auto line_end = source_.find_first_of("\r\n", offset_);
            const auto length =
                (line_end == std::string_view::npos ? source_.size() : line_end) - statement_offset;
            if (apply_deftype_line(source_.substr(statement_offset, length))) {
                offset_ = statement_offset + length;
                return true;
            }
        }
        if (consume_keyword("beep")) {
            return true;
        }
        {
            const auto before_call = offset_;
            if (consume_keyword("callbyname")) {
                skip_horizontal_whitespace();
                const bool parenthesized = !at_end() && current() == '(';
                if (parenthesized) advance();
                std::vector<Value> values;
                skip_horizontal_whitespace();
                while (!at_statement_end() && !(parenthesized && current() == ')')) {
                    auto value = parse_expression();
                    if (!value.has_value()) {
                        return false;
                    }
                    values.push_back(std::move(*value));
                    skip_horizontal_whitespace();
                    if (!consume(',')) break;
                    skip_horizontal_whitespace();
                }
                if (parenthesized && !consume(')')) {
                    set_error("WFC0005", "expected closing parenthesis", offset_);
                    return false;
                }
                skip_horizontal_whitespace();
                if (!at_statement_end()) {
                    offset_ = before_call;  // `CallByName(...).Member` etc. is not a statement
                    set_error("WFC0010", "expected statement", statement_offset);
                    return false;
                }
                return evaluate_misc_function("callbyname", values, statement_offset).has_value();
            }
        }
        if (consume_keyword("msgbox") || consume_keyword("appactivate") ||
            consume_keyword("sendkeys")) {
            // The statement form evaluates and ignores its arguments.
            skip_horizontal_whitespace();
            while (!at_statement_end()) {
                if (!parse_expression().has_value()) {
                    return false;
                }
                skip_horizontal_whitespace();
                if (!consume(',')) break;
                skip_horizontal_whitespace();
            }
            return true;
        }
        if (const auto shell_start = offset_; consume_keyword("shell")) {
            skip_horizontal_whitespace();
            if (at_end() || current() == '=' || current() == '.' || at_statement_end()) {
                offset_ = shell_start;
            } else {
                std::vector<Value> values;
                while (!at_statement_end()) {
                    auto value = parse_expression();
                    if (!value.has_value()) return false;
                    values.push_back(std::move(*value));
                    skip_horizontal_whitespace();
                    if (!consume(',')) break;
                    skip_horizontal_whitespace();
                }
                if (!execute_) return true;
                return evaluate_misc_function("shell", values, statement_offset).has_value();
            }
        }
        if (consume_keyword("savesetting") || consume_keyword("deletesetting") ||
            consume_keyword("setattr") || consume_keyword("chdrive")) {
            const std::string_view word = source_.substr(statement_offset, 4);
            const bool is_save = ascii_lower(word[0]) == 's' && ascii_lower(word[1]) == 'a';
            std::vector<Value> values;
            skip_horizontal_whitespace();
            while (!at_statement_end()) {
                auto value = parse_expression();
                if (!value.has_value()) {
                    return false;
                }
                values.push_back(std::move(*value));
                skip_horizontal_whitespace();
                if (!consume(',')) break;
                skip_horizontal_whitespace();
            }
            if (execute_ && ascii_lower(word[0]) == 's' && ascii_lower(word[1]) == 'e') {
                // SetAttr path, attributes: only the read-only bit has an effect.
                const auto* path = values.empty() ? nullptr : std::get_if<std::string>(&values[0]);
                const auto attributes = values.size() < 2U ? std::nullopt : whole_value(values[1]);
                if (path == nullptr || !attributes.has_value()) {
                    set_error("WFC0073", "SetAttr requires a path and attributes", statement_offset);
                    return false;
                }
                std::error_code ec;
                if (!std::filesystem::exists(*path, ec)) {
                    return raise_runtime(53, "File not found", statement_offset);
                }
                std::filesystem::permissions(
                    *path,
                    std::filesystem::perms::owner_write | std::filesystem::perms::group_write |
                        std::filesystem::perms::others_write,
                    (*attributes & 1) != 0 ? std::filesystem::perm_options::remove
                                           : std::filesystem::perm_options::add,
                    ec);
                return true;
            }
            if (execute_ && values.size() >= 3U) {
                std::string key;
                bool strings = true;
                for (std::size_t i = 0; i < 3U; ++i) {
                    const auto* part = std::get_if<std::string>(&values[i]);
                    strings = strings && part != nullptr;
                    if (part != nullptr) key += *part + "\x01";
                }
                if (strings && is_save && values.size() == 4U) {
                    if (const auto* text = std::get_if<std::string>(&values[3])) {
                        settings_[key] = *text;
                    }
                } else if (strings && ascii_lower(word[0]) == 'd') {
                    settings_.erase(key);
                }
            }
            return true;
        }
        {
            // File width / record locking: accepted, no effect.
            const auto before_lock = offset_;
            if (consume_keyword("width") || consume_keyword("lock") || consume_keyword("unlock")) {
                skip_horizontal_whitespace();
                if (!at_end() && current() != '=' && current() != '(' && current() != '.') {
                    skip_to_statement_end();
                    return true;
                }
                offset_ = before_lock;
            }
        }
        if (consume_keyword("reset")) {
            if (execute_) {
                for (auto& [number, file] : files_) std::fclose(file.handle);
                files_.clear();
            }
            return true;
        }
        {
            const auto before_declare = offset_;
            static_cast<void>(consume_keyword("public") || consume_keyword("private"));
            skip_horizontal_whitespace();
            if (consume_keyword("declare")) {
                skip_comment();  // handled by scan_procedures
                return true;
            }
            offset_ = before_declare;
        }
        if (consume_keyword("attribute")) {
            skip_comment();  // `Attribute X.VB_... = ...` lines inside bodies
            return true;
        }
        {
            const auto before_debug = offset_;
            if (consume_keyword("debug")) {
                skip_horizontal_whitespace();
                if (consume('.')) {
                    skip_horizontal_whitespace();
                    if (consume_keyword("print")) {
                        // Immediate-window output: collected in
                        // `Evaluation::debug_output`, not in `output`.
                        const bool saved_discard = discard_print_;
                        discard_print_ = true;
                        const bool ok = parse_print_statement();
                        discard_print_ = saved_discard;
                        return ok;
                    }
                    if (consume_keyword("assert")) {
                        // Stripped from compiled programs: the condition is not evaluated.
                        skip_comment_free_statement_text();
                        return true;
                    }
                }
                offset_ = before_debug;
            }
        }
        if (consume_keyword("option")) {
            return parse_option_statement(statement_offset);
        }
        if (consume_keyword("rem")) {
            skip_comment();
            return true;
        }
        if (allow_declarations_) {
            module_body_started_ = true;
        }
        if (consume_keyword("if")) {
            return parse_if_statement();
        }
        if (consume_keyword("while")) {
            return parse_while_statement();
        }
        if (consume_keyword("do")) {
            return parse_do_statement();
        }
        if (consume_keyword("for")) {
            skip_horizontal_whitespace();
            if (consume_keyword("each")) {
                return parse_for_each_statement();
            }
            return parse_for_statement();
        }
        if (consume_keyword("select")) {
            return parse_select_statement();
        }
        if (consume_keyword("with")) {
            return parse_with_statement(statement_offset);
        }
        {
            const auto pre_enum_offset = offset_;
            static_cast<void>(consume_keyword("public") || consume_keyword("private"));
            skip_horizontal_whitespace();
            if (consume_keyword("enum")) {
                return parse_enum_statement(statement_offset);
            }
            if (consume_keyword("type")) {
                return parse_type_statement_skip(statement_offset);
            }
            offset_ = pre_enum_offset;
        }
        if (consume_keyword("case")) {
            set_error("WFC0058", "unexpected Case", statement_offset);
            return false;
        }
        if (consume_keyword("next")) {
            set_error("WFC0048", "unexpected Next", statement_offset);
            return false;
        }
        if (consume_keyword("exit")) {
            return parse_exit_statement(statement_offset);
        }
        if (consume_keyword("loop")) {
            set_error("WFC0038", "unexpected Loop", statement_offset);
            return false;
        }
        if (consume_keyword("wend")) {
            set_error("WFC0033", "unexpected Wend", statement_offset);
            return false;
        }
        if (consume_keyword("sub") || consume_keyword("function")) {
            return parse_procedure_declaration_skip(statement_offset);
        }
        if (consume_keyword("property")) {
            return parse_property_declaration_skip(statement_offset);
        }
        {
            // REQ-0248: module-level `Public|Private|Global [Const] name ...`
            // declares a module variable/constant (visibility is not
            // enforced: this evaluator has a single standard module).
            const auto pre_visibility_offset = offset_;
            if (consume_keyword("public") || consume_keyword("private") ||
                consume_keyword("global")) {
                skip_horizontal_whitespace();
                if (consume_keyword("const")) {
                    return parse_constant_declaration();
                }
                if (!at_end() && is_identifier_start(current())) {
                    const auto probe = offset_;
                    char probe_type_character{};
                    const auto word = parse_identifier(&probe_type_character);
                    offset_ = probe;
                    if (word.has_value() && *word != "sub" && *word != "function" &&
                        *word != "property" && *word != "declare" && *word != "static" &&
                        *word != "enum" && *word != "type" && *word != "event" &&
                        *word != "sub" && procedures_.find(*word) == procedures_.end()) {
                        if (!allow_declarations_) {
                            set_error(
                                "WFC0027",
                                "declarations are not supported in conditional blocks",
                                statement_offset);
                            return false;
                        }
                        return parse_declaration();
                    }
                }
                offset_ = pre_visibility_offset;
            }
        }
        {
            // `Public`/`Private Sub|Function Name(...)` -- the modifier is
            // parsed (matching real VB6 source) but not enforced at module
            // level (see scan_procedures); restore position if what
            // follows isn't actually a procedure declaration; consumed
            // here so `Public`/`Private` (both are reserved keywords) never
            // exists as a fully-unrecognized standalone statement.
            const auto pre_modifier_offset = offset_;
            const bool had_modifier = consume_keyword("public") || consume_keyword("private") ||
                                      consume_keyword("friend");
            skip_horizontal_whitespace();
            const bool had_static = consume_keyword("static");
            if (had_modifier || had_static) {
                skip_horizontal_whitespace();
                if (consume_keyword("sub") || consume_keyword("function")) {
                    return parse_procedure_declaration_skip(statement_offset);
                }
                if (consume_keyword("property")) {
                    return parse_property_declaration_skip(statement_offset);
                }
                offset_ = pre_modifier_offset;
            }
        }
        if (consume_keyword("call")) {
            return parse_call_statement();
        }
        if (consume_keyword("raiseevent")) {
            return parse_raise_event_statement(statement_offset);
        }
        if (consume_keyword("print")) {
            return parse_print_statement();
        }
        if (consume_keyword("randomize")) {
            return parse_randomize_statement(statement_offset);
        }
        if (const auto rnd_start = offset_ - 0U; consume_keyword("rnd")) {
            // `Rnd -1` as a statement: the call's result is discarded.
            skip_horizontal_whitespace();
            if (at_end() || current() == '(' || current() == '=') {
                offset_ = rnd_start;
            } else {
                double argument = 1.0;
                const bool has_argument = current() != ':' && current() != '\r' &&
                                          current() != '\n' && current() != '\'';
                if (has_argument) {
                    auto value = parse_expression();
                    if (!value.has_value()) return false;
                    if (!is_number(*value) && !std::holds_alternative<bool>(*value)) {
                        if (!execute_) return true;
                        set_error("WFC0073", "Rnd requires a numeric argument", statement_offset);
                        return false;
                    }
                    argument = std::holds_alternative<bool>(*value)
                                   ? (std::get<bool>(*value) ? -1.0 : 0.0)
                                   : as_double(*value);
                }
                if (execute_ && !(has_argument && argument == 0.0)) {
                    rnd_state_ = (has_argument && argument < 0.0) ? seed_from_number(argument)
                                                                  : rnd_step(rnd_state_);
                    rnd_last_value_ = static_cast<float>(rnd_value(rnd_state_));
                }
                return true;
            }
        }
        // REQ-0271: Dim/Static/Const are legal inside blocks; a declaration
        // executed again (loop iteration) is a no-op.
        if (consume_keyword("dim")) {
            if (current_procedure_def_ != nullptr && current_procedure_def_->static_locals) {
                return parse_static_declaration(statement_offset);
            }
            return parse_declaration();
        }
        if (consume_keyword("static")) {
            return parse_static_declaration(statement_offset);
        }
        if (consume_keyword("const")) {
            return parse_constant_declaration();
        }
        if (consume_keyword("redim")) {
            // Unlike Dim/Static/Const, ReDim is an executable statement (it
            // resizes an already-declared dynamic array), not a
            // declaration, so it is not gated by allow_declarations_ --
            // real VB6 permits it inside a conditional block.
            return parse_redim_statement();
        }
        if (consume_keyword("erase")) {
            return parse_erase_statement();
        }
        if (consume_keyword("set")) {
            return parse_set_statement();
        }

        offset_ = statement_offset;
        identifier_statements_.insert(source_.data() + statement_offset);
        return parse_identifier_statement(statement_offset);
    }

    // `[Let] name ...`: an assignment, array/member write, or a bare call.
    [[nodiscard]] bool parse_identifier_statement(const std::size_t statement_offset) {
        offset_ = statement_offset;
        const bool has_let = consume_keyword("let");
        if (has_let) {
            skip_horizontal_whitespace();
        }
        const auto identifier_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value()) {
            set_error("WFC0010", "expected statement", statement_offset);
            return false;
        }
        // A bare `Name` statement (no `Call`, zero arguments; REQ-0217)
        // invokes a module-level or class-sibling Sub/Function, discarding
        // any Function result -- checked only when `Name` is not a
        // variable and nothing else follows it on this statement (no `=`,
        // no `(`, no argument list), so this never competes with an
        // ordinary assignment or array-element write. A bare `Name arg1,
        // arg2` (with arguments) remains unsupported, avoiding the classic
        // ambiguity that form has with other statement shapes; see
        // REQ-0217's Scope.
        if (const auto handled = parse_bare_call(*identifier, identifier_offset, type_character, has_let)) {
            return *handled;
        }
        return parse_assignment_or_array_element(std::move(*identifier), type_character);
    }

    // ---- File I/O (REQ-0245) ---------------------------------------------

    struct OpenFile {
        std::FILE* handle{};
        int mode{};  // 1 = Input, 2 = Output, 3 = Append, 4 = Binary, 5 = Random
        long record_length{128};
    };

    [[nodiscard]] static std::FILE* open_file(const std::string& path, const char* mode) {
#ifdef _WIN32
        std::FILE* handle = nullptr;
        return fopen_s(&handle, path.c_str(), mode) == 0 ? handle : nullptr;
#else
        return std::fopen(path.c_str(), mode);
#endif
    }

    [[nodiscard]] static std::string environment_variable(const std::string& name) {
#ifdef _WIN32
        char* buffer = nullptr;
        std::size_t length = 0;
        std::string result;
        if (_dupenv_s(&buffer, &length, name.c_str()) == 0 && buffer != nullptr) {
            result = buffer;
            std::free(buffer);
        }
        return result;
#else
        const char* found = std::getenv(name.c_str());
        return found != nullptr ? found : "";
#endif
    }

    [[nodiscard]] bool raise_runtime(
        const Integer number, const std::string& description, const std::size_t offset) {
        err_number_ = number;
        err_description_ = description;
        err_source_.clear();
        set_error("WFC0300", description, offset);
        return false;
    }

    [[nodiscard]] OpenFile* find_open_file(const Integer number, const std::size_t offset) {
        const auto found = files_.find(number);
        if (found == files_.end()) {
            static_cast<void>(raise_runtime(52, "Bad file name or number", offset));
            return nullptr;
        }
        return &found->second;
    }

    [[nodiscard]] bool write_to_file(
        const Integer number, const std::string& text, const std::size_t offset) {
        auto* const file = find_open_file(number, offset);
        if (file == nullptr) {
            return false;
        }
        if (file->mode == 1) {
            return raise_runtime(54, "Bad file mode", offset);
        }
        const std::string bytes = text_to_ansi_bytes(text);
        if (!bytes.empty() && std::fwrite(bytes.data(), 1, bytes.size(), file->handle) != bytes.size()) {
            return raise_runtime(57, "Device I/O error", offset);
        }
        return true;
    }

    // Reads one line (without the terminator) into `line`; false at EOF.
    [[nodiscard]] static bool read_file_line(std::FILE* handle, std::string& line) {
        line.clear();
        int c = std::fgetc(handle);
        if (c == EOF) {
            return false;
        }
        while (c != EOF && c != '\n') {
            if (c == '\r') {
                const int next = std::fgetc(handle);
                if (next != '\n' && next != EOF) {
                    std::ungetc(next, handle);
                }
                break;
            }
            line.push_back(static_cast<char>(c));
            c = std::fgetc(handle);
        }
        return true;
    }

    [[nodiscard]] static bool file_at_eof(std::FILE* handle) {
        const int c = std::fgetc(handle);
        if (c == EOF) {
            return true;
        }
        std::ungetc(c, handle);
        return false;
    }

    // Reads one `Input #` field: a quoted string or a run up to ',' / newline.
    [[nodiscard]] static bool read_input_field(std::FILE* handle, std::string& token, bool& quoted) {
        token.clear();
        quoted = false;
        int c = std::fgetc(handle);
        while (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            c = std::fgetc(handle);
        }
        if (c == EOF) {
            return false;
        }
        if (c == '"') {
            quoted = true;
            c = std::fgetc(handle);
            while (c != EOF && c != '"') {
                token.push_back(static_cast<char>(c));
                c = std::fgetc(handle);
            }
            c = std::fgetc(handle);
            while (c == ' ' || c == '\t') c = std::fgetc(handle);
            if (c != ',' && c != '\n' && c != '\r' && c != EOF) std::ungetc(c, handle);
            return true;
        }
        while (c != EOF && c != ',' && c != '\n' && c != '\r') {
            token.push_back(static_cast<char>(c));
            c = std::fgetc(handle);
        }
        if (c == '\r') {
            const int next = std::fgetc(handle);
            if (next != '\n' && next != EOF) std::ungetc(next, handle);
        }
        while (!token.empty() && (token.back() == ' ' || token.back() == '\t')) token.pop_back();
        return true;
    }

    [[nodiscard]] bool store_input_token(
        Value& target, const bool is_variant, const std::string& token, const bool quoted,
        const std::size_t offset) {
        if (is_variant) {
            if (!quoted && token.size() >= 2 && token.front() == '#' && token.back() == '#') {
                std::string inner = token.substr(1, token.size() - 2);
                std::string lowered;
                for (const char ch : inner) lowered.push_back(ascii_lower(ch));
                if (lowered == "true" || lowered == "false") {
                    target = lowered == "true";
                    return true;
                }
                if (lowered == "null") {
                    target = Null{};
                    return true;
                }
                if (const auto parsed = parse_date_text(inner)) {
                    target = DateValue{*parsed};
                    return true;
                }
            }
            if (!quoted) {
                const auto number = parse_numeric_string(token);
                if (number.status == NumericStringStatus::valid) {
                    const bool whole = std::floor(number.value) == number.value;
                    if (whole && number.value >= -32768.0 && number.value <= 32767.0) {
                        target = Value{static_cast<Int16>(number.value)};
                    } else if (whole && std::fabs(number.value) < 2147483648.0) {
                        target = Value{static_cast<Integer>(number.value)};
                    } else {
                        target = Value{number.value};
                    }
                    return true;
                }
            }
            target = token;
            return true;
        }
        if (std::holds_alternative<std::string>(target)) {
            target = token;
            return true;
        }
        if (std::holds_alternative<bool>(target)) {
            std::string lowered;
            for (const char ch : token) lowered.push_back(ascii_lower(ch));
            target = lowered == "#true#" || lowered == "true";
            return true;
        }
        if (std::holds_alternative<DateValue>(target)) {
            std::string inner = token;
            if (inner.size() >= 2 && inner.front() == '#' && inner.back() == '#') {
                inner = inner.substr(1, inner.size() - 2);
            }
            const auto parsed = parse_date_text(inner);
            target = DateValue{parsed.value_or(0.0)};
            return true;
        }
        const auto number = parse_numeric_string(token.empty() ? "0" : token);
        if (number.status != NumericStringStatus::valid) {
            return raise_runtime(13, "Type mismatch", offset);
        }
        Value converted{number.value};
        if (std::holds_alternative<Integer>(target)) {
            const double rounded = std::nearbyint(number.value);
            if (!(rounded >= -2147483648.0 && rounded <= 2147483647.0)) {
                return raise_runtime(6, "Overflow", offset);
            }
            converted = static_cast<Integer>(rounded);
        } else if (!coerce_numeric_value(converted, target.index(), offset)) {
            return false;
        }
        if (converted.index() != target.index()) {
            return raise_runtime(13, "Type mismatch", offset);
        }
        target = std::move(converted);
        return true;
    }

    [[nodiscard]] std::string write_item_text(const Value& value) {
        if (const auto* text = std::get_if<std::string>(&value)) {
            return "\"" + *text + "\"";
        }
        if (const auto* flag = std::get_if<bool>(&value)) {
            return *flag ? "#TRUE#" : "#FALSE#";
        }
        if (std::holds_alternative<Null>(value)) {
            return "#NULL#";
        }
        if (std::holds_alternative<Empty>(value)) {
            return "";
        }
        if (const auto* date = std::get_if<DateValue>(&value)) {
            const auto parts = split_date(date->serial);
            char buffer[64];
            const bool has_time = parts.hour != 0 || parts.minute != 0 || parts.second != 0;
            const bool has_date = std::floor(date->serial) != 0.0 || !has_time;
            std::string text = "#";
            if (has_date) {
                std::snprintf(
                    buffer, sizeof(buffer), "%04lld-%02lld-%02lld",
                    static_cast<long long>(parts.year), static_cast<long long>(parts.month),
                    static_cast<long long>(parts.day));
                text += buffer;
            }
            if (has_time) {
                std::snprintf(
                    buffer, sizeof(buffer), "%02lld:%02lld:%02lld",
                    static_cast<long long>(parts.hour), static_cast<long long>(parts.minute),
                    static_cast<long long>(parts.second));
                text += (has_date ? " " : "") + std::string(buffer);
            }
            return text + "#";
        }
        return render(value);
    }

    [[nodiscard]] bool parse_hash_file_number(Integer& number) {
        skip_horizontal_whitespace();
        static_cast<void>(consume('#'));
        skip_horizontal_whitespace();
        const auto offset = offset_;
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        if (!coerce_numeric_value(*value, Value{Integer{}}.index(), offset) ||
            !std::holds_alternative<Integer>(*value)) {
            set_error("WFC0073", "file number must be a Long", offset);
            return false;
        }
        number = std::get<Integer>(*value);
        return true;
    }

    // `Mid[$](var, start[, length]) = expression` (REQ-0246): overwrites part
    // of a String variable in place, never changing its length.
    [[nodiscard]] std::optional<bool> parse_mid_statement(const std::size_t statement_offset) {
        const auto start = offset_;
        if (!consume_keyword("mid")) {
            return std::nullopt;
        }
        static_cast<void>(consume('$'));
        skip_horizontal_whitespace();
        if (!consume('(')) {
            offset_ = start;
            return std::nullopt;
        }
        skip_horizontal_whitespace();
        const auto variable_offset = offset_;
        char type_character{};
        auto name = parse_identifier(&type_character);
        if (!name.has_value()) {
            offset_ = start;
            return std::nullopt;
        }
        const auto variable = find_variable(*name);
        if (variable.value == nullptr) {
            set_error("WFC0015", "undeclared variable", variable_offset);
            return false;
        }
        skip_horizontal_whitespace();
        const auto long_argument = [&](Integer& out) -> bool {
            skip_horizontal_whitespace();
            const auto argument_offset = offset_;
            auto value = parse_expression();
            if (!value.has_value()) {
                return false;
            }
            if (!coerce_numeric_value(*value, Value{Integer{}}.index(), argument_offset) ||
                !std::holds_alternative<Integer>(*value)) {
                set_error("WFC0073", "Mid requires Long arguments", argument_offset);
                return false;
            }
            out = std::get<Integer>(*value);
            return true;
        };
        Integer position{};
        Integer length = -1;
        if (!consume(',') || !long_argument(position)) {
            if (error_.diagnostic.empty()) {
                set_error("WFC0014", "expected start in Mid statement", offset_);
            }
            return false;
        }
        skip_horizontal_whitespace();
        if (consume(',') && !long_argument(length)) {
            return false;
        }
        skip_horizontal_whitespace();
        if (!consume(')')) {
            set_error("WFC0005", "expected closing parenthesis", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        if (!consume('=')) {
            set_error("WFC0014", "expected assignment operator", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        auto replacement = parse_expression();
        if (!replacement.has_value()) {
            return false;
        }
        if (!execute_) {
            return true;
        }
        auto* const target = std::get_if<std::string>(variable.value);
        const auto* text = std::get_if<std::string>(&*replacement);
        if (target == nullptr || text == nullptr) {
            set_error("WFC0016", "Mid statement requires String operands", statement_offset);
            return false;
        }
        if (position < 1 || length < -1) {
            return raise_runtime(5, "Invalid procedure call or argument", statement_offset);
        }
        const auto begin = static_cast<std::size_t>(position - 1);
        if (!is_ascii_text(*target) || !is_ascii_text(*text)) {
            auto target_units = to_utf16_units(*target);
            const auto text_units = to_utf16_units(*text);
            if (begin >= target_units.size()) {
                return true;
            }
            std::size_t unit_count = std::min(text_units.size(), target_units.size() - begin);
            if (length >= 0) {
                unit_count = std::min(unit_count, static_cast<std::size_t>(length));
            }
            target_units.replace(begin, unit_count, text_units, 0, unit_count);
            *target = from_utf16_units(target_units);
            return true;
        }
        if (begin >= target->size()) {
            return true;
        }
        std::size_t count = std::min(text->size(), target->size() - begin);
        if (length >= 0) {
            count = std::min(count, static_cast<std::size_t>(length));
        }
        target->replace(begin, count, *text, 0, count);
        return true;
    }

    // Positions a Binary/Random file for `Get`/`Put`/`Seek` (1-based `position`).
    static void seek_record(OpenFile& file, const long position) {
        const long unit = file.mode == 5 ? file.record_length : 1;
        std::fseek(file.handle, (position - 1) * unit, SEEK_SET);
    }

    // `Get|Put [#]n, [position], variable` and `Seek [#]n, position`.
    struct LValue {
        Value* ptr{};
        std::size_t fixed{};
    };

    // `name`, `name(i, ...)`, `.field` chains: a storage location for Get.
    [[nodiscard]] bool parse_lvalue_path(LValue& result) {
        const auto variable_offset = offset_;
        char type_character{};
        auto name = parse_identifier(&type_character);
        if (!name.has_value()) {
            set_error("WFC0011", "expected variable name", variable_offset);
            return false;
        }
        const auto variable = find_variable(*name);
        if (variable.value == nullptr) {
            set_error("WFC0015", "undeclared variable", variable_offset);
            return false;
        }
        result.ptr = variable.value;
        if (const auto fixed = variable.scope->fixed_string_lengths.find(*name);
            fixed != variable.scope->fixed_string_lengths.end()) {
            result.fixed = fixed->second;
        }
        while (true) {
            skip_horizontal_whitespace();
            if (at_end()) return true;
            if (current() == '(' && result.ptr != nullptr &&
                std::holds_alternative<ArrayValue>(*result.ptr)) {
                auto& array = std::get<ArrayValue>(*result.ptr);
                advance();
                auto indices = parse_index_list(array_expected_dimension_count(array));
                if (!indices.has_value()) return false;
                if (execute_) {
                    const auto flat_offset = array_flat_offset(array, *indices);
                    if (!flat_offset.has_value()) return false;
                    result.ptr = &array.elements[*flat_offset];
                    result.fixed = 0;
                } else {
                    result.ptr = nullptr;
                }
                continue;
            }
            if (current() == '.') {
                advance();
                skip_horizontal_whitespace();
                char field_type_character{};
                const auto field_offset = offset_;
                auto field_name = parse_identifier(&field_type_character);
                if (!field_name.has_value()) {
                    set_error("WFC0011", "expected member name after '.'", field_offset);
                    return false;
                }
                if (!execute_ || result.ptr == nullptr) {
                    result.ptr = nullptr;
                    continue;
                }
                auto* instance = std::get_if<ObjectInstance>(result.ptr);
                if (instance == nullptr) {
                    set_error("WFC0136", "member access requires an object reference", field_offset);
                    return false;
                }
                const auto field = instance->data->fields.variables.find(*field_name);
                if (field == instance->data->fields.variables.end()) {
                    set_error("WFC0135", "unknown member", field_offset);
                    return false;
                }
                result.ptr = &field->second;
                const auto fixed = instance->data->fields.fixed_string_lengths.find(*field_name);
                result.fixed = fixed != instance->data->fields.fixed_string_lengths.end()
                    ? fixed->second : 0U;
                continue;
            }
            return true;
        }
    }

    [[nodiscard]] std::optional<bool> parse_binary_statement(const std::size_t statement_offset) {
        const auto start = offset_;
        const bool is_get = consume_keyword("get");
        const bool is_put = !is_get && consume_keyword("put");
        const bool is_seek = !is_get && !is_put && consume_keyword("seek");
        if (!is_get && !is_put && !is_seek) {
            return std::nullopt;
        }
        skip_horizontal_whitespace();
        if (!at_end() && (current() == '=' || current() == '(' || current() == '.')) {
            offset_ = start;  // a variable named Get/Put/Seek
            return std::nullopt;
        }
        Integer number{};
        if (!parse_hash_file_number(number)) {
            return false;
        }
        skip_horizontal_whitespace();
        if (!consume(',')) {
            set_error("WFC0014", "expected comma after file number", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        std::optional<long> position;
        if (is_seek || (!at_end() && current() != ',')) {
            const auto position_offset = offset_;
            auto value = parse_expression();
            if (!value.has_value()) {
                return false;
            }
            if (!coerce_numeric_value(*value, Value{Integer{}}.index(), position_offset) ||
                !std::holds_alternative<Integer>(*value)) {
                set_error("WFC0073", "file position must be a Long", position_offset);
                return false;
            }
            position = static_cast<long>(std::get<Integer>(*value));
        }
        if (is_seek) {
            if (!execute_) return true;
            auto* const file = find_open_file(number, statement_offset);
            if (file == nullptr) return false;
            if (*position < 1) {
                return raise_runtime(63, "Bad record number", statement_offset);
            }
            seek_record(*file, *position);
            return true;
        }
        skip_horizontal_whitespace();
        if (!consume(',')) {
            set_error("WFC0014", "expected comma before variable", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        LValue lvalue;
        if (!parse_lvalue_path(lvalue)) {
            return false;
        }
        if (!execute_) {
            return true;
        }
        auto* const file = find_open_file(number, statement_offset);
        if (file == nullptr) return false;
        if (file->mode < 4) {
            return raise_runtime(54, "Bad file mode", statement_offset);
        }
        if (position.has_value()) {
            if (*position < 1) {
                return raise_runtime(63, "Bad record number", statement_offset);
            }
            seek_record(*file, *position);
        } else {
            std::fseek(file->handle, std::ftell(file->handle), SEEK_SET);
        }
        const long record_start = std::ftell(file->handle);
        const auto transfer = [&](void* data, const std::size_t size) {
            return is_get ? std::fread(data, 1, size, file->handle) == size
                          : std::fwrite(data, 1, size, file->handle) == size;
        };
        const std::function<bool(Value&, std::size_t, bool)> transfer_value =
            [&](Value& target, const std::size_t fixed, const bool nested) -> bool {
            bool ok = true;
            if (auto* v1 = std::get_if<Integer>(&target)) {
                std::int32_t x = *v1; ok = transfer(&x, 4); if (is_get) *v1 = x;
            } else if (auto* v2 = std::get_if<Int16>(&target)) {
                std::int16_t x = *v2; ok = transfer(&x, 2); if (is_get) *v2 = x;
            } else if (auto* v3 = std::get_if<Byte>(&target)) {
                std::uint8_t x = *v3; ok = transfer(&x, 1); if (is_get) *v3 = x;
            } else if (auto* v4 = std::get_if<float>(&target)) {
                float x = *v4; ok = transfer(&x, 4); if (is_get) *v4 = x;
            } else if (auto* v5 = std::get_if<double>(&target)) {
                double x = *v5; ok = transfer(&x, 8); if (is_get) *v5 = x;
            } else if (auto* v6 = std::get_if<Currency>(&target)) {
                std::int64_t x = v6->scaled; ok = transfer(&x, 8); if (is_get) v6->scaled = x;
            } else if (auto* v7 = std::get_if<DateValue>(&target)) {
                double x = v7->serial; ok = transfer(&x, 8); if (is_get) v7->serial = x;
            } else if (auto* v8 = std::get_if<bool>(&target)) {
                std::int16_t x = *v8 ? -1 : 0; ok = transfer(&x, 2); if (is_get) *v8 = x != 0;
            } else if (auto* v9 = std::get_if<std::string>(&target)) {
                // On disk a string is ANSI bytes; in memory it is Unicode.
                std::string bytes;
                if (fixed != 0U) {
                    if (!is_get) {
                        fit_to_units(*v9, fixed);
                        bytes = text_to_ansi_bytes(*v9);
                    } else {
                        bytes.assign(fixed, '\0');
                    }
                    ok = transfer(bytes.data(), fixed);
                } else {
                    bytes = is_get ? std::string(utf16_length(*v9), '\0') : text_to_ansi_bytes(*v9);
                    if (file->mode == 5 || nested) {
                        std::uint16_t length = static_cast<std::uint16_t>(bytes.size());
                        ok = transfer(&length, 2);
                        if (is_get && ok) bytes.assign(length, '\0');
                    }
                    if (ok && !bytes.empty()) {
                        ok = transfer(bytes.data(), bytes.size());
                    }
                }
                if (is_get && ok) *v9 = ansi_bytes_to_text(bytes);
            } else if (auto* array = std::get_if<ArrayValue>(&target)) {
                for (auto& element : array->elements) {
                    if (!transfer_value(element, 0, true)) return false;
                }
            } else if (auto* instance = std::get_if<ObjectInstance>(&target)) {
                const auto class_iterator = class_definitions_.find(instance->data->class_name);
                if (class_iterator == class_definitions_.end() || !class_iterator->second.is_udt) {
                    return raise_runtime(5, "Invalid procedure call or argument", statement_offset);
                }
                for (const auto& field_name : class_iterator->second.field_order) {
                    const auto field = instance->data->fields.variables.find(field_name);
                    if (field == instance->data->fields.variables.end()) continue;
                    const auto fixed_length = instance->data->fields.fixed_string_lengths.find(field_name);
                    if (!transfer_value(
                            field->second,
                            fixed_length != instance->data->fields.fixed_string_lengths.end()
                                ? fixed_length->second
                                : 0U,
                            true)) {
                        return false;
                    }
                }
            } else {
                return raise_runtime(5, "Invalid procedure call or argument", statement_offset);
            }
            if (!ok) {
                if (is_get) {
                    return raise_runtime(62, "Input past end of file", statement_offset);
                }
                return raise_runtime(57, "Device I/O error", statement_offset);
            }
            return true;
        };
        if (!transfer_value(*lvalue.ptr, lvalue.fixed, false)) {
            return false;
        }
        if (file->mode == 5) {
            const long end = record_start + file->record_length;
            if (!is_get && std::ftell(file->handle) < end) {
                std::fseek(file->handle, 0, SEEK_END);
                if (std::ftell(file->handle) < end) {
                    const std::string padding(static_cast<std::size_t>(end - std::ftell(file->handle)), '\0');
                    std::fwrite(padding.data(), 1, padding.size(), file->handle);
                }
            }
            std::fseek(file->handle, end, SEEK_SET);
        }
        return true;
    }

    [[nodiscard]] std::optional<bool> parse_file_statement(const std::size_t statement_offset) {
        const auto start = offset_;
        if (consume_keyword("open")) {
            skip_horizontal_whitespace();
            auto path = parse_expression();
            if (!path.has_value()) {
                return false;
            }
            skip_horizontal_whitespace();
            int mode = 0;
            if (consume_keyword("for")) {
                skip_horizontal_whitespace();
                if (consume_keyword("input")) mode = 1;
                else if (consume_keyword("output")) mode = 2;
                else if (consume_keyword("append")) mode = 3;
                else if (consume_keyword("binary")) mode = 4;
                else if (consume_keyword("random")) mode = 5;
                else {
                    set_error("WFC0321", "unsupported Open mode", offset_);
                    return false;
                }
            } else {
                set_error("WFC0321", "expected For after Open path", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            if (consume_keyword("access")) {
                skip_horizontal_whitespace();
                static_cast<void>(consume_keyword("read"));
                skip_horizontal_whitespace();
                static_cast<void>(consume_keyword("write"));
                skip_horizontal_whitespace();
            }
            if (consume_keyword("shared") || consume_keyword("lock")) {
                skip_horizontal_whitespace();
                static_cast<void>(consume_keyword("read") || consume_keyword("write"));
                skip_horizontal_whitespace();
                static_cast<void>(consume_keyword("write"));
                skip_horizontal_whitespace();
            }
            if (!consume_keyword("as")) {
                set_error("WFC0147", "expected As in Open statement", offset_);
                return false;
            }
            Integer number{};
            if (!parse_hash_file_number(number)) {
                return false;
            }
            skip_horizontal_whitespace();
            long record_length = 128;
            if (consume_keyword("len")) {
                skip_horizontal_whitespace();
                static_cast<void>(consume('='));
                auto length = parse_expression();
                if (!length.has_value()) {
                    return false;
                }
                if (const auto size = whole_value(*length)) {
                    record_length = *size;
                }
            }
            if (!execute_) {
                return true;
            }
            const auto* path_text = std::get_if<std::string>(&*path);
            if (path_text == nullptr) {
                set_error("WFC0073", "Open requires a String path", statement_offset);
                return false;
            }
            if (number < 1 || number > 511) {
                return raise_runtime(52, "Bad file name or number", statement_offset);
            }
            if (files_.contains(number)) {
                return raise_runtime(55, "File already open", statement_offset);
            }
            std::FILE* handle = nullptr;
            if (mode >= 4) {
                handle = open_file(*path_text, "r+b");
                if (handle == nullptr) {
                    handle = open_file(*path_text, "w+b");
                }
            } else {
                handle = open_file(*path_text, mode == 1 ? "rb" : mode == 2 ? "wb" : "ab");
            }
            if (handle == nullptr) {
                std::error_code path_error;
                const auto parent = std::filesystem::path(*path_text).parent_path();
                if (!parent.empty() && !std::filesystem::is_directory(parent, path_error)) {
                    return raise_runtime(76, "Path not found", statement_offset);
                }
                if (mode == 1 && !std::filesystem::exists(std::filesystem::path(*path_text), path_error)) {
                    return raise_runtime(53, "File not found", statement_offset);
                }
                return raise_runtime(70, "Permission denied", statement_offset);
            }
            files_[number] = OpenFile{handle, mode, record_length > 0 ? record_length : 128};
            return true;
        }
        if (consume_keyword("close")) {
            skip_horizontal_whitespace();
            std::vector<Integer> numbers;
            while (!at_statement_end()) {
                Integer number{};
                if (!parse_hash_file_number(number)) {
                    return false;
                }
                numbers.push_back(number);
                skip_horizontal_whitespace();
                if (!consume(',')) {
                    break;
                }
            }
            if (!execute_) {
                return true;
            }
            if (numbers.empty()) {
                for (auto& [n, file] : files_) std::fclose(file.handle);
                files_.clear();
                return true;
            }
            for (const auto number : numbers) {
                const auto found = files_.find(number);
                if (found != files_.end()) {
                    std::fclose(found->second.handle);
                    files_.erase(found);
                }
            }
            return true;
        }
        if (consume_keyword("write")) {
            skip_horizontal_whitespace();
            if (at_end() || current() != '#') {
                offset_ = start;
                return std::nullopt;
            }
            Integer number{};
            if (!parse_hash_file_number(number)) {
                return false;
            }
            skip_horizontal_whitespace();
            std::string line;
            if (consume(',')) {
                bool first = true;
                while (true) {
                    skip_horizontal_whitespace();
                    if (at_statement_end()) break;
                    auto value = parse_expression();
                    if (!value.has_value()) {
                        return false;
                    }
                    if (execute_) {
                        if (!first) line += ",";
                        line += write_item_text(*value);
                    }
                    first = false;
                    skip_horizontal_whitespace();
                    if (!consume(',') && !consume(';')) break;
                }
            }
            return execute_ ? write_to_file(number, line + "\r\n", statement_offset) : true;
        }
        if (consume_keyword("line")) {
            skip_horizontal_whitespace();
            if (!consume_keyword("input")) {
                offset_ = start;
                return std::nullopt;
            }
            Integer number{};
            if (!parse_hash_file_number(number)) {
                return false;
            }
            skip_horizontal_whitespace();
            if (!consume(',')) {
                set_error("WFC0014", "expected comma after file number", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            const auto variable_offset = offset_;
            char type_character{};
            auto name = parse_identifier(&type_character);
            if (!name.has_value()) {
                set_error("WFC0011", "expected variable name", variable_offset);
                return false;
            }
            const auto variable = find_variable(*name);
            if (variable.value == nullptr) {
                set_error("WFC0015", "undeclared variable", variable_offset);
                return false;
            }
            if (!execute_) {
                return true;
            }
            auto* const file = find_open_file(number, statement_offset);
            if (file == nullptr) {
                return false;
            }
            if (file->mode != 1) {
                return raise_runtime(54, "Bad file mode", statement_offset);
            }
            std::string line;
            if (!read_file_line(file->handle, line)) {
                return raise_runtime(62, "Input past end of file", statement_offset);
            }
            line = ansi_bytes_to_text(line);
            if (!std::holds_alternative<std::string>(*variable.value) &&
                !variable.scope->variant_variables.contains(*name)) {
                set_error("WFC0016", "Line Input requires a String or Variant variable", variable_offset);
                return false;
            }
            *variable.value = std::move(line);
            return true;
        }
        if (consume_keyword("input")) {
            skip_horizontal_whitespace();
            if (at_end() || current() != '#') {
                offset_ = start;
                return std::nullopt;
            }
            Integer number{};
            if (!parse_hash_file_number(number)) {
                return false;
            }
            skip_horizontal_whitespace();
            if (!consume(',')) {
                set_error("WFC0014", "expected comma after file number", offset_);
                return false;
            }
            while (true) {
                skip_horizontal_whitespace();
                const auto variable_offset = offset_;
                char type_character{};
                auto name = parse_identifier(&type_character);
                if (!name.has_value()) {
                    set_error("WFC0011", "expected variable name", variable_offset);
                    return false;
                }
                const auto variable = find_variable(*name);
                if (variable.value == nullptr) {
                    set_error("WFC0015", "undeclared variable", variable_offset);
                    return false;
                }
                if (execute_) {
                    auto* const file = find_open_file(number, statement_offset);
                    if (file == nullptr) {
                        return false;
                    }
                    if (file->mode != 1) {
                        return raise_runtime(54, "Bad file mode", statement_offset);
                    }
                    std::string token;
                    bool quoted{};
                    if (!read_input_field(file->handle, token, quoted)) {
                        return raise_runtime(62, "Input past end of file", statement_offset);
                    }
                    token = ansi_bytes_to_text(token);
                    if (!store_input_token(
                            *variable.value, variable.scope->variant_variables.contains(*name),
                            token, quoted, variable_offset)) {
                        return false;
                    }
                }
                skip_horizontal_whitespace();
                if (!consume(',')) {
                    return true;
                }
            }
        }
        if (consume_keyword("kill") || consume_keyword("mkdir") || consume_keyword("rmdir")) {
            const std::string_view word = source_.substr(start, offset_ - start);
            char first = ascii_lower(word.front());
            skip_horizontal_whitespace();
            auto path = parse_expression();
            if (!path.has_value()) {
                return false;
            }
            if (!execute_) {
                return true;
            }
            const auto* text = std::get_if<std::string>(&*path);
            if (text == nullptr) {
                set_error("WFC0073", "path must be a String", statement_offset);
                return false;
            }
            std::error_code ec;
            const std::filesystem::path target(*text);
            if (first == 'k' && target.filename().string().find_first_of("*?") != std::string::npos) {
                const auto directory =
                    target.has_parent_path() ? target.parent_path() : std::filesystem::path(".");
                const std::string mask = target.filename().string();
                std::vector<std::filesystem::path> matches;
                for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
                    if (entry.is_regular_file(ec) &&
                        like_match(entry.path().filename().string(), mask, true)) {
                        matches.push_back(entry.path());
                    }
                }
                if (matches.empty()) {
                    return raise_runtime(53, "File not found", statement_offset);
                }
                for (const auto& match : matches) {
                    std::filesystem::remove(match, ec);
                }
            } else if (first == 'k') {
                if (!std::filesystem::is_regular_file(target, ec)) {
                    return raise_runtime(53, "File not found", statement_offset);
                }
                std::filesystem::remove(target, ec);
            } else if (first == 'm') {
                if (!std::filesystem::create_directory(target, ec) || ec) {
                    return raise_runtime(75, "Path/File access error", statement_offset);
                }
            } else {
                if (!std::filesystem::is_directory(target, ec)) {
                    return raise_runtime(76, "Path not found", statement_offset);
                }
                std::filesystem::remove(target, ec);
                if (ec) {
                    return raise_runtime(75, "Path/File access error", statement_offset);
                }
            }
            return true;
        }
        if (consume_keyword("name") || consume_keyword("chdir")) {
            const bool is_name = ascii_lower(source_[start + 0]) == 'n';
            skip_horizontal_whitespace();
            if (is_name && (at_statement_end() || current() == '=' || current() == '(' ||
                            current() == '.' || current() == ',')) {
                offset_ = start;  // an ordinary variable called Name
                return std::nullopt;
            }
            auto first = parse_expression();
            if (!first.has_value()) {
                return false;
            }
            std::optional<Value> second;
            if (is_name) {
                skip_horizontal_whitespace();
                if (!consume_keyword("as")) {
                    set_error("WFC0147", "expected As in Name statement", offset_);
                    return false;
                }
                skip_horizontal_whitespace();
                second = parse_expression();
                if (!second.has_value()) {
                    return false;
                }
            }
            if (!execute_) {
                return true;
            }
            const auto* from_text = std::get_if<std::string>(&*first);
            const auto* to_text = second ? std::get_if<std::string>(&*second) : nullptr;
            if (from_text == nullptr || (is_name && to_text == nullptr)) {
                set_error("WFC0073", "path must be a String", statement_offset);
                return false;
            }
            std::error_code ec;
            if (is_name) {
                if (!std::filesystem::exists(*from_text, ec)) {
                    return raise_runtime(53, "File not found", statement_offset);
                }
                if (std::filesystem::exists(*to_text, ec)) {
                    return raise_runtime(58, "File already exists", statement_offset);
                }
                std::filesystem::rename(*from_text, *to_text, ec);
                if (ec) {
                    return raise_runtime(75, "Path/File access error", statement_offset);
                }
            } else {
                std::filesystem::current_path(*from_text, ec);
                if (ec) {
                    return raise_runtime(76, "Path not found", statement_offset);
                }
            }
            return true;
        }
        if (consume_keyword("filecopy")) {
            skip_horizontal_whitespace();
            auto from = parse_expression();
            if (!from.has_value()) {
                return false;
            }
            skip_horizontal_whitespace();
            if (!consume(',')) {
                set_error("WFC0014", "expected comma in FileCopy", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            auto to = parse_expression();
            if (!to.has_value()) {
                return false;
            }
            if (!execute_) {
                return true;
            }
            const auto* source_text = std::get_if<std::string>(&*from);
            const auto* target_text = std::get_if<std::string>(&*to);
            if (source_text == nullptr || target_text == nullptr) {
                set_error("WFC0073", "FileCopy requires String paths", statement_offset);
                return false;
            }
            std::error_code ec;
            std::filesystem::copy_file(
                *source_text, *target_text, std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) {
                return raise_runtime(53, "File not found", statement_offset);
            }
            return true;
        }
        offset_ = start;
        return std::nullopt;
    }

    [[nodiscard]] static bool is_file_function_name(const std::string_view name) {
        return name == "eof" || name == "lof" || name == "freefile" || name == "dir" ||
               name == "dir$" || name == "curdir" || name == "curdir$" || name == "filelen" ||
               name == "input" || name == "input$" || name == "inputb" || name == "inputb$" ||
               name == "environ" || name == "environ$" || name == "loc" || name == "seek";
    }

    [[nodiscard]] std::optional<Value> evaluate_file_function(
        const std::string_view name, std::vector<Value>& arguments, const std::size_t offset) {
        const auto count = arguments.size();
        const auto arity = [&](const std::size_t low, const std::size_t high) {
            if (count < low || count > high) {
                set_error("WFC0072", "function received the wrong number of arguments", offset);
                return false;
            }
            return true;
        };
        const auto long_at = [&](const std::size_t index) -> std::optional<Integer> {
            if (const auto* i = std::get_if<Integer>(&arguments[index])) return *i;
            if (const auto* i = std::get_if<Int16>(&arguments[index])) return static_cast<Integer>(*i);
            return std::nullopt;
        };
        if (name == "freefile") {
            if (!arity(0, 1)) return std::nullopt;
            if (!execute_) return Value{Integer{}};
            for (Integer n = 1; n <= 255; ++n) {
                if (!files_.contains(n)) return Value{n};
            }
            return Value{Integer{}};
        }
        if (name == "seek") {
            if (!arity(1, 1)) return std::nullopt;
            const auto number = long_at(0);
            if (!number) {
                set_error("WFC0073", "file number must be a Long", offset);
                return std::nullopt;
            }
            if (!execute_) return Value{Integer{}};
            auto* const file = find_open_file(*number, offset);
            if (file == nullptr) return std::nullopt;
            const long position = std::ftell(file->handle);
            const long unit = file->mode == 5 ? file->record_length : 1;
            return Value{static_cast<Integer>(position / unit + 1)};
        }
        if (name == "eof" || name == "lof" || name == "loc") {
            if (!arity(1, 1)) return std::nullopt;
            const auto number = long_at(0);
            if (!number) {
                set_error("WFC0073", "file number must be a Long", offset);
                return std::nullopt;
            }
            if (!execute_) return Value{name == "eof" ? Value{false} : Value{Integer{}}};
            auto* const file = find_open_file(*number, offset);
            if (file == nullptr) return std::nullopt;
            if (name == "eof") {
                return Value{file->mode == 1 || file->mode >= 4 ? file_at_eof(file->handle) : true};
            }
            std::fflush(file->handle);
            const long position = std::ftell(file->handle);
            if (name == "loc") return Value{static_cast<Integer>(position < 0 ? 0 : position)};
            std::fseek(file->handle, 0, SEEK_END);
            const long size = std::ftell(file->handle);
            std::fseek(file->handle, position, SEEK_SET);
            return Value{static_cast<Integer>(size < 0 ? 0 : size)};
        }
        if (name == "curdir" || name == "curdir$") {
            if (!arity(0, 1)) return std::nullopt;
            std::error_code ec;
            return Value{execute_ ? std::filesystem::current_path(ec).string() : std::string{}};
        }
        if (name == "filelen") {
            if (!arity(1, 1)) return std::nullopt;
            const auto* path = std::get_if<std::string>(&arguments[0]);
            if (path == nullptr) {
                set_error("WFC0073", "FileLen requires a String path", offset);
                return std::nullopt;
            }
            if (!execute_) return Value{Integer{}};
            std::error_code ec;
            const auto size = std::filesystem::file_size(*path, ec);
            if (ec) {
                static_cast<void>(raise_runtime(53, "File not found", offset));
                return std::nullopt;
            }
            return Value{static_cast<Integer>(size)};
        }
        if (name == "environ" || name == "environ$") {
            if (!arity(1, 1)) return std::nullopt;
            if (!execute_) return Value{std::string{}};
            if (const auto* variable = std::get_if<std::string>(&arguments[0])) {
                return Value{environment_variable(*variable)};
            }
            set_error("WFC0073", "Environ requires a String name", offset);
            return std::nullopt;
        }
        if (name == "dir" || name == "dir$") {
            if (!arity(0, 2)) return std::nullopt;
            if (!execute_) return Value{std::string{}};
            if (count >= 1U) {
                const auto* pattern = std::get_if<std::string>(&arguments[0]);
                if (pattern == nullptr) {
                    set_error("WFC0073", "Dir requires a String pattern", offset);
                    return std::nullopt;
                }
                dir_matches_.clear();
                dir_index_ = 0;
                std::error_code ec;
                std::filesystem::path full(*pattern);
                const auto directory = full.has_parent_path() ? full.parent_path() : std::filesystem::path(".");
                const std::string mask = full.filename().string();
                const bool want_directories = count >= 2U && long_at(1).value_or(0) & 16;
                if (mask.find_first_of("*?") == std::string::npos) {
                    if (std::filesystem::exists(full, ec) &&
                        (want_directories || !std::filesystem::is_directory(full, ec))) {
                        dir_matches_.push_back(full.filename().string());
                    }
                } else {
                    for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
                        const std::string entry_name = entry.path().filename().string();
                        if (!want_directories && entry.is_directory(ec)) {
                            continue;
                        }
                        if (like_match(entry_name, mask, true)) {
                            dir_matches_.push_back(entry_name);
                        }
                    }
                    std::sort(dir_matches_.begin(), dir_matches_.end());
                    if (want_directories) {
                        for (const char* dots : {"..", "."}) {
                            if (like_match(dots, mask, true)) {
                                dir_matches_.insert(dir_matches_.begin(), dots);
                            }
                        }
                    }
                }
            }
            if (dir_index_ < dir_matches_.size()) {
                return Value{dir_matches_[dir_index_++]};
            }
            return Value{std::string{}};
        }
        if (name == "input" || name == "input$" || name == "inputb" || name == "inputb$") {
            if (!arity(2, 2)) return std::nullopt;
            const auto length = long_at(0);
            const auto number = long_at(1);
            if (!length || !number) {
                set_error("WFC0073", "Input requires Long arguments", offset);
                return std::nullopt;
            }
            if (!execute_) return Value{std::string{}};
            auto* const file = find_open_file(*number, offset);
            if (file == nullptr) return std::nullopt;
            if (file->mode != 1) {
                static_cast<void>(raise_runtime(54, "Bad file mode", offset));
                return std::nullopt;
            }
            std::string text;
            for (Integer i = 0; i < *length; ++i) {
                const int c = std::fgetc(file->handle);
                if (c == EOF) {
                    static_cast<void>(raise_runtime(62, "Input past end of file", offset));
                    return std::nullopt;
                }
                text.push_back(static_cast<char>(c));
            }
            return Value{ansi_bytes_to_text(text)};
        }
        set_error("WFC0071", "unsupported function", offset);
        return std::nullopt;
    }

    // Consumes `#n,` (file number) when present; `file_number` < 0 means
    // "standard output". A `#` followed by digits and a comma is a file
    // number, anything else (`Print #1/1/2000#`) is an expression.
    [[nodiscard]] bool parse_file_number_prefix(Integer& file_number, bool& found) {
        found = false;
        file_number = -1;
        skip_horizontal_whitespace();
        if (at_end() || current() != '#') {
            return true;
        }
        std::size_t look = offset_ + 1;
        while (look < source_.size() && is_identifier_part(source_[look])) {
            ++look;
        }
        std::size_t after = look;
        while (after < source_.size() && (source_[after] == ' ' || source_[after] == '\t')) ++after;
        if (look == offset_ + 1 || after >= source_.size() ||
            (source_[after] != ',' && source_[after] != '\r' && source_[after] != '\n' &&
             source_[after] != ':')) {
            return true;
        }
        advance();
        const auto number_offset = offset_;
        auto number = parse_expression();
        if (!number.has_value()) {
            return false;
        }
        if (!coerce_numeric_value(*number, Value{Integer{}}.index(), number_offset) ||
            !std::holds_alternative<Integer>(*number)) {
            set_error("WFC0073", "file number must be a Long", number_offset);
            return false;
        }
        file_number = std::get<Integer>(*number);
        found = true;
        skip_horizontal_whitespace();
        static_cast<void>(consume(','));
        return true;
    }

    [[nodiscard]] bool at_statement_end() const noexcept {
        return at_end() || current() == '\r' || current() == '\n' || current() == ':' ||
               current() == '\'';
    }

    // `Print [#n,] item [; | , item ...] [;]` -- `;` joins, `,` advances to
    // the next 14-column zone, a trailing separator suppresses the newline,
    // `Spc(n)` and `Tab(n)` pad (REQ-0245).
    [[nodiscard]] bool parse_print_statement() {
        if (!allow_identifiers_) {
            // The expression-only `Print <expression>` entry point keeps its
            // original single-expression grammar.
            skip_horizontal_whitespace();
            auto value = parse_expression();
            if (!value.has_value()) {
                return false;
            }
            if (execute_) {
                if (has_output_line_) {
                    output_.push_back('\n');
                }
                output_ += render(*value);
                has_output_line_ = true;
            }
            return true;
        }
        Integer file_number{};
        bool to_file{};
        if (!parse_file_number_prefix(file_number, to_file)) {
            return false;
        }
        std::string text;
        bool newline = true;
        while (true) {
            skip_horizontal_whitespace();
            if (at_statement_end()) {
                break;
            }
            {
                const auto before_else = offset_;
                if (consume_keyword("else")) {
                    offset_ = before_else;
                    break;
                }
            }
            if (current() == ';') {
                advance();
                newline = false;
                continue;
            }
            if (current() == ',') {
                advance();
                text.append(14U - text.size() % 14U, ' ');
                newline = false;
                continue;
            }
            const auto save = offset_;
            const bool is_spc = consume_keyword("spc");
            const bool is_tab = !is_spc && consume_keyword("tab");
            if (is_spc || is_tab) {
                skip_horizontal_whitespace();
                if (consume('(')) {
                    skip_horizontal_whitespace();
                    auto amount = parse_expression();
                    if (!amount.has_value()) {
                        return false;
                    }
                    skip_horizontal_whitespace();
                    if (!consume(')')) {
                        set_error("WFC0005", "expected closing parenthesis", offset_);
                        return false;
                    }
                    const auto count_value = whole_value(*amount);
                    const Integer* const count = count_value ? &*count_value : nullptr;
                    if (count == nullptr) {
                        set_error("WFC0073", "Spc/Tab requires a Long argument", offset_);
                        return false;
                    }
                    if (execute_ && *count > 0) {
                        if (is_spc) {
                            text.append(static_cast<std::size_t>(*count), ' ');
                        } else if (static_cast<std::size_t>(*count - 1) > text.size()) {
                            text.append(static_cast<std::size_t>(*count - 1) - text.size(), ' ');
                        }
                    }
                    newline = true;
                    continue;
                }
                offset_ = save;
            }
            auto value = parse_expression();
            if (!value.has_value() || !resolve_default_value(*value, offset_)) {
                return false;
            }
            if (execute_) {
                if (std::holds_alternative<Null>(*value)) {
                    text += "Null";
                } else if (vb_number_spacing_ && is_number(*value) &&
                           !std::holds_alternative<DateValue>(*value)) {
                    // VB6 reserves a sign position before a number and adds a trailing space.
                    const std::string digits = render(*value);
                    if (digits.empty() || digits.front() != '-') text.push_back(' ');
                    text += digits;
                    text.push_back(' ');
                } else {
                    text += render(*value);
                }
            }
            newline = true;
        }
        if (!execute_) {
            return true;
        }
        if (discard_print_) {
            // `Debug.Print`: the Immediate window; kept apart from the output.
            debug_output_ += text;
            if (newline) debug_output_.push_back('\n');
            return true;
        }
        if (to_file) {
            return write_to_file(file_number, newline ? text + "\r\n" : text, offset_);
        }
        if (has_output_line_ && !output_line_open_) {
            output_.push_back('\n');
        }
        output_ += text;
        has_output_line_ = true;
        output_line_open_ = !newline;
        return true;
    }

    [[nodiscard]] bool parse_randomize_statement(const std::size_t statement_offset) {
        skip_horizontal_whitespace();
        if (at_end() || current() == ':' || current() == '\r' || current() == '\n' ||
            current() == '\'') {
            if (execute_) {
                rnd_state_ = seed_from_number(entropy_seed());
            }
            return true;
        }
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        if (!is_number(*value) && !std::holds_alternative<bool>(*value)) {
            if (!execute_) return true;
            set_error("WFC0073", "Randomize requires a numeric seed", statement_offset);
            return false;
        }
        if (execute_) {
            const double seed = std::holds_alternative<bool>(*value)
                                     ? (std::get<bool>(*value) ? -1.0 : 0.0)
                                     : as_double(*value);
            rnd_state_ = seed_from_number(seed);
        }
        return true;
    }

    [[nodiscard]] bool parse_option_statement(const std::size_t statement_offset) {
        if (!allow_declarations_) {
            set_error("WFC0068", "Option directives are only valid at module level", statement_offset);
            return false;
        }
        skip_horizontal_whitespace();
        if (module_body_started_) {
            set_error("WFC0066", "Option directives must precede module statements", statement_offset);
            return false;
        }
        if (consume_keyword("private")) {
            // `Option Private Module`: every module is private to the project already.
            skip_horizontal_whitespace();
            static_cast<void>(consume_keyword("module"));
            return true;
        }
        if (consume_keyword("explicit")) {
            if (option_explicit_) {
                set_error("WFC0067", "duplicate Option Explicit", statement_offset);
                return false;
            }
            option_explicit_ = true;
            return true;
        }
        if (consume_keyword("compare")) {
            if (option_compare_set_) {
                set_error("WFC0069", "duplicate Option Compare", statement_offset);
                return false;
            }
            skip_horizontal_whitespace();
            if (consume_keyword("binary") || consume_keyword("database")) {
                option_compare_text_ = false;
            } else if (consume_keyword("text")) {
                option_compare_text_ = true;
            } else {
                set_error("WFC0070", "expected Binary or Text after Option Compare", offset_);
                return false;
            }
            option_compare_set_ = true;
            return true;
        }
        if (consume_keyword("base")) {
            if (option_base_set_) {
                set_error("WFC0152", "duplicate Option Base", statement_offset);
                return false;
            }
            skip_horizontal_whitespace();
            if (consume('0')) {
                option_base_one_ = false;
            } else if (consume('1')) {
                option_base_one_ = true;
            } else {
                set_error("WFC0153", "expected 0 or 1 after Option Base", offset_);
                return false;
            }
            option_base_set_ = true;
            return true;
        }
        set_error("WFC0065", "expected Explicit, Compare, or Base after Option", offset_);
        return false;
    }

    [[nodiscard]] bool parse_exit_statement(const std::size_t statement_offset) {
        skip_horizontal_whitespace();
        if (consume_keyword("do")) {
            if (do_depth_ == 0U) {
                set_error("WFC0042", "Exit Do is not inside a Do loop", statement_offset);
                return false;
            }
            if (execute_) {
                exit_do_requested_ = true;
            }
            return true;
        }
        if (consume_keyword("for")) {
            if (for_depth_ == 0U) {
                set_error("WFC0052", "Exit For is not inside a For loop", statement_offset);
                return false;
            }
            if (execute_) {
                exit_for_requested_ = true;
            }
            return true;
        }
        if (consume_keyword("sub")) {
            if (!in_procedure() || current_scope().is_function_frame) {
                set_error("WFC0124", "Exit Sub is not inside a Sub", statement_offset);
                return false;
            }
            if (execute_) {
                exit_sub_requested_ = true;
            }
            return true;
        }
        if (consume_keyword("function")) {
            if (!in_procedure() || !current_scope().is_function_frame) {
                set_error("WFC0125", "Exit Function is not inside a Function", statement_offset);
                return false;
            }
            if (execute_) {
                exit_function_requested_ = true;
            }
            return true;
        }
        if (consume_keyword("property")) {
            if (!in_procedure()) {
                set_error("WFC0124", "Exit Property is not inside a Property", statement_offset);
                return false;
            }
            if (execute_) {
                if (current_scope().is_function_frame) {
                    exit_function_requested_ = true;
                } else {
                    exit_sub_requested_ = true;
                }
            }
            return true;
        }
        set_error("WFC0041", "expected Do, For, Sub, Function, or Property after Exit", offset_);
        return false;
    }

    [[nodiscard]] bool control_exit_requested() const noexcept {
        return exit_do_requested_ || exit_for_requested_ || exit_sub_requested_ ||
               exit_function_requested_;
    }

    [[nodiscard]] bool parse_if_statement() {
        skip_horizontal_whitespace();
        const auto condition_offset = offset_;
        auto condition = parse_expression();
        if (!condition.has_value()) {
            return false;
        }
        const auto boolean =
            coerce_condition_boolean(*condition, condition_offset, "WFC0021", "If condition must be Boolean");
        if (!boolean.has_value()) {
            return false;
        }

        skip_horizontal_whitespace();
        if (!consume_keyword("then")) {
            set_error("WFC0022", "expected Then", offset_);
            return false;
        }

        const bool enclosing_execution = execute_;
        skip_horizontal_whitespace();
        if (!at_end() && (current() == '\r' || current() == '\n' || current() == '\'')) {
            return parse_block_if_statement(enclosing_execution, *boolean);
        }
        execute_ = enclosing_execution && *boolean;
        if (!parse_inline_statement_list()) {
            execute_ = enclosing_execution;
            return false;
        }

        skip_horizontal_whitespace();
        if (consume_keyword("else")) {
            execute_ = enclosing_execution && !*boolean && !control_exit_requested();
            if (!parse_inline_statement_list()) {
                execute_ = enclosing_execution;
                return false;
            }
        }
        execute_ = control_exit_requested() ? false : enclosing_execution;
        return true;
    }

    // REQ-0271: `If c Then a : b Else c : d` -- colon-separated statements
    // after Then / Else all belong to that branch.
    [[nodiscard]] bool parse_inline_statement_list() {
        const bool branch_execution = execute_;
        while (true) {
            if (!parse_inline_statement()) {
                return false;
            }
            if (control_exit_requested()) {
                execute_ = false;
            }
            skip_horizontal_whitespace();
            if (at_end() || current() != ':') {
                execute_ = branch_execution && !control_exit_requested();
                return true;
            }
            const auto colon_offset = offset_;
            advance();
            skip_horizontal_whitespace();
            const auto probe = offset_;
            if (at_end() || current() == '\r' || current() == '\n' || current() == '\'' ||
                consume_keyword("else")) {
                offset_ = at_end() || current() == '\r' || current() == '\n' || current() == '\''
                              ? colon_offset
                              : probe;
                execute_ = branch_execution && !control_exit_requested();
                return true;
            }
            offset_ = probe;
        }
    }

    [[nodiscard]] bool parse_block_if_statement(
        const bool enclosing_execution,
        const bool condition) {
        if (!consume_block_line_end()) {
            return false;
        }

        bool has_else{};
        bool branch_selected = condition;
        execute_ = enclosing_execution && condition;
        while (true) {
            skip_program_leading_trivia();
            if (at_end()) {
                execute_ = enclosing_execution;
                set_error("WFC0024", "expected End If", offset_);
                return false;
            }
            if (consume_keyword("elseif")) {
                const auto elseif_offset = offset_ - 6U;
                if (has_else) {
                    execute_ = enclosing_execution;
                    set_error("WFC0030", "ElseIf is not permitted after Else", elseif_offset);
                    return false;
                }

                const bool evaluate_condition = enclosing_execution && !branch_selected;
                execute_ = evaluate_condition;
                skip_horizontal_whitespace();
                const auto condition_offset = offset_;
                auto elseif_condition = parse_expression();
                if (!elseif_condition.has_value()) {
                    execute_ = enclosing_execution;
                    return false;
                }
                const auto elseif_boolean = coerce_condition_boolean(
                    *elseif_condition, condition_offset, "WFC0028", "ElseIf condition must be Boolean");
                if (!elseif_boolean.has_value()) {
                    execute_ = enclosing_execution;
                    return false;
                }

                skip_horizontal_whitespace();
                if (!consume_keyword("then")) {
                    execute_ = enclosing_execution;
                    set_error("WFC0029", "expected Then after ElseIf", offset_);
                    return false;
                }
                if (!consume_block_line_end()) {
                    execute_ = enclosing_execution;
                    return false;
                }

                const bool select_branch = !branch_selected && *elseif_boolean;
                branch_selected = branch_selected || *elseif_boolean;
                execute_ = enclosing_execution && select_branch;
                continue;
            }
            if (consume_keyword("else")) {
                if (has_else) {
                    execute_ = enclosing_execution;
                    set_error("WFC0026", "duplicate Else", offset_ - 4U);
                    return false;
                }
                has_else = true;
                if (!consume_block_line_end()) {
                    execute_ = enclosing_execution;
                    return false;
                }
                execute_ = enclosing_execution && !branch_selected;
                continue;
            }
            if (consume_keyword("end")) {
                skip_horizontal_whitespace();
                if (!consume_keyword("if")) {
                    execute_ = enclosing_execution;
                    set_error("WFC0025", "expected If after End", offset_);
                    return false;
                }
                execute_ = control_exit_requested() ? false : enclosing_execution;
                return true;
            }
            const bool enclosing_declaration_permission = allow_declarations_;
            allow_declarations_ = false;
            const bool parsed_statement = parse_statement();
            const bool consumed_statement_end = parsed_statement && consume_statement_end();
            allow_declarations_ = enclosing_declaration_permission;
            if (!parsed_statement || !consumed_statement_end) {
                execute_ = enclosing_execution;
                return false;
            }
            if (control_exit_requested()) {
                execute_ = false;
            }
        }
    }

    [[nodiscard]] bool parse_while_statement() {
        const bool enclosing_execution = execute_;
        skip_horizontal_whitespace();
        const auto condition_offset = offset_;
        auto condition = parse_expression();
        if (!condition.has_value()) {
            return false;
        }
        const auto boolean = coerce_condition_boolean(
            *condition, condition_offset, "WFC0031", "While condition must be Boolean");
        if (!boolean.has_value()) {
            return false;
        }
        if (!consume_loop_header_end()) {
            return false;
        }

        const auto body_offset = offset_;
        std::size_t continuation_offset{};
        if (!enclosing_execution || !*boolean) {
            execute_ = false;
            const bool parsed_body = parse_while_body(continuation_offset);
            execute_ = enclosing_execution;
            return parsed_body;
        }

        bool continue_loop = true;
        while (continue_loop) {
            offset_ = body_offset;
            execute_ = enclosing_execution;
            if (!parse_while_body(continuation_offset)) {
                execute_ = enclosing_execution;
                return false;
            }
            if (control_exit_requested()) {
                offset_ = continuation_offset;
                execute_ = false;
                return true;
            }

            offset_ = condition_offset;
            execute_ = enclosing_execution;
            auto next_condition = parse_expression();
            if (!next_condition.has_value()) {
                execute_ = enclosing_execution;
                return false;
            }
            const auto next_boolean = coerce_condition_boolean(
                *next_condition, condition_offset, "WFC0031", "While condition must be Boolean");
            if (!next_boolean.has_value()) {
                execute_ = enclosing_execution;
                return false;
            }
            if (!consume_loop_header_end()) {
                execute_ = enclosing_execution;
                return false;
            }
            continue_loop = *next_boolean;
        }

        offset_ = continuation_offset;
        execute_ = enclosing_execution;
        return true;
    }

    // `Type Name ... End Type` was registered as a class by scan_udt_types
    // (REQ-0241); at run time the block is just skipped.
    [[nodiscard]] bool parse_type_statement_skip(const std::size_t statement_offset) {
        skip_rest_of_line();
        while (true) {
            skip_program_leading_trivia();
            if (at_end()) {
                set_error("WFC0025", "expected End Type", statement_offset);
                return false;
            }
            if (consume_keyword("end")) {
                skip_horizontal_whitespace();
                if (consume_keyword("type")) {
                    return true;
                }
            }
            skip_rest_of_line();
        }
    }

    // `Enum Name` ... `End Enum` (REQ-0237): each member becomes a Long
    // module constant; `As Name` is accepted wherever `As Long` is.
    [[nodiscard]] bool parse_enum_statement(const std::size_t statement_offset) {
        skip_horizontal_whitespace();
        char type_character{};
        const auto name_offset = offset_;
        auto enum_name = parse_identifier(&type_character);
        if (!enum_name.has_value()) {
            set_error("WFC0011", "expected Enum name", name_offset);
            return false;
        }
        if (!consume_block_line_end()) {
            return false;
        }
        Integer next_value = 0;
        while (true) {
            skip_program_leading_trivia();
            if (at_end()) {
                set_error("WFC0025", "expected End Enum", statement_offset);
                return false;
            }
            if (consume_keyword("end")) {
                skip_horizontal_whitespace();
                if (!consume_keyword("enum")) {
                    set_error("WFC0025", "expected Enum after End", offset_);
                    return false;
                }
                return true;
            }
            const auto member_offset = offset_;
            auto member = parse_identifier(&type_character);
            if (!member.has_value() || type_character != '\0' ||
                is_reserved_identifier(*member)) {
                set_error("WFC0011", "expected Enum member name", member_offset);
                return false;
            }
            skip_horizontal_whitespace();
            if (consume('=')) {
                skip_horizontal_whitespace();
                constant_expression_ = true;
                auto value = parse_expression();
                constant_expression_ = false;
                if (!value.has_value()) {
                    return false;
                }
                if (!coerce_numeric_value(*value, Value{Integer{}}.index(), member_offset) ||
                    !std::holds_alternative<Integer>(*value)) {
                    set_error("WFC0016", "Enum member value must be a Long", member_offset);
                    return false;
                }
                next_value = std::get<Integer>(*value);
            }
            if (execute_) {
                if (current_scope().variables.contains(*member)) {
                    set_error(
                        "WFC0013", "duplicate variable or constant declaration", member_offset);
                    return false;
                }
                current_scope().variables.emplace(*member, Value{next_value});
                current_scope().constants.insert(*member);
            }
            ++next_value;
            if (!consume_block_line_end()) {
                return false;
            }
        }
    }

    // `With expr ... End With` (REQ-0236): the object is held in a hidden
    // variable named "with.N" (un-spellable in source), and parse_identifier
    // yields that name for a leading `.member`.
    [[nodiscard]] bool parse_with_statement(const std::size_t statement_offset) {
        const bool enclosing_execution = execute_;
        skip_horizontal_whitespace();
        const auto expression_offset = offset_;
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        const bool is_object = std::holds_alternative<ObjectInstance>(*value) ||
            std::holds_alternative<Nothing>(*value);
        if (enclosing_execution && !is_object) {
            set_error("WFC0136", "With requires an object reference", expression_offset);
            return false;
        }
        if (!consume_loop_header_end()) {
            return false;
        }
        const std::string name = "with." + std::to_string(++with_counter_);
        with_scope_.object_variables.insert(name);
        with_slots_[name] = is_object ? std::move(*value) : Value{Nothing{}};
        with_names_.push_back(name);
        const auto cleanup = [&] {
            with_names_.pop_back();
            with_slots_.erase(name);
        };
        while (true) {
            skip_program_leading_trivia();
            if (at_end()) {
                cleanup();
                set_error("WFC0025", "expected End With", statement_offset);
                return false;
            }
            if (consume_keyword("end")) {
                skip_horizontal_whitespace();
                if (!consume_keyword("with")) {
                    cleanup();
                    set_error("WFC0025", "expected With after End", offset_);
                    return false;
                }
                cleanup();
                execute_ = control_exit_requested() ? false : enclosing_execution;
                return true;
            }
            const bool enclosing_declaration_permission = allow_declarations_;
            allow_declarations_ = false;
            const bool parsed_statement = parse_statement();
            const bool consumed_statement_end = parsed_statement && consume_statement_end();
            allow_declarations_ = enclosing_declaration_permission;
            if (!parsed_statement || !consumed_statement_end) {
                cleanup();
                execute_ = enclosing_execution;
                return false;
            }
            if (control_exit_requested()) {
                execute_ = false;
            }
        }
    }

    // REQ-0252: offset of the line that closes the innermost loop
    // (`Loop`/`Next`/`Wend`) enclosing `from`, scanning forward with simple
    // nesting counts; npos when none is found.
    [[nodiscard]] std::size_t find_loop_end(const std::size_t from) const {
        std::size_t position = source_.find('\n', from);
        if (position == std::string_view::npos) {
            return std::string_view::npos;
        }
        ++position;
        int depth = 0;
        while (position < source_.size()) {
            std::size_t line_end = source_.find('\n', position);
            if (line_end == std::string_view::npos) {
                line_end = source_.size();
            }
            std::size_t i = position;
            while (i < line_end && (source_[i] == ' ' || source_[i] == '\t')) ++i;
            const auto word_is = [&](const std::string_view w) {
                if (line_end - i < w.size()) return false;
                for (std::size_t k = 0; k < w.size(); ++k) {
                    if (ascii_lower(source_[i + k]) != w[k]) return false;
                }
                return line_end - i == w.size() || !is_identifier_part(source_[i + w.size()]);
            };
            if (word_is("do") || word_is("while") || word_is("for")) {
                ++depth;
            } else if (word_is("loop") || word_is("wend") || word_is("next")) {
                if (depth == 0) {
                    return position;
                }
                --depth;
                if (word_is("next")) {
                    // `Next j, i` closes several loops at once.
                    for (std::size_t k = i; k < line_end && source_[k] != '\''; ++k) {
                        if (source_[k] == ',' && depth > 0) --depth;
                    }
                }
            }
            position = line_end + 1;
        }
        return std::string_view::npos;
    }

    // A pending `GoTo`/`GoSub`/`Resume` jump whose label lies inside the loop
    // body currently being parsed is taken in place, keeping the block
    // context (`GoTo skip` ... `skip:` ... `Next`).
    [[nodiscard]] bool take_local_jump(
        const std::size_t body_start, const std::size_t statement_start) {
        if (!jump_pending_) {
            return false;
        }
        const auto target = jump_target_;
        bool local = false;
        if (target >= body_start && target <= statement_start) {
            local = true;
        } else if (target > statement_start) {
            const auto end = find_loop_end(statement_start);
            local = end != std::string_view::npos && target < end;
        }
        if (!local) {
            return false;
        }
        jump_pending_ = false;
        error_ = wfc::Evaluation{};
        offset_ = target;
        execute_ = true;
        return true;
    }

    [[nodiscard]] bool parse_while_body(std::size_t& continuation_offset) {
        const auto body_start = offset_;
        while (true) {
            skip_program_leading_trivia();
            if (at_end()) {
                set_error("WFC0032", "expected Wend", offset_);
                return false;
            }
            if (consume_keyword("wend")) {
                continuation_offset = offset_;
                return true;
            }

            const auto statement_offset = offset_;
            const bool enclosing_declaration_permission = allow_declarations_;
            allow_declarations_ = false;
            const bool parsed_statement = parse_statement();
            const bool consumed_statement_end = parsed_statement && consume_statement_end();
            allow_declarations_ = enclosing_declaration_permission;
            if (!parsed_statement && take_local_jump(body_start, statement_offset)) {
                continue;
            }
            if (!parsed_statement || !consumed_statement_end) {
                return false;
            }
            if (control_exit_requested()) {
                execute_ = false;
            }
        }
    }

    [[nodiscard]] bool parse_do_statement() {
        const bool enclosing_execution = execute_;
        skip_horizontal_whitespace();
        if (!at_end() && (current() == '\r' || current() == '\n' || current() == ':' || current() == '\'')) {
            return parse_posttest_do_statement(enclosing_execution);
        }
        bool until{};
        if (consume_keyword("while")) {
            until = false;
        } else if (consume_keyword("until")) {
            until = true;
        } else {
            set_error("WFC0036", "expected While or Until after Do", offset_);
            return false;
        }

        skip_horizontal_whitespace();
        const auto condition_offset = offset_;
        auto condition = parse_expression();
        if (!condition.has_value()) {
            return false;
        }
        const auto boolean = coerce_condition_boolean(
            *condition, condition_offset, "WFC0035", "Do condition must be Boolean");
        if (!boolean.has_value()) {
            return false;
        }
        if (!consume_loop_header_end()) {
            return false;
        }

        const auto body_offset = offset_;
        std::size_t continuation_offset{};
        bool continue_loop = until ? !*boolean : *boolean;
        if (!enclosing_execution || !continue_loop) {
            execute_ = false;
            ++do_depth_;
            const bool parsed_body = parse_do_body(continuation_offset);
            --do_depth_;
            execute_ = enclosing_execution;
            return parsed_body;
        }

        while (continue_loop) {
            offset_ = body_offset;
            execute_ = enclosing_execution;
            ++do_depth_;
            const bool parsed_body = parse_do_body(continuation_offset);
            --do_depth_;
            if (!parsed_body) {
                execute_ = enclosing_execution;
                return false;
            }
            if (exit_do_requested_) {
                exit_do_requested_ = false;
                offset_ = continuation_offset;
                execute_ = enclosing_execution;
                return true;
            }
            if (exit_for_requested_ || exit_sub_requested_ || exit_function_requested_) {
                offset_ = continuation_offset;
                execute_ = false;
                return true;
            }

            offset_ = condition_offset;
            execute_ = enclosing_execution;
            auto next_condition = parse_expression();
            if (!next_condition.has_value()) {
                execute_ = enclosing_execution;
                return false;
            }
            const auto next_boolean = coerce_condition_boolean(
                *next_condition, condition_offset, "WFC0035", "Do condition must be Boolean");
            if (!next_boolean.has_value()) {
                execute_ = enclosing_execution;
                return false;
            }
            if (!consume_loop_header_end()) {
                execute_ = enclosing_execution;
                return false;
            }
            continue_loop = until ? !*next_boolean : *next_boolean;
        }

        offset_ = continuation_offset;
        execute_ = enclosing_execution;
        return true;
    }

    [[nodiscard]] bool parse_posttest_do_statement(const bool enclosing_execution) {
        if (!consume_loop_header_end()) {
            return false;
        }

        const auto body_offset = offset_;
        std::size_t continuation_offset{};
        bool continue_loop{};
        do {
            offset_ = body_offset;
            execute_ = enclosing_execution;
            ++do_depth_;
            const bool parsed_body = parse_do_body(continuation_offset);
            --do_depth_;
            if (!parsed_body) {
                execute_ = enclosing_execution;
                return false;
            }
            const bool exit_do_requested = exit_do_requested_;
            const bool exit_for_requested =
                exit_for_requested_ || exit_sub_requested_ || exit_function_requested_;

            skip_horizontal_whitespace();
            bool until{};
            bool has_condition = true;
            if (consume_keyword("while")) {
                until = false;
            } else if (consume_keyword("until")) {
                until = true;
            } else if (at_end() || current() == '\r' || current() == '\n' ||
                       current() == ':' || current() == '\'') {
                // REQ-0225: an unconditional `Do ... Loop` (no `While`/
                // `Until` on either the `Do` or the `Loop` line) repeats
                // forever, relying entirely on `Exit Do`/`Exit For` to end
                // it -- the same convention `Do While True` already lets a
                // caller express, just without writing a condition at all.
                has_condition = false;
            } else {
                execute_ = enclosing_execution;
                set_error("WFC0040", "expected While or Until after Loop", offset_);
                return false;
            }

            if (has_condition) {
                skip_horizontal_whitespace();
                const auto condition_offset = offset_;
                auto condition = parse_expression();
                if (!condition.has_value()) {
                    execute_ = enclosing_execution;
                    return false;
                }
                const auto boolean = coerce_condition_boolean(
                    *condition, condition_offset, "WFC0035", "Do condition must be Boolean");
                if (!boolean.has_value()) {
                    execute_ = enclosing_execution;
                    return false;
                }
                continuation_offset = offset_;
                if (exit_do_requested) {
                    exit_do_requested_ = false;
                    continue_loop = false;
                } else if (exit_for_requested) {
                    continue_loop = false;
                } else {
                    continue_loop = enclosing_execution && (until ? !*boolean : *boolean);
                }
            } else {
                continuation_offset = offset_;
                if (exit_do_requested) {
                    exit_do_requested_ = false;
                    continue_loop = false;
                } else if (exit_for_requested) {
                    continue_loop = false;
                } else {
                    // No condition ever ends this loop on its own; only
                    // `enclosing_execution` (a dead branch parses the body
                    // once, no repeat -- matching every other loop kind's
                    // own dry-run behavior) or `Exit Do`/`Exit For` above
                    // can stop it.
                    continue_loop = enclosing_execution;
                }
            }
        } while (continue_loop);

        offset_ = continuation_offset;
        execute_ = enclosing_execution;
        return true;
    }

    [[nodiscard]] bool parse_do_body(std::size_t& continuation_offset) {
        const auto body_start = offset_;
        while (true) {
            skip_program_leading_trivia();
            if (at_end()) {
                set_error("WFC0037", "expected Loop", offset_);
                return false;
            }
            if (consume_keyword("loop")) {
                continuation_offset = offset_;
                return true;
            }

            const auto statement_offset = offset_;
            const bool enclosing_declaration_permission = allow_declarations_;
            allow_declarations_ = false;
            const bool parsed_statement = parse_statement();
            const bool consumed_statement_end = parsed_statement && consume_statement_end();
            allow_declarations_ = enclosing_declaration_permission;
            if (!parsed_statement && take_local_jump(body_start, statement_offset)) {
                continue;
            }
            if (!parsed_statement || !consumed_statement_end) {
                return false;
            }
            if (control_exit_requested()) {
                execute_ = false;
            }
        }
    }

    [[nodiscard]] bool parse_for_statement() {
        const bool enclosing_execution = execute_;
        skip_horizontal_whitespace();
        const auto variable_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value()) {
            set_error("WFC0043", "expected For control variable", variable_offset);
            return false;
        }
        const auto variable = find_variable(*identifier);
        if (variable.value == nullptr) {
            set_error("WFC0015", "undeclared variable", variable_offset);
            return false;
        }
        if (!type_character_matches(*variable.value, type_character, variable_offset)) {
            return false;
        }
        const bool is_variant_variable = variable.scope->variant_variables.contains(*identifier);
        enum class Slot { integer, int16, byte, floating_double, floating_single };
        Slot slot{};
        if (std::holds_alternative<Integer>(*variable.value)) {
            slot = Slot::integer;
        } else if (std::holds_alternative<Int16>(*variable.value)) {
            slot = Slot::int16;
        } else if (std::holds_alternative<Byte>(*variable.value)) {
            slot = Slot::byte;
        } else if (std::holds_alternative<double>(*variable.value)) {
            slot = Slot::floating_double;
        } else if (std::holds_alternative<float>(*variable.value)) {
            slot = Slot::floating_single;
        } else if (is_variant_variable) {
            slot = Slot::integer;  // refined from the bounds below
        } else {
            set_error("WFC0045", "For control variable must be numeric", variable_offset);
            return false;
        }

        skip_horizontal_whitespace();
        if (!consume('=')) {
            set_error("WFC0014", "expected assignment operator", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        auto start_value = parse_expression();
        if (!start_value.has_value()) {
            return false;
        }
        skip_horizontal_whitespace();
        if (!consume_keyword("to")) {
            set_error("WFC0044", "expected To", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        auto end_value = parse_expression();
        if (!end_value.has_value()) {
            return false;
        }
        Value step_value{Integer{1}};
        skip_horizontal_whitespace();
        if (consume_keyword("step")) {
            skip_horizontal_whitespace();
            auto parsed_step = parse_expression();
            if (!parsed_step.has_value()) {
                return false;
            }
            step_value = std::move(*parsed_step);
        }
        const auto numeric = [this](const Value& value) -> std::optional<double> {
            if (const auto* v = std::get_if<Integer>(&value)) return static_cast<double>(*v);
            if (const auto* v = std::get_if<Int16>(&value)) return static_cast<double>(*v);
            if (const auto* v = std::get_if<Byte>(&value)) return static_cast<double>(*v);
            if (const auto* v = std::get_if<double>(&value)) return *v;
            if (const auto* v = std::get_if<float>(&value)) return static_cast<double>(*v);
            if (std::holds_alternative<Currency>(value) || std::holds_alternative<Decimal>(value)) {
                return as_double(value);
            }
            if (const auto* v = std::get_if<DateValue>(&value)) return v->serial;
            if (std::holds_alternative<Empty>(value)) return 0.0;
            if (const auto* v = std::get_if<bool>(&value)) return *v ? -1.0 : 0.0;
            if (!execute_) return 0.0;  // a placeholder operand of a not-taken branch
            return std::nullopt;
        };
        const auto start_number = numeric(*start_value);
        const auto end_number = numeric(*end_value);
        const auto step_number = numeric(step_value);
        if (!start_number || !end_number || !step_number) {
            set_error("WFC0045", "For bounds and Step must be numeric", variable_offset);
            return false;
        }
        if (is_variant_variable) {
            const bool any_floating = std::holds_alternative<double>(*start_value) ||
                std::holds_alternative<float>(*start_value) ||
                std::holds_alternative<double>(*end_value) ||
                std::holds_alternative<float>(*end_value) ||
                std::holds_alternative<double>(step_value) ||
                std::holds_alternative<float>(step_value);
            slot = any_floating ? Slot::floating_double : Slot::integer;
        }
        if (*step_number == 0.0) {
            set_error("WFC0047", "For Step cannot be zero", variable_offset);
            return false;
        }
        if (!consume_loop_header_end()) {
            return false;
        }

        const bool floating = slot == Slot::floating_double || slot == Slot::floating_single;
        std::int64_t whole_min = std::numeric_limits<Integer>::min();
        std::int64_t whole_max = std::numeric_limits<Integer>::max();
        if (slot == Slot::int16) {
            whole_min = std::numeric_limits<Int16>::min();
            whole_max = std::numeric_limits<Int16>::max();
        } else if (slot == Slot::byte) {
            whole_min = 0;
            whole_max = 255;
        }
        const auto round_whole = [](const double number) {
            return static_cast<std::int64_t>(std::nearbyint(number));
        };
        const auto store = [&](const double number) {
            switch (slot) {
            case Slot::integer: *variable.value = static_cast<Integer>(round_whole(number)); break;
            case Slot::int16: *variable.value = static_cast<Int16>(round_whole(number)); break;
            case Slot::byte: *variable.value = static_cast<Byte>(round_whole(number)); break;
            case Slot::floating_double: *variable.value = number; break;
            case Slot::floating_single: *variable.value = static_cast<float>(number); break;
            }
        };
        const auto read = [&]() -> double {
            const auto current = numeric(*variable.value);
            return current.has_value() ? *current : 0.0;
        };
        double limit = *end_number;
        double step = *step_number;
        double current_value = *start_number;
        if (!floating) {
            limit = static_cast<double>(round_whole(limit));
            step = static_cast<double>(round_whole(step));
            current_value = static_cast<double>(round_whole(current_value));
            if (step == 0.0) {
                set_error("WFC0047", "For Step cannot be zero", variable_offset);
                return false;
            }
            if (enclosing_execution &&
                (current_value < static_cast<double>(whole_min) ||
                 current_value > static_cast<double>(whole_max))) {
                set_error("WFC0009", "numeric overflow", variable_offset);
                return false;
            }
        } else if (slot == Slot::floating_single) {
            limit = static_cast<float>(limit);
            step = static_cast<float>(step);
            current_value = static_cast<float>(current_value);
        }
        const auto should_continue = [&](const double current) {
            return step > 0.0 ? current <= limit : current >= limit;
        };

        const auto body_offset = offset_;
        std::size_t continuation_offset{};
        if (enclosing_execution) {
            store(current_value);
        }
        bool continue_loop = enclosing_execution && should_continue(current_value);
        if (!continue_loop) {
            execute_ = false;
            ++for_depth_;
            const bool parsed_body = parse_for_body(*identifier, continuation_offset);
            --for_depth_;
            execute_ = enclosing_execution;
            return parsed_body;
        }

        while (continue_loop) {
            store(current_value);
            offset_ = body_offset;
            execute_ = enclosing_execution;
            ++for_depth_;
            const bool parsed_body = parse_for_body(*identifier, continuation_offset);
            --for_depth_;
            if (!parsed_body) {
                execute_ = enclosing_execution;
                return false;
            }
            if (exit_for_requested_) {
                exit_for_requested_ = false;
                offset_ = continuation_offset;
                execute_ = enclosing_execution;
                return true;
            }
            if (exit_do_requested_) {
                offset_ = continuation_offset;
                execute_ = false;
                return true;
            }

            // The body may have assigned the control variable.
            double next = read() + step;
            if (slot == Slot::floating_single) {
                next = static_cast<float>(next);
            }
            if (!floating && (next < static_cast<double>(whole_min) ||
                              next > static_cast<double>(whole_max))) {
                set_error("WFC0047", "For control variable overflow", variable_offset);
                execute_ = enclosing_execution;
                return false;
            }
            current_value = next;
            continue_loop = should_continue(current_value);
        }

        store(current_value);
        offset_ = continuation_offset;
        execute_ = enclosing_execution;
        return true;
    }

    [[nodiscard]] bool parse_for_body(
        const std::string_view identifier,
        std::size_t& continuation_offset) {
        pending_next_comma_ = false;
        const auto body_start = offset_;
        while (true) {
            if (pending_next_comma_) {
                // REQ-0248: the nested loop's `Next j, i` left `, i` for us.
                skip_horizontal_whitespace();
                if (consume(',')) {
                    pending_next_comma_ = false;
                    skip_horizontal_whitespace();
                    const auto name_offset = offset_;
                    char name_type_character{};
                    auto name = parse_identifier(&name_type_character);
                    if (!name.has_value() || *name != identifier) {
                        set_error("WFC0049", "Next variable does not match For", name_offset);
                        return false;
                    }
                    skip_horizontal_whitespace();
                    if (!at_end() && current() == ',') {
                        pending_next_comma_ = true;
                    }
                    continuation_offset = offset_;
                    return true;
                }
            }
            skip_program_leading_trivia();
            if (at_end()) {
                set_error("WFC0046", "expected Next", offset_);
                return false;
            }
            if (consume_keyword("next")) {
                skip_horizontal_whitespace();
                const auto next_identifier_offset = offset_;
                char next_type_character{};
                auto next_identifier = parse_identifier(&next_type_character);
                if (next_identifier.has_value() && *next_identifier != identifier) {
                    set_error("WFC0049", "Next variable does not match For", next_identifier_offset);
                    return false;
                }
                if (next_identifier.has_value()) {
                    const auto variable = find_variable(*next_identifier);
                    if (variable.value == nullptr ||
                        !type_character_matches(
                            *variable.value, next_type_character, next_identifier_offset)) {
                        return false;
                    }
                }
                skip_horizontal_whitespace();
                if (!at_end() && current() == ',') {
                    pending_next_comma_ = true;
                }
                continuation_offset = offset_;
                return true;
            }

            const auto statement_offset = offset_;
            const bool enclosing_declaration_permission = allow_declarations_;
            allow_declarations_ = false;
            const bool parsed_statement = parse_statement();
            const bool consumed_statement_end = parsed_statement && consume_statement_end();
            allow_declarations_ = enclosing_declaration_permission;
            if (!parsed_statement && take_local_jump(body_start, statement_offset)) {
                continue;
            }
            if (!parsed_statement || !consumed_statement_end) {
                return false;
            }
            if (control_exit_requested()) {
                execute_ = false;
            }
        }
    }

    // `For Each identifier In arrayExpr ... Next [identifier]` (REQ-0209).
    // Only an array is an iterable collection in this evaluator (no other
    // collection type exists yet). The control variable must already be
    // declared: a Variant control variable retypes to each element in
    // turn, the same as any Variant assignment; a fixed-type one requires
    // the array's element type to match exactly, using the same
    // widening/narrowing and type-mismatch rules as an ordinary scalar
    // assignment. Iterates over a snapshot of the array taken once at loop
    // entry, so a `ReDim` inside the loop body cannot affect the ongoing
    // iteration (a disclosed simplification: real VB6 disallows `ReDim`ing
    // an array that is the subject of an active `For Each` at all). An
    // unallocated dynamic array (REQ-0207) iterates zero times, the same as
    // any other empty array -- reasoned by analogy with a zero-length
    // `ParamArray`, not independently verified against the reference
    // runtime for this specific case (see Scope).
    [[nodiscard]] bool parse_for_each_statement() {
        const bool enclosing_execution = execute_;
        skip_horizontal_whitespace();
        const auto variable_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value()) {
            set_error("WFC0043", "expected For Each control variable", variable_offset);
            return false;
        }
        const auto variable = find_variable(*identifier);
        if (variable.value == nullptr) {
            set_error("WFC0015", "undeclared variable", variable_offset);
            return false;
        }
        if (!type_character_matches(*variable.value, type_character, variable_offset)) {
            return false;
        }
        skip_horizontal_whitespace();
        if (!consume_keyword("in")) {
            set_error("WFC0147", "expected In after For Each control variable", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        const auto collection_offset = offset_;
        auto collection_value = parse_expression();
        if (!collection_value.has_value()) {
            return false;
        }
        // REQ-0243: For Each over a Collection (or any class exposing a
        // `WfcItems` method) iterates the array that method returns.
        if (execute_) {
            if (const auto* holder = std::get_if<ObjectInstance>(&*collection_value)) {
                auto class_iterator = class_definitions_.find(holder->data->class_name);
                // A class exposing a `NewEnum` method (VB_UserMemId -4): iterate what it returns.
                if (class_iterator != class_definitions_.end() &&
                    !class_iterator->second.methods.contains("wfcitems") &&
                    class_iterator->second.methods.contains("newenum")) {
                    auto enumerator = call_class_method(
                        *holder->data, class_iterator->second, "newenum", collection_offset,
                        /*require_function=*/true);
                    if (!enumerator.has_value()) {
                        return false;
                    }
                    collection_value = std::move(enumerator);
                    holder = std::get_if<ObjectInstance>(&*collection_value);
                    class_iterator = holder != nullptr
                        ? class_definitions_.find(holder->data->class_name)
                        : class_definitions_.end();
                }
                if (holder != nullptr && class_iterator != class_definitions_.end() &&
                    class_iterator->second.methods.contains("wfcitems")) {
                    auto items = call_class_method(
                        *holder->data, class_iterator->second, "wfcitems", collection_offset,
                        /*require_function=*/true);
                    if (!items.has_value()) {
                        return false;
                    }
                    collection_value = std::move(items);
                }
            }
        }
        const auto* array = std::get_if<ArrayValue>(&*collection_value);
        if (array == nullptr && !execute_) {
            static const ArrayValue empty_array{};  // a placeholder in a not-taken branch
            array = &empty_array;
        }
        if (array == nullptr) {
            set_error("WFC0147", "For Each requires an array", collection_offset);
            return false;
        }
        if (!consume_loop_header_end()) {
            return false;
        }

        const bool target_is_variant = variable.scope->variant_variables.contains(*identifier);
        const auto count = array->elements.size();
        const auto assign_element = [&](const std::size_t index) -> bool {
            Value element_value = array->elements[index];
            if (variable.scope->object_variables.contains(*identifier) &&
                is_object_reference(element_value)) {
                *variable.value = std::move(element_value);
                return true;
            }
            if (target_is_variant) {
                *variable.value = std::move(element_value);
                return true;
            }
            if (!coerce_numeric_value(element_value, variable.value->index(), variable_offset)) {
                return false;
            }
            if (element_value.index() != variable.value->index()) {
                set_error("WFC0016", "assignment type mismatch", variable_offset);
                return false;
            }
            *variable.value = std::move(element_value);
            return true;
        };

        const auto body_offset = offset_;
        std::size_t continuation_offset{};
        std::size_t index = 0;
        bool continue_loop = enclosing_execution && index < count;
        if (continue_loop && !assign_element(index)) {
            execute_ = enclosing_execution;
            return false;
        }
        if (!continue_loop) {
            execute_ = false;
            ++for_depth_;
            const bool parsed_body = parse_for_body(*identifier, continuation_offset);
            --for_depth_;
            execute_ = enclosing_execution;
            return parsed_body;
        }

        while (continue_loop) {
            offset_ = body_offset;
            execute_ = enclosing_execution;
            ++for_depth_;
            const bool parsed_body = parse_for_body(*identifier, continuation_offset);
            --for_depth_;
            if (!parsed_body) {
                execute_ = enclosing_execution;
                return false;
            }
            if (exit_for_requested_) {
                exit_for_requested_ = false;
                offset_ = continuation_offset;
                execute_ = enclosing_execution;
                return true;
            }
            if (exit_do_requested_) {
                offset_ = continuation_offset;
                execute_ = false;
                return true;
            }
            ++index;
            continue_loop = index < count;
            if (continue_loop && !assign_element(index)) {
                execute_ = enclosing_execution;
                return false;
            }
        }

        offset_ = continuation_offset;
        execute_ = enclosing_execution;
        return true;
    }

    [[nodiscard]] bool parse_select_statement() {
        const bool enclosing_execution = execute_;
        skip_horizontal_whitespace();
        if (!consume_keyword("case")) {
            set_error("WFC0054", "expected Case after Select", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        const auto selector_offset = offset_;
        auto selector = parse_expression();
        if (!selector.has_value()) {
            return false;
        }
        if (!consume_loop_header_end()) {
            return false;
        }

        bool has_case{};
        bool has_else{};
        bool branch_selected{};
        execute_ = false;
        while (true) {
            skip_program_leading_trivia();
            if (at_end()) {
                execute_ = enclosing_execution;
                set_error("WFC0054", "expected End Select", offset_);
                return false;
            }
            if (consume_keyword("case")) {
                const auto case_offset = offset_ - 4U;
                skip_horizontal_whitespace();
                if (consume_keyword("else")) {
                    if (has_else) {
                        execute_ = enclosing_execution;
                        set_error("WFC0056", "duplicate Case Else", case_offset);
                        return false;
                    }
                    has_else = true;
                    has_case = true;
                    if (!consume_case_line_end()) {
                        execute_ = enclosing_execution;
                        return false;
                    }
                    execute_ = enclosing_execution && !branch_selected;
                    branch_selected = true;
                    continue;
                }
                if (has_else) {
                    execute_ = enclosing_execution;
                    set_error("WFC0057", "Case is not permitted after Case Else", case_offset);
                    return false;
                }

                bool case_matches{};
                while (true) {
                    const bool evaluate_case =
                        enclosing_execution && !branch_selected && !case_matches;
                    execute_ = evaluate_case;
                    const auto value_offset = offset_;
                    std::string relational_operator;
                    if (consume_keyword("is")) {
                        skip_horizontal_whitespace();
                        const auto operator_offset = offset_;
                        if (consume('<')) {
                            relational_operator = "<";
                            if (consume('=')) {
                                relational_operator = "<=";
                            } else if (consume('>')) {
                                relational_operator = "<>";
                            }
                        } else if (consume('>')) {
                            relational_operator = consume('=') ? ">=" : ">";
                        } else if (consume('=')) {
                            relational_operator = "=";
                        } else {
                            execute_ = enclosing_execution;
                            set_error(
                                "WFC0061",
                                "expected relational operator after Case Is",
                                operator_offset);
                            return false;
                        }
                        skip_horizontal_whitespace();
                    }
                    if (at_end() || current() == ',' || current() == '\r' ||
                        current() == '\n') {
                        execute_ = enclosing_execution;
                        set_error("WFC0059", "expected Case value", value_offset);
                        return false;
                    }
                    auto case_value = parse_expression();
                    if (!case_value.has_value()) {
                        execute_ = enclosing_execution;
                        return false;
                    }
                    const bool both_numeric = is_number(*case_value) && is_number(*selector);
                    if (case_value->index() != selector->index() && !both_numeric &&
                        enclosing_execution && !std::holds_alternative<Empty>(*selector) &&
                        !std::holds_alternative<Empty>(*case_value)) {
                        execute_ = enclosing_execution;
                        set_error("WFC0053", "Case value must match selector type", value_offset);
                        return false;
                    }
                    skip_horizontal_whitespace();
                    bool item_matches{};
                    if (!relational_operator.empty()) {
                        auto comparison = compare(
                            *selector,
                            *case_value,
                            relational_operator,
                            value_offset);
                        if (!comparison.has_value()) {
                            execute_ = enclosing_execution;
                            return false;
                        }
                        // A Null selector/case value makes `compare` return
                        // Null itself (three-valued logic), which is never a
                        // match, matching Select Case Null never selecting
                        // any Case clause in real VB6.
                        const auto* comparison_boolean = std::get_if<bool>(&*comparison);
                        item_matches = comparison_boolean != nullptr && *comparison_boolean;
                    } else if (consume_keyword("to")) {
                        skip_horizontal_whitespace();
                        const auto upper_offset = offset_;
                        auto upper_value = parse_expression();
                        if (!upper_value.has_value()) {
                            execute_ = enclosing_execution;
                            return false;
                        }
                        if ((upper_value->index() != selector->index() &&
                             !(is_number(*upper_value) && is_number(*selector))) ||
                            std::holds_alternative<bool>(*selector) ||
                            (!is_number(*selector) &&
                             !std::holds_alternative<std::string>(*selector))) {
                            execute_ = enclosing_execution;
                            set_error(
                                "WFC0060",
                                "Case range requires same-type Long or String values",
                                upper_offset);
                            return false;
                        }
                        if (is_number(*selector)) {
                            const double selected_number = as_double(*selector);
                            item_matches =
                                as_double(*case_value) <= selected_number &&
                                selected_number <= as_double(*upper_value);
                        } else {
                            const auto& selected_string = std::get<std::string>(*selector);
                            item_matches =
                                compare_strings(
                                    std::get<std::string>(*case_value),
                                    selected_string) <= 0 &&
                                compare_strings(
                                    selected_string,
                                    std::get<std::string>(*upper_value)) <= 0;
                        }
                        skip_horizontal_whitespace();
                    } else {
                        item_matches = both_numeric
                            ? as_double(*case_value) == as_double(*selector)
                            : values_equal(*case_value, *selector);
                    }
                    case_matches = case_matches || item_matches;
                    if (!consume(',')) {
                        break;
                    }
                    skip_horizontal_whitespace();
                }
                if (!consume_case_line_end()) {
                    execute_ = enclosing_execution;
                    return false;
                }
                const bool select_branch = !branch_selected && case_matches;
                branch_selected = branch_selected || case_matches;
                has_case = true;
                execute_ = enclosing_execution && select_branch;
                continue;
            }
            if (consume_keyword("end")) {
                skip_horizontal_whitespace();
                if (!consume_keyword("select")) {
                    execute_ = enclosing_execution;
                    set_error("WFC0055", "expected Select after End", offset_);
                    return false;
                }
                execute_ = control_exit_requested() ? false : enclosing_execution;
                return true;
            }
            if (!has_case) {
                execute_ = enclosing_execution;
                set_error("WFC0054", "expected Case or End Select", selector_offset);
                return false;
            }

            const bool enclosing_declaration_permission = allow_declarations_;
            allow_declarations_ = false;
            const bool parsed_statement = parse_statement();
            const bool consumed_statement_end = parsed_statement && consume_statement_end();
            allow_declarations_ = enclosing_declaration_permission;
            if (!parsed_statement || !consumed_statement_end) {
                execute_ = enclosing_execution;
                return false;
            }
            if (control_exit_requested()) {
                execute_ = false;
            }
        }
    }

    // Bare `Name [args]` call statement (see the REQ-0217 notes where this is
    // used); nullopt when the statement is not such a call.
    struct ParenGroup {
        bool ends_statement{};  // `( ... )` is balanced and ends the statement
        bool is_list{};         // empty, or has a top-level comma
    };

    [[nodiscard]] ParenGroup scan_statement_paren_group(const std::size_t open_offset) const {
        ParenGroup group;
        std::size_t depth = 0;
        bool in_string = false;
        bool top_level_comma = false;
        bool empty = true;
        bool balanced = false;
        std::size_t look = open_offset;
        for (; look < source_.size(); ++look) {
            const char c = source_[look];
            if (c == '"') in_string = !in_string;
            if (in_string) continue;
            if (c == '(') {
                ++depth;
            } else if (c == ')') {
                if (--depth == 0) { balanced = true; ++look; break; }
            } else if (c == ',' && depth == 1) {
                top_level_comma = true;
            } else if (c == '\r' || c == '\n') {
                break;
            } else if (depth == 1 && c != ' ' && c != '\t') {
                empty = false;
            }
        }
        if (balanced) {
            while (look < source_.size() && (source_[look] == ' ' || source_[look] == '\t')) ++look;
            group.ends_statement = look >= source_.size() || source_[look] == '\r' ||
                source_[look] == '\n' || source_[look] == ':' || source_[look] == '\'';
            group.is_list = top_level_comma || empty;
        }
        return group;
    }

    [[nodiscard]] std::optional<bool> parse_bare_call(
        const std::string& identifier, const std::size_t identifier_offset,
        const char type_character, const bool has_let) {
        if (!has_let && type_character == '\0' && find_variable(identifier).value == nullptr) {
            const auto saved_offset = offset_;
            skip_horizontal_whitespace();
            const bool bare_statement_end = at_end() || current() == '\r' || current() == '\n' ||
                current() == ':' || current() == '\'';
            const bool arguments_follow = !bare_statement_end && current() != '=' &&
                current() != '(' && current() != '.';
            // `Name (arg)` / `Name(a, b)` as a statement: a parenthesized
            // list ending the statement. One argument is passed by value
            // (VB evaluates `(x)` as an expression); several are an
            // ordinary argument list.
            bool parenthesized_call = false;
            bool parenthesized_list = false;
            if (!bare_statement_end && !at_end() && current() == '(' &&
                (procedures_.contains(identifier) ||
                 (current_instance() != nullptr && current_class_def() != nullptr &&
                  current_class_def()->methods.contains(identifier)))) {
                const auto group = scan_statement_paren_group(offset_);
                parenthesized_call = group.ends_statement;
                parenthesized_list = group.is_list;
            }
            offset_ = saved_offset;
            if (parenthesized_call) {
                bare_call_arguments_ = !parenthesized_list;
                std::optional<Value> result;
                if (procedures_.contains(identifier)) {
                    result = call_procedure(identifier, identifier_offset, false);
                } else {
                    result = call_class_method(
                        *current_instance(), *current_class_def(), identifier, identifier_offset,
                        false);
                }
                bare_call_arguments_ = false;
                return result.has_value();
            }
            if (bare_statement_end || arguments_follow) {
                bare_call_arguments_ = arguments_follow;
                if (procedures_.contains(identifier)) {
                    const auto result = call_procedure(identifier, identifier_offset, false);
                    bare_call_arguments_ = false;
                    return result.has_value();
                }
                if (auto* const instance = current_instance()) {
                    if (const auto* const class_def = current_class_def()) {
                        if (class_def->methods.contains(identifier)) {
                            const auto result = call_class_method(
                                *instance, *class_def, identifier, identifier_offset, false);
                            bare_call_arguments_ = false;
                            return result.has_value();
                        }
                    }
                }
                bare_call_arguments_ = false;
            }
        }
        return std::nullopt;
    }

    // One statement of a single-line `If` branch; a runtime error in it is
    // handled by the frame's `On Error` mode like any other statement.
    [[nodiscard]] bool parse_inline_statement() {
        while (true) {
            const auto start = offset_;
            const bool entry_execute = execute_;
            if (parse_inline_statement_core()) {
                return true;
            }
            if (!entry_execute || jump_pending_) {
                return false;
            }
            execute_ = entry_execute;
            if (!recover_runtime_error(start)) {
                return false;
            }
            if (!retry_statement_) {
                return true;
            }
            retry_statement_ = false;
            offset_ = start;
        }
    }

    [[nodiscard]] bool parse_inline_statement_core() {
        skip_horizontal_whitespace();
        const auto statement_offset = offset_;
        if (at_end() || current() == '\r' || current() == '\n' || current() == ':' ||
            consume_keyword("else")) {
            offset_ = statement_offset;
            set_error("WFC0023", "expected Print or assignment branch", statement_offset);
            return false;
        }
        if (consume_keyword("print")) {
            return parse_print_statement();
        }
        if (consume_keyword("set")) {
            return parse_set_statement();
        }
        if (consume_keyword("call")) {
            return parse_call_statement();
        }
        if (consume_keyword("exit")) {
            return parse_exit_statement(statement_offset);
        }
        if (const auto handled = parse_error_handling_statement(statement_offset, false)) {
            return *handled;
        }

        {
            // Other simple statements run through the ordinary dispatcher
            // (`If a Then If b Then ...`, `If a Then Close #1`, `GoTo`...).
            const auto probe = offset_;
            char probe_type_character{};
            const auto word = parse_identifier(&probe_type_character);
            offset_ = probe;
            static const std::set<std::string, std::less<>> delegated = {
                "if", "goto", "gosub", "return", "resume", "redim", "erase", "open", "close",
                "write", "input", "line", "get", "put", "seek", "kill", "name", "mkdir", "rmdir",
                "chdir", "randomize", "lset", "rset", "end", "stop", "raiseevent", "mid",
                "savesetting", "deletesetting", "chdrive", "unlock", "lock", "reset", "load",
                "unload", "beep", "doevents", "date", "time", "dim", "static", "const", "error", "debug"};
            if (word.has_value() &&
                (probe_type_character == '\0' || (probe_type_character == '$' && *word == "mid")) &&
                delegated.contains(*word) && *word != "date" && *word != "time") {
                return parse_statement_core();
            }
        }
        const bool has_let = consume_keyword("let");
        if (has_let) {
            skip_horizontal_whitespace();
        }
        const auto inline_identifier_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value() || is_reserved_identifier(*identifier)) {
            set_error("WFC0023", "expected Print or assignment branch", statement_offset);
            return false;
        }
        if (const auto handled = parse_bare_call(
                *identifier, inline_identifier_offset, type_character, has_let)) {
            return *handled;
        }
        return parse_assignment_or_array_element(std::move(*identifier), type_character);
    }

    // `Static name [As Type]` inside a Sub/Function/Property body
    // (REQ-0206): unlike an ordinary local `Dim`, the variable's value
    // survives from one call to the next. Storage lives on the currently
    // executing procedure's own ProcedureDef (`current_procedure_def_->
    // statics`, found once by scan_procedures/scan_class_body and never
    // moved afterward) rather than in this call's transient Scope;
    // invoke_definition copies the frame's final value back into that
    // persistent storage just before discarding the frame. Scoped to
    // scalar/Variant types only for this first increment -- no arrays, no
    // `Object`/class types (a Static array or object reference would need
    // the same persistent-storage treatment `ArrayValue`/`ObjectInstance`
    // do not yet have outside a Scope's ordinary variables map).
    [[nodiscard]] bool parse_static_declaration(const std::size_t statement_offset) {
        while (true) {
            if (!parse_single_static_declaration(statement_offset)) {
                return false;
            }
            skip_horizontal_whitespace();
            if (at_end() || current() != ',') {
                return true;
            }
            advance();
        }
    }

    [[nodiscard]] bool parse_single_static_declaration(const std::size_t statement_offset) {
        if (current_procedure_def_ == nullptr) {
            set_error(
                "WFC0144", "Static is only valid inside a Sub, Function, or Property",
                statement_offset);
            return false;
        }
        skip_horizontal_whitespace();
        const auto identifier_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value()) {
            set_error("WFC0011", "expected variable name", identifier_offset);
            return false;
        }
        if (is_reserved_identifier(*identifier)) {
            set_error(
                "WFC0017", "reserved keyword cannot be a variable name", identifier_offset);
            return false;
        }
        if (!validate_type_character(type_character, identifier_offset)) {
            return false;
        }
        skip_horizontal_whitespace();

        // REQ-0231: `Static arr(<bounds>) As Type` -- always fixed-size,
        // 1-D or multi-dimensional (REQ-0201/REQ-0210's own bound
        // grammar, reused via `parse_fixed_array_bounds`). Unlike `Dim`,
        // `Static` has no dynamic (bound-less/comma-only) array form at
        // all: real VB6 requires a `Static` array's bounds to be fixed at
        // declaration time, with no `ReDim` counterpart ever possible for
        // one, so `Static arr()` is rejected outright rather than parsed
        // as an unallocated dynamic array the way `Dim arr()` is.
        bool is_array = false;
        Integer array_lower = 0;
        Integer array_upper = 0;
        std::vector<std::pair<Integer, Integer>> array_dimensions;
        if (!at_end() && current() == '(') {
            is_array = true;
            advance();
            skip_horizontal_whitespace();
            if (!at_end() && current() == ')') {
                set_error(
                    "WFC0149",
                    "a Static array must have fixed bounds (Static arr(n) As Type)", offset_);
                return false;
            }
            auto parsed_dimensions = parse_fixed_array_bounds(identifier_offset);
            if (!parsed_dimensions.has_value()) {
                return false;
            }
            array_dimensions = std::move(*parsed_dimensions);
            if (array_dimensions.size() == 1U) {
                array_lower = array_dimensions.front().first;
                array_upper = array_dimensions.front().second;
                array_dimensions.clear();
            }
            skip_horizontal_whitespace();
        }

        Value element_default;
        bool is_variant = false;
        bool is_object = false;
        std::string declared_class_name;
        if (type_character != '\0') {
            if (consume_keyword("as")) {
                set_error(
                    "WFC0012", "type-declaration character cannot be combined with As",
                    offset_);
                return false;
            }
            if (type_character == '$') {
                element_default = std::string{};
            } else if (type_character == '#') {
                element_default = 0.0;
            } else if (type_character == '!') {
                element_default = 0.0f;
            } else if (type_character == '@') {
                element_default = Currency{};
            } else if (type_character == '%') {
                element_default = Int16{};
            } else {
                element_default = Integer{};
            }
        } else if (!consume_keyword("as")) {
            if (at_end() || current() == '\r' || current() == '\n' || current() == ':' ||
                current() == '\'') {
                element_default = Empty{};
                is_variant = true;
            } else {
                set_error(
                    "WFC0012",
                    "expected As Integer, As Long, As Double, As Single, As Currency, As "
                    "String, As Boolean, As Object, or As Variant",
                    offset_);
                return false;
            }
        } else {
            skip_horizontal_whitespace();
            const auto type_offset = offset_;
            // REQ-0231: `Static o As Object`/`As SomeClassName`, reusing
            // the same class-name resolver a class-typed field/return
            // type/parameter already uses (REQ-0203/REQ-0228), extending
            // `Static` beyond the fixed-scalar/`Variant` forms it
            // originally supported alone (REQ-0206's Scope).
            const auto type_result = parse_scalar_object_or_class_type();
            if (!type_result.has_value()) {
                set_error(
                    "WFC0012",
                    "expected As Integer, As Long, As Double, As Single, As Currency, As "
                    "String, As Boolean, As Object, or As Variant",
                    type_offset);
                return false;
            }
            if (type_result->is_object) {
                element_default = Nothing{};
                is_object = true;
                declared_class_name = type_result->class_name;
            } else if (type_result->is_variant) {
                element_default = Empty{};
                is_variant = true;
            } else {
                element_default = zero_value_for_index(type_result->type_index);
            }
        }

        if (is_array && (is_variant || is_object)) {
            set_error(
                "WFC0149",
                "a Static array's element type must be a fixed scalar type, not Variant or "
                "Object",
                identifier_offset);
            return false;
        }

        Value initial_value;
        if (is_array) {
            const auto element_type_index = element_default.index();
            if (!array_dimensions.empty()) {
                std::size_t total_size = 1U;
                for (const auto& dimension : array_dimensions) {
                    total_size *=
                        static_cast<std::size_t>(dimension.second - dimension.first) + 1U;
                }
                initial_value = ArrayValue{
                    std::vector<Value>(total_size, element_default), /*lower_bound=*/0,
                    /*is_dynamic=*/false, /*is_allocated=*/true, element_type_index,
                    array_dimensions};
            } else {
                const auto size = static_cast<std::size_t>(array_upper - array_lower) + 1U;
                initial_value = ArrayValue{
                    std::vector<Value>(size, std::move(element_default)), array_lower,
                    /*is_dynamic=*/false, /*is_allocated=*/true, element_type_index};
            }
        } else {
            initial_value = std::move(element_default);
        }

        if (current_scope().variables.contains(*identifier)) {
            if (!allow_declarations_) {
                return true;  // REQ-0271: re-executed declaration inside a block
            }
            set_error("WFC0013", "duplicate variable declaration", identifier_offset);
            return false;
        }
        auto& statics = current_instance() != nullptr
            ? current_instance()->static_scopes[current_procedure_def_]
            : current_procedure_def_->statics;
        if (!statics.variables.contains(*identifier)) {
            statics.variables.emplace(*identifier, std::move(initial_value));
            if (is_variant) {
                statics.variant_variables.insert(*identifier);
            }
            if (is_object) {
                statics.object_variables.insert(*identifier);
                if (!declared_class_name.empty()) {
                    statics.object_class_names.emplace(*identifier, declared_class_name);
                }
            }
        }
        current_scope().variables.emplace(*identifier, statics.variables.at(*identifier));
        if (is_variant) {
            current_scope().variant_variables.insert(*identifier);
        }
        if (is_object) {
            current_scope().object_variables.insert(*identifier);
            if (!declared_class_name.empty()) {
                current_scope().object_class_names.emplace(*identifier, declared_class_name);
            }
        }
        current_scope().static_variable_names.insert(*identifier);
        return true;
    }

    // Parses a comma-separated list of fixed-size array bounds (`<bound>`
    // or `<lower> To <upper>`, REQ-0201/REQ-0210), with the opening `(`
    // already consumed, through and including the closing `)`. Shared by
    // `Dim`'s own fixed-size array form (`parse_declaration`) and
    // `Static`'s array form (REQ-0231, `parse_static_declaration`) --
    // `Static` never has a dynamic (bound-less/comma-only) form at all
    // (real VB6 requires every `Static` array's bounds to be fixed at
    // declaration time, with no `ReDim` counterpart), so this covers the
    // one shape both need, without `parse_declaration`'s own additional
    // dynamic-array detection ahead of it.
    [[nodiscard]] std::optional<std::vector<std::pair<Integer, Integer>>>
    parse_fixed_array_bounds(const std::size_t identifier_offset) {
        std::vector<std::pair<Integer, Integer>> dimensions;
        while (true) {
            const auto first_offset = offset_;
            auto first_bound = parse_expression();
            if (!first_bound.has_value()) {
                return std::nullopt;
            }
            const auto first_long = coerce_long(*first_bound, first_offset);
            if (!first_long.has_value()) {
                return std::nullopt;
            }
            Integer dimension_lower = 0;
            Integer dimension_upper = 0;
            skip_horizontal_whitespace();
            if (consume_keyword("to")) {
                skip_horizontal_whitespace();
                const auto second_offset = offset_;
                auto second_bound = parse_expression();
                if (!second_bound.has_value()) {
                    return std::nullopt;
                }
                const auto second_long = coerce_long(*second_bound, second_offset);
                if (!second_long.has_value()) {
                    return std::nullopt;
                }
                dimension_lower = *first_long;
                dimension_upper = *second_long;
            } else {
                // REQ-0226: a bound-less dimension (`<bound>`, no `<lower>
                // To`) takes its lower bound from `Option Base` -- `0`
                // unless `Option Base 1` was declared for this module.
                dimension_lower = option_base_one_ ? 1 : 0;
                dimension_upper = *first_long;
            }
            if (dimension_lower > dimension_upper) {
                set_error(
                    "WFC0117", "array lower bound must not exceed the upper bound",
                    identifier_offset);
                return std::nullopt;
            }
            dimensions.emplace_back(dimension_lower, dimension_upper);
            skip_horizontal_whitespace();
            if (!at_end() && current() == ',') {
                advance();
                skip_horizontal_whitespace();
                continue;
            }
            break;
        }
        if (!consume(')')) {
            set_error("WFC0005", "expected closing parenthesis", offset_);
            return std::nullopt;
        }
        return dimensions;
    }

    // REQ-0248: `Dim a As Long, b As String` -- one declarator at a time.
    [[nodiscard]] bool parse_declaration() {
        while (true) {
            if (!parse_single_declaration()) {
                return false;
            }
            skip_horizontal_whitespace();
            if (at_end() || current() != ',') {
                return true;
            }
            advance();
        }
    }

    [[nodiscard]] bool parse_single_declaration() {
        udt_array_ = false;
        fixed_string_length_ = 0;
        skip_horizontal_whitespace();
        const auto identifier_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value()) {
            set_error("WFC0011", "expected variable name", identifier_offset);
            return false;
        }
        const bool repeat_in_block =
            !allow_declarations_ && current_scope().variables.contains(*identifier);
        if (is_reserved_identifier(*identifier)) {
            set_error("WFC0017", "reserved keyword cannot be a variable name", identifier_offset);
            return false;
        }
        if (!validate_type_character(type_character, identifier_offset)) {
            return false;
        }

        skip_horizontal_whitespace();
        bool is_array = false;
        bool is_dynamic_array = false;
        Integer array_lower = 0;
        Integer array_upper = 0;
        // Populated only when the declaration writes two or more
        // comma-separated bounds (a fixed-size multi-dimensional array,
        // REQ-0210); stays empty for the ordinary 1-D forms, which
        // continue to use array_lower/array_upper alone.
        std::vector<std::pair<Integer, Integer>> array_dimensions;
        // Only meaningful when `is_dynamic_array`: a comma-only
        // declaration's pre-declared dimension count (REQ-0219), e.g.
        // `Dim arr(,) As Type` is 2, `Dim arr(,,) As Type` is 3; stays 0
        // for the plain `Dim arr()` form, which leaves the count
        // unconstrained until the first `ReDim` decides it.
        std::size_t dynamic_dimension_count = 0;
        if (!at_end() && current() == '(') {
            is_array = true;
            advance();
            skip_horizontal_whitespace();
            if (!at_end() && current() == ')') {
                // `Dim identifier()` with no bound: a dynamic array,
                // unallocated until its first `ReDim` (REQ-0207). WFC0116
                // previously rejected this form outright; retired now that
                // dynamic arrays are supported.
                is_dynamic_array = true;
                advance();
                skip_horizontal_whitespace();
            } else {
                // `Dim identifier(,)`, `Dim identifier(,,)`, ... (REQ-0219):
                // a dynamic array whose dimension count is fixed in advance
                // (one more than the comma count), still unallocated until
                // its first `ReDim`. Falls through to the ordinary bound
                // parsing below when the parenthesized content is not
                // comma-only (the common case).
                const auto comma_check_offset = offset_;
                std::size_t comma_count = 0;
                while (!at_end() && current() == ',') {
                    advance();
                    skip_horizontal_whitespace();
                    ++comma_count;
                }
                if (comma_count > 0 && !at_end() && current() == ')') {
                    is_dynamic_array = true;
                    dynamic_dimension_count = comma_count + 1U;
                    advance();
                    skip_horizontal_whitespace();
                } else {
                    offset_ = comma_check_offset;
                }
            }
            if (is_dynamic_array) {
                // No bounds to parse; fall through to the shared `As Type`
                // handling below.
            } else {
                auto parsed_dimensions = parse_fixed_array_bounds(identifier_offset);
                if (!parsed_dimensions.has_value()) {
                    return false;
                }
                array_dimensions = std::move(*parsed_dimensions);
                if (array_dimensions.size() == 1U) {
                    array_lower = array_dimensions.front().first;
                    array_upper = array_dimensions.front().second;
                    array_dimensions.clear();
                }
            }
            skip_horizontal_whitespace();
        }

        Value element_default;
        bool is_variant = false;
        bool is_object = false;
        std::string declared_class_name;
        if (type_character != '\0') {
            if (consume_keyword("as")) {
                set_error("WFC0012", "type-declaration character cannot be combined with As", offset_);
                return false;
            }
            if (type_character == '$') {
                element_default = std::string{};
            } else if (type_character == '#') {
                element_default = 0.0;
            } else if (type_character == '!') {
                element_default = 0.0f;
            } else if (type_character == '@') {
                element_default = Currency{};
            } else if (type_character == '%') {
                element_default = Int16{};
            } else {
                element_default = Integer{};
            }
        } else if (!consume_keyword("as")) {
            // A bare `Dim x` with no As clause and no type-declaration
            // character implicitly declares a Variant, matching real VB6.
            // Arrays require an explicit element type in this evaluator
            // (Variant-element arrays are outside the current array scope).
            if (const auto default_type = default_type_for(*identifier);
                default_type.has_value() &&
                (at_end() || current() == '\r' || current() == '\n' || current() == ':' ||
                 current() == '\'' || current() == ',')) {
                element_default = zero_value_for_index(*default_type);
            } else if (at_end() || current() == '\r' || current() == '\n' ||
                current() == ':' || current() == '\'' || current() == ',') {
                element_default = Empty{};
                is_variant = true;
            } else {
                set_error(
                    "WFC0012",
                    "expected As Integer, As Long, As Double, As Single, As Currency, As "
                    "String, As Boolean, As Object, or As Variant",
                    offset_);
                return false;
            }
        } else {
            skip_horizontal_whitespace();
            bool eager_new = false;
            bool lazy_new = false;
            if (consume_keyword("long")) {
                element_default = Integer{};
            } else if (consume_keyword("integer")) {
                element_default = Int16{};
            } else if (consume_keyword("double")) {
                element_default = 0.0;
            } else if (consume_keyword("single")) {
                element_default = 0.0f;
            } else if (consume_keyword("date")) {
                element_default = DateValue{};
            } else if (consume_keyword("byte")) {
                element_default = Byte{};
            } else if (consume_keyword("decimal")) {
                element_default = Decimal{};
            } else if (consume_keyword("currency")) {
                element_default = Currency{};
            } else if (consume_keyword("string")) {
                element_default = std::string{};
                skip_horizontal_whitespace();
                if (!at_end() && current() == '*') {
                    advance();
                    skip_horizontal_whitespace();
                    const auto length_offset = offset_;
                    auto length = parse_expression();
                    if (!length.has_value()) {
                        return false;
                    }
                    const auto size_value = whole_value(*length);
                    const Integer* const size = size_value ? &*size_value : nullptr;
                    if (size == nullptr || *size < 1 || *size > 65526) {
                        set_error("WFC0012", "fixed String length must be 1 to 65526", length_offset);
                        return false;
                    }
                    fixed_string_length_ = static_cast<std::size_t>(*size);
                    element_default = std::string(fixed_string_length_, ' ');
                }
            } else if (consume_keyword("boolean")) {
                element_default = false;
            } else if (consume_keyword("object")) {
                element_default = Nothing{};
                is_object = true;
            } else if (consume_keyword("variant")) {
                element_default = Empty{};
                is_variant = true;
            } else if (!is_array && consume_keyword("new")) {
                // `As New ClassName` eagerly instantiates the class right
                // here (a documented simplification of VB6's lazy
                // auto-instantiation, which only creates the instance on
                // first use; see REQ-0203's Scope).
                skip_horizontal_whitespace();
                const auto class_name_offset = offset_;
                char class_type_character{};
                auto class_name = parse_identifier(&class_type_character);
                if (!class_name.has_value() || class_type_character != '\0' ||
                    !class_definitions_.contains(*class_name)) {
                    set_error("WFC0134", "unknown class name", class_name_offset);
                    return false;
                }
                declared_class_name = std::move(*class_name);
                is_object = true;
                if (is_udt_class(declared_class_name) || is_array) {
                    eager_new = true;
                } else {
                    lazy_new = true;
                    element_default = Nothing{};
                }
            } else {
                // A bare identifier here, if it names a known class, is a
                // fixed `As ClassName` declaration (initialized to
                // Nothing, like `As Object`, but Set-checked against this
                // specific class -- see parse_set_statement).
                const auto class_name_offset = offset_;
                const auto saved_offset = offset_;
                char class_type_character{};
                auto class_name = parse_identifier(&class_type_character);
                if (class_name.has_value() && class_type_character == '\0' &&
                    class_definitions_.contains(*class_name)) {
                    declared_class_name = std::move(*class_name);
                    element_default = Nothing{};
                    is_object = true;
                    if (is_udt_class(declared_class_name)) {
                        if (is_array) {
                            udt_array_ = true;
                        } else {
                            eager_new = true;
                        }
                    }
                } else {
                    offset_ = saved_offset;
                    set_error(
                        "WFC0012",
                        is_array
                            ? "expected As Integer, As Long, As Double, As Single, As Currency, "
                              "As String, As Boolean, As Object, As Variant, or a known class "
                              "name"
                            : "expected As Integer, As Long, As Double, As Single, As Currency, "
                              "As String, As Boolean, As Object, As Variant, or a known class "
                              "name",
                        class_name_offset);
                    return false;
                }
            }

            if (eager_new && repeat_in_block) {
                element_default = Nothing{};
            } else if (eager_new) {
                auto instance = instantiate_class(declared_class_name, identifier_offset);
                if (!instance.has_value()) {
                    return false;
                }
                element_default = std::move(*instance);
            }

            if (lazy_new && !repeat_in_block) {
                pending_lazy_new_ = true;
            }
            if (!declared_class_name.empty() && !is_array) {
                // An array's declared_class_name (REQ-0214) is threaded
                // straight into its own ArrayValue.element_class_name
                // below instead, since `object_class_names` is keyed by
                // variable name for a whole scalar/object variable's own
                // Set-target class check, the same reasoning
                // is_variant/is_object already follow for
                // variant_variables/object_variables above.
                current_scope().object_class_names.emplace(*identifier, declared_class_name);
            }
        }

        Value initial_value;
        if (is_array) {
            const auto element_type_index = element_default.index();
            if (is_dynamic_array) {
                ArrayValue array_value{
                    /*elements=*/{}, /*lower_bound=*/0, /*is_dynamic=*/true,
                    /*is_allocated=*/false, element_type_index, /*dimensions=*/{}, is_variant,
                    is_object, declared_class_name};
                array_value.dynamic_dimension_count = dynamic_dimension_count;
                array_value.dimension_count_declared = dynamic_dimension_count > 0U;
                initial_value = std::move(array_value);
            } else if (!array_dimensions.empty()) {
                std::size_t total_size = 1U;
                for (const auto& dimension : array_dimensions) {
                    total_size *=
                        static_cast<std::size_t>(dimension.second - dimension.first) + 1U;
                }
                initial_value = ArrayValue{
                    std::vector<Value>(total_size, element_default), /*lower_bound=*/0,
                    /*is_dynamic=*/false, /*is_allocated=*/true, element_type_index,
                    array_dimensions, is_variant, is_object, declared_class_name};
            } else {
                const auto size = static_cast<std::size_t>(array_upper - array_lower) + 1U;
                initial_value = ArrayValue{
                    std::vector<Value>(size, std::move(element_default)), array_lower,
                    /*is_dynamic=*/false, /*is_allocated=*/true, element_type_index,
                    /*dimensions=*/{}, is_variant, is_object, declared_class_name};
            }
        } else {
            initial_value = std::move(element_default);
        }
        if (is_array && fixed_string_length_ != 0U) {
            if (auto* const fixed_array = std::get_if<ArrayValue>(&initial_value)) {
                fixed_array->element_fixed_length = fixed_string_length_;
            }
        }
        if (udt_array_ && !repeat_in_block) {
            udt_array_ = false;
            if (auto* const array = std::get_if<ArrayValue>(&initial_value)) {
                for (auto& element : array->elements) {
                    auto instance = instantiate_class(declared_class_name, identifier_offset);
                    if (!instance.has_value()) {
                        return false;
                    }
                    element = std::move(*instance);
                }
            }
        }

        const auto [entry, inserted] =
            current_scope().variables.emplace(*identifier, std::move(initial_value));
        (void)entry;
        if (!inserted) {
            if (repeat_in_block) {
                return true;  // REQ-0271: re-executed declaration inside a block
            }
            set_error("WFC0013", "duplicate variable declaration", identifier_offset);
            return false;
        }
        if (fixed_string_length_ != 0U && !is_array) {
            current_scope().fixed_string_lengths[*identifier] = fixed_string_length_;
        }
        if (pending_lazy_new_) {
            pending_lazy_new_ = false;
            current_scope().auto_new_variables.insert(*identifier);
        }
        // A Variant/Object-*element* array (REQ-0212) does not itself go in
        // variant_variables/object_variables: those sets govern a whole
        // scalar/object variable's own retyping-on-assignment and
        // Set-only rules, which do not apply to the array *variable*
        // itself (a whole-array assignment like `arr1 = arr2` is an
        // ordinary same-type value copy, matching every other array kind's
        // existing REQ-0201 simplification) -- only to its elements,
        // checked directly against ArrayValue.is_variant_element/
        // is_object_element wherever an element is read or written.
        if (is_variant && !is_array) {
            current_scope().variant_variables.insert(*identifier);
        }
        if (is_object && !is_array) {
            current_scope().object_variables.insert(*identifier);
        }
        return true;
    }

    // `ReDim [Preserve] identifier(<bound>, ...)` (REQ-0207, extended to
    // multiple comma-separated bounds by REQ-0219). Unlike `Dim`, `ReDim`
    // is an ordinary executable statement, not a declaration: it targets
    // a variable already declared `Dim identifier()`/`Dim identifier(,
    // ...)` (a dynamic array, `ArrayValue.is_dynamic`), reallocating it to
    // the new bounds. `ReDim` never carries an `As Type` clause -- the
    // element type is fixed by the original `Dim` and remembered on the
    // array itself (`element_type_index`). The array's *dimension count*,
    // once fixed (by a comma-only `Dim`, or by an earlier `ReDim`), never
    // changes again -- only bounds do; every `ReDim` after the first must
    // name the same number of dimensions. `Preserve` on a multi-
    // dimensional array may only change the *last* dimension's bounds
    // (matching real VB6); every other dimension must keep its exact
    // current bounds. `Preserve` copies every element whose absolute
    // index survives into the new last-dimension range from the old
    // array; without it (or before the array's first `ReDim`), every slot
    // is reset to the element type's default value.
    [[nodiscard]] bool parse_redim_statement() {
        skip_horizontal_whitespace();
        const bool preserve = consume_keyword("preserve");
        if (preserve) {
            skip_horizontal_whitespace();
        }
        while (true) {
            if (!parse_redim_declarator(preserve)) {
                return false;
            }
            skip_horizontal_whitespace();
            if (!consume(',')) {
                return true;
            }
            skip_horizontal_whitespace();
        }
    }

    [[nodiscard]] bool parse_redim_declarator(const bool preserve) {
        const auto identifier_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value()) {
            set_error("WFC0011", "expected array name", identifier_offset);
            return false;
        }
        std::vector<std::string> member_path;  // `ReDim obj.field(...)`
        skip_horizontal_whitespace();
        while (!at_end() && current() == '.') {
            advance();
            skip_horizontal_whitespace();
            char member_type_character{};
            auto member = parse_identifier(&member_type_character);
            if (!member.has_value()) {
                set_error("WFC0011", "expected member name after '.'", offset_);
                return false;
            }
            member_path.push_back(std::move(*member));
            skip_horizontal_whitespace();
        }
        if (at_end() || current() != '(') {
            set_error("WFC0145", "ReDim requires an array bound", offset_);
            return false;
        }
        advance();
        skip_horizontal_whitespace();
        if (!at_end() && current() == ')') {
            set_error("WFC0145", "ReDim requires an array bound", offset_);
            return false;
        }
        std::vector<std::pair<Integer, Integer>> new_dimensions;
        while (true) {
            const auto first_offset = offset_;
            auto first_bound = parse_expression();
            if (!first_bound.has_value()) {
                return false;
            }
            const auto first_long = coerce_long(*first_bound, first_offset);
            if (!first_long.has_value()) {
                return false;
            }
            skip_horizontal_whitespace();
            Integer dimension_lower = 0;
            Integer dimension_upper = 0;
            if (consume_keyword("to")) {
                skip_horizontal_whitespace();
                const auto second_offset = offset_;
                auto second_bound = parse_expression();
                if (!second_bound.has_value()) {
                    return false;
                }
                const auto second_long = coerce_long(*second_bound, second_offset);
                if (!second_long.has_value()) {
                    return false;
                }
                dimension_lower = *first_long;
                dimension_upper = *second_long;
            } else {
                // REQ-0226: matches Dim's own bound-less-dimension rule.
                dimension_lower = option_base_one_ ? 1 : 0;
                dimension_upper = *first_long;
            }
            if (execute_ && dimension_lower > dimension_upper) {
                return raise_runtime(9, "Subscript out of range", identifier_offset);
            }
            new_dimensions.emplace_back(dimension_lower, dimension_upper);
            skip_horizontal_whitespace();
            if (!at_end() && current() == ',') {
                advance();
                skip_horizontal_whitespace();
                continue;
            }
            break;
        }
        if (!consume(')')) {
            set_error("WFC0005", "expected closing parenthesis", offset_);
            return false;
        }
        // `ReDim a(n) As Type`: the type must agree with the array's own; for a
        // Variant it is the new array's element type.
        std::optional<ResolvedType> redim_type;
        skip_horizontal_whitespace();
        if (consume_keyword("as")) {
            skip_horizontal_whitespace();
            const auto type_offset = offset_;
            redim_type = parse_scalar_object_or_class_type();
            if (!redim_type.has_value()) {
                set_error("WFC0012", "expected a type after As", type_offset);
                return false;
            }
        }

        if (!execute_) {
            return true;
        }

        auto variable_lookup = find_variable(*identifier);
        if (variable_lookup.value == nullptr && member_path.empty() && !strict_declarations_ &&
            !in_with_identifier(*identifier)) {
            // Without Option Explicit, ReDim declares the (Variant) array too.
            current_scope().variables.emplace(*identifier, Value{Empty{}});
            current_scope().variant_variables.insert(*identifier);
            variable_lookup = find_variable(*identifier);
        }
        Value* redim_target = variable_lookup.value;
        for (const auto& member : member_path) {
            auto* holder = redim_target != nullptr ? std::get_if<ObjectInstance>(redim_target) : nullptr;
            if (holder == nullptr) {
                set_error("WFC0136", "member access requires an object reference", identifier_offset);
                return false;
            }
            const auto field = holder->data->fields.variables.find(member);
            if (field == holder->data->fields.variables.end()) {
                set_error("WFC0135", "unknown member", identifier_offset);
                return false;
            }
            redim_target = &field->second;
        }
        if (redim_target != nullptr && !std::holds_alternative<ArrayValue>(*redim_target) &&
            (std::holds_alternative<Empty>(*redim_target) ||
             (member_path.empty() &&
              variable_lookup.scope->variant_variables.contains(*identifier)))) {
            // A Variant (or Empty) becomes a dynamic array.
            ArrayValue created{};
            created.is_dynamic = true;
            created.is_allocated = false;
            if (redim_type.has_value() && !redim_type->is_variant) {
                if (redim_type->is_object) {
                    created.is_object_element = true;
                    created.element_class_name = redim_type->class_name;
                    created.element_type_index = Value{Nothing{}}.index();
                } else {
                    created.element_type_index = redim_type->type_index;
                }
            } else {
                created.is_variant_element = true;
                created.element_type_index = Value{Empty{}}.index();
            }
            *redim_target = std::move(created);
        }
        struct RedimVariable { Value* value; };
        const RedimVariable variable{redim_target};
        if (variable.value == nullptr || !std::holds_alternative<ArrayValue>(*variable.value) ||
            !std::get<ArrayValue>(*variable.value).is_dynamic) {
            set_error(
                "WFC0145",
                "ReDim requires a previously declared dynamic array (Dim identifier())",
                identifier_offset);
            return false;
        }
        auto& array = std::get<ArrayValue>(*variable.value);

        // REQ-0219: a dynamic array's dimension count, once fixed (by a
        // comma-only `Dim` or an earlier `ReDim`), is authoritative for
        // every later `ReDim`; only an array that has never been
        // allocated *and* was declared with the plain `Dim identifier()`
        // form (no pre-declared count) lets this first `ReDim` decide it.
        std::size_t required_dimension_count = new_dimensions.size();
        if (array.dimension_count_declared) {
            required_dimension_count = array.dynamic_dimension_count;  // `Dim a(,)` fixed it
        } else if (array.is_allocated && preserve) {
            required_dimension_count = array.dimensions.empty() ? 1U : array.dimensions.size();
        }
        if (new_dimensions.size() != required_dimension_count) {
            set_error(
                "WFC0115",
                "ReDim dimension count does not match the array's declared dimension count",
                identifier_offset);
            return false;
        }

        // REQ-0219: `ReDim Preserve` on a multi-dimensional array may only
        // resize the last dimension; every earlier dimension must keep
        // its exact current bounds, matching real VB6's own restriction
        // (a 1-D array has no "earlier dimension" to check, so this loop
        // never runs for one).
        if (preserve && array.is_allocated && new_dimensions.size() > 1U) {
            for (std::size_t dimension = 0; dimension + 1U < new_dimensions.size(); ++dimension) {
                if (new_dimensions[dimension] != array.dimensions[dimension]) {
                    set_error(
                        "WFC0151",
                        "ReDim Preserve may only change a multi-dimensional array's last "
                        "dimension",
                        identifier_offset);
                    return false;
                }
            }
        }

        std::size_t total_size = 1U;
        for (const auto& dimension : new_dimensions) {
            total_size *= static_cast<std::size_t>(dimension.second - dimension.first) + 1U;
        }
        std::vector<Value> new_elements(
            total_size,
            array.element_fixed_length != 0U
                ? Value{std::string(array.element_fixed_length, ' ')}
                : array_element_default(array.element_type_index));

        if (preserve && array.is_allocated && !array.elements.empty()) {
            // Every dimension except the last keeps identical bounds
            // (validated above, or there is only one dimension), so the
            // "outer" stride is the same in the old and new arrays; only
            // the last dimension's absolute-index overlap needs
            // computing -- generalizing the original 1-D-only overlap
            // logic (which allowed the single dimension's bounds to
            // shift, not just grow/shrink) to the last dimension of any
            // dimension count, 1-D included.
            const std::pair<Integer, Integer> old_last =
                array.dimensions.empty()
                    ? std::pair<Integer, Integer>{
                          array.lower_bound,
                          array.lower_bound + static_cast<Integer>(array.elements.size()) - 1}
                    : array.dimensions.back();
            const auto& new_last = new_dimensions.back();
            const Integer overlap_lower = std::max(old_last.first, new_last.first);
            const Integer overlap_upper = std::min(old_last.second, new_last.second);
            if (overlap_lower <= overlap_upper) {
                const auto old_last_size =
                    static_cast<std::size_t>(old_last.second - old_last.first) + 1U;
                const auto new_last_size =
                    static_cast<std::size_t>(new_last.second - new_last.first) + 1U;
                const auto outer_size = array.elements.size() / old_last_size;
                for (std::size_t outer_index = 0; outer_index < outer_size; ++outer_index) {
                    for (Integer absolute_index = overlap_lower; absolute_index <= overlap_upper;
                         ++absolute_index) {
                        const auto old_flat = outer_index * old_last_size +
                            static_cast<std::size_t>(absolute_index - old_last.first);
                        const auto new_flat = outer_index * new_last_size +
                            static_cast<std::size_t>(absolute_index - new_last.first);
                        new_elements[new_flat] = array.elements[old_flat];
                    }
                }
            }
        }

        if (array.is_object_element && is_udt_class(array.element_class_name)) {
            // REQ-0241/0253: every slot of a UDT array owns its own instance.
            for (auto& slot : new_elements) {
                if (std::holds_alternative<Nothing>(slot)) {
                    auto instance = instantiate_class(array.element_class_name, identifier_offset);
                    if (!instance.has_value()) {
                        return false;
                    }
                    slot = std::move(*instance);
                }
            }
        }

        array.elements = std::move(new_elements);
        if (new_dimensions.size() == 1U) {
            array.lower_bound = new_dimensions.front().first;
            array.dimensions.clear();
        } else {
            array.lower_bound = 0;
            array.dimensions = new_dimensions;
        }
        array.is_allocated = true;
        array.dynamic_dimension_count = new_dimensions.size();
        return true;
    }

    // `Erase identifier[, identifier...]` (REQ-0208). For a fixed-size
    // array, resets every element to the declared type's default value
    // (the array stays allocated at its original bounds). For a dynamic
    // array, deallocates it entirely -- as if it had never been `ReDim`'d
    // -- matching real VB6's differing `Erase` behavior for the two array
    // kinds.
    [[nodiscard]] bool parse_erase_statement() {
        std::vector<std::pair<std::string, std::size_t>> targets;
        while (true) {
            skip_horizontal_whitespace();
            const auto identifier_offset = offset_;
            char type_character{};
            auto identifier = parse_identifier(&type_character);
            if (!identifier.has_value()) {
                set_error("WFC0011", "expected array name", identifier_offset);
                return false;
            }
            targets.emplace_back(std::move(*identifier), identifier_offset);
            skip_horizontal_whitespace();
            if (!at_end() && current() == ',') {
                advance();
                continue;
            }
            break;
        }
        if (!execute_) {
            return true;
        }
        for (const auto& target : targets) {
            const auto variable = find_variable(target.first);
            if (variable.value == nullptr || !std::holds_alternative<ArrayValue>(*variable.value)) {
                set_error("WFC0146", "Erase requires an array argument", target.second);
                return false;
            }
            auto& array = std::get<ArrayValue>(*variable.value);
            if (array.is_dynamic) {
                // REQ-0219: a multi-dimensional dynamic array's
                // `dimensions` must be cleared too, not just `elements` --
                // otherwise a later index read/write would still see the
                // old (now-stale) per-dimension bounds as "in range" while
                // `elements` is empty, indexing past the end of an empty
                // vector. If the array was ever allocated,
                // `dynamic_dimension_count` is locked in from its
                // about-to-be-cleared shape first, so a later `ReDim`
                // still enforces the same dimension count Erase does not
                // let the array forget, matching real VB6; if it was
                // never allocated at all (Erase on an untouched `Dim
                // arr()`/`Dim arr(,)`), any already-pre-declared count is
                // left exactly as it was.
                if (array.is_allocated) {
                    array.dynamic_dimension_count = array_expected_dimension_count(array);
                }
                array.dimensions.clear();
                array.elements.clear();
                array.lower_bound = 0;
                array.is_allocated = false;
            } else {
                std::fill(
                    array.elements.begin(), array.elements.end(),
                    array_element_default(array.element_type_index));
            }
        }
        return true;
    }

    [[nodiscard]] bool parse_constant_declaration() {
        while (true) {
            if (!parse_single_constant_declaration()) {
                return false;
            }
            skip_horizontal_whitespace();
            if (at_end() || current() != ',') {
                return true;
            }
            advance();
        }
    }

    [[nodiscard]] bool parse_single_constant_declaration() {
        skip_horizontal_whitespace();
        const auto identifier_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value()) {
            set_error("WFC0011", "expected constant name", identifier_offset);
            return false;
        }
        if (is_reserved_identifier(*identifier)) {
            set_error("WFC0017", "reserved keyword cannot be a constant name", identifier_offset);
            return false;
        }
        if (!validate_type_character(type_character, identifier_offset)) {
            return false;
        }
        const bool repeat_constant =
            !allow_declarations_ && current_scope().variables.contains(*identifier);
        if (!repeat_constant && current_scope().variables.contains(*identifier)) {
            set_error("WFC0013", "duplicate variable or constant declaration", identifier_offset);
            return false;
        }

        // REQ-0227: a `Const` with neither a type-declaration character
        // nor an `As Type` clause infers its type from the initializer's
        // own value, the same real-VB6 asymmetry that makes an untyped
        // `Const` type-inferred while an untyped `Dim` instead defaults
        // to `Variant` (parse_declaration's own bare-`Dim` branch, above).
        // `has_explicit_type` stays false in that case, and `expected_type`
        // is set from the parsed value below instead of checked against it.
        bool has_explicit_type = type_character != '\0';
        std::size_t expected_type{};
        skip_horizontal_whitespace();
        if (type_character != '\0') {
            if (consume_keyword("as")) {
                set_error("WFC0012", "type-declaration character cannot be combined with As", offset_);
                return false;
            }
            expected_type = type_character_index(type_character);
        } else if (consume_keyword("as")) {
            has_explicit_type = true;
            skip_horizontal_whitespace();
            if (consume_keyword("long")) {
                expected_type = Value{Integer{}}.index();
            } else if (consume_keyword("integer")) {
                expected_type = Value{Int16{}}.index();
            } else if (consume_keyword("double")) {
                expected_type = Value{0.0}.index();
            } else if (consume_keyword("single")) {
                expected_type = Value{0.0f}.index();
            } else if (consume_keyword("date")) {
                expected_type = Value{DateValue{}}.index();
            } else if (consume_keyword("byte")) {
                expected_type = Value{Byte{}}.index();
            } else if (consume_keyword("decimal")) {
                expected_type = Value{Decimal{}}.index();
            } else if (consume_keyword("currency")) {
                expected_type = Value{Currency{}}.index();
            } else if (consume_keyword("string")) {
                expected_type = Value{std::string{}}.index();
            } else if (consume_keyword("boolean")) {
                expected_type = Value{false}.index();
            } else {
                set_error(
                    "WFC0012",
                    "expected As Integer, As Long, As Double, As Single, As Currency, As "
                    "String, or As Boolean",
                    offset_);
                return false;
            }
        }

        skip_horizontal_whitespace();
        if (!consume('=')) {
            set_error("WFC0014", "expected constant initializer", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        constant_expression_ = true;
        auto value = parse_expression();
        constant_expression_ = false;
        if (!value.has_value()) {
            return false;
        }
        if (has_explicit_type) {
            if (!coerce_numeric_value(*value, expected_type, identifier_offset)) {
                return false;
            }
            if (value->index() != expected_type) {
                set_error("WFC0016", "constant initializer type mismatch", identifier_offset);
                return false;
            }
        }

        if (repeat_constant) {
            return true;
        }
        current_scope().variables.emplace(*identifier, std::move(*value));
        current_scope().constants.insert(std::move(*identifier));
        return true;
    }

    enum class AppendOutcome { not_applicable, done, failed };

    // `s = s & expr [& expr...]` extends the variable's string in place instead of building
    // and copying a new string each time (quadratic in loops that accumulate text). Used only
    // when the rest of the statement is a plain concatenation of side-effect-free operands.
    [[nodiscard]] bool append_statement_eligible(const std::string& identifier) {
        std::size_t i = offset_;
        for (const char expected : identifier) {
            if (i >= source_.size() || ascii_lower(source_[i]) != expected) return false;
            ++i;
        }
        if (i < source_.size() && (is_identifier_part(source_[i]) || source_[i] == '.')) return false;
        while (i < source_.size() && (source_[i] == ' ' || source_[i] == '\t')) ++i;
        if (i >= source_.size() || source_[i] != '&') return false;
        ++i;
        if (i < source_.size() && source_[i] == '=') return false;
        const auto* const class_def = current_class_def();
        bool in_string = false;
        while (i < source_.size() && source_[i] != '\r' && source_[i] != '\n') {
            const char ch = source_[i];
            if (ch == '"') {
                in_string = !in_string;
                ++i;
                continue;
            }
            if (in_string) {
                ++i;
                continue;
            }
            if (ch == ':' || ch == '\'') break;
            if (ch == '=' || ch == '<' || ch == '>') return false;
            if (is_identifier_start(ch)) {
                const bool member_access = i > 0 && source_[i - 1] == '.';
                std::string word;
                while (i < source_.size() && is_identifier_part(source_[i])) {
                    word.push_back(ascii_lower(source_[i]));
                    ++i;
                }
                if (member_access) {
                    continue;
                }
                static const std::set<std::string, std::less<>> excluded = {
                    "and", "or", "xor", "eqv", "imp", "like", "is", "else", "not", "input",
                    "rnd", "then", "to", "step", "typeof", "new", "set", "let", "get", "put",
                    "callbyname", "createobject", "getobject", "shell", "inputb", "inputbox"};
                if (excluded.contains(word) || procedures_.contains(word) ||
                    (class_def != nullptr && class_def->methods.contains(word))) {
                    return false;
                }
                continue;
            }
            ++i;
        }
        return !in_string;
    }

    [[nodiscard]] AppendOutcome try_append_assignment(const std::string& identifier, std::string& target) {
        const char* const key = source_.data() + offset_;
        auto cached = append_eligibility_.find(key);
        if (cached == append_eligibility_.end()) {
            cached = append_eligibility_.emplace(key, append_statement_eligible(identifier)).first;
        }
        if (!cached->second) {
            return AppendOutcome::not_applicable;
        }
        offset_ += identifier.size();
        skip_horizontal_whitespace();
        static_cast<void>(consume('&'));
        std::string appended;
        while (true) {
            skip_horizontal_whitespace();
            const auto operand_offset = offset_;
            auto operand = parse_additive();
            if (!operand.has_value()) {
                return AppendOutcome::failed;
            }
            if (!resolve_default_value(*operand, operand_offset)) {
                return AppendOutcome::failed;
            }
            if (is_object_reference(*operand) || std::holds_alternative<ArrayValue>(*operand)) {
                set_error("WFC0020", "concatenation requires String or Long operands", operand_offset);
                return AppendOutcome::failed;
            }
            if (const auto* text = std::get_if<std::string>(&*operand)) {
                appended += *text;
            } else if (!std::holds_alternative<Null>(*operand)) {
                appended += render(*operand);
            }
            skip_horizontal_whitespace();
            if (!consume('&')) break;
        }
        target += appended;
        return AppendOutcome::done;
    }

    // Whether the parenthesised group opening at `open_offset` is followed by a '.'.
    [[nodiscard]] bool paren_followed_by_dot(const std::size_t open_offset) const noexcept {
        std::size_t depth = 0;
        bool in_string = false;
        for (std::size_t i = open_offset; i < source_.size(); ++i) {
            const char ch = source_[i];
            if (ch == '\r' || ch == '\n') return false;
            if (ch == '"') {
                in_string = !in_string;
            } else if (!in_string) {
                if (ch == '(') {
                    ++depth;
                } else if (ch == ')') {
                    if (--depth == 0) {
                        std::size_t k = i + 1;
                        while (k < source_.size() && (source_[k] == ' ' || source_[k] == '\t')) ++k;
                        return k < source_.size() && source_[k] == '.';
                    }
                }
            }
        }
        return false;
    }

    std::size_t chain_counter_{};

    [[nodiscard]] bool parse_assignment(
        std::string identifier,
        const char type_character = '\0') {
        const auto identifier_offset = offset_ - identifier.size() -
            (type_character == '\0' ? 0U : 1U);
        const auto variable = find_variable(identifier);
        if (variable.value == nullptr) {
            // An unqualified write to a sibling Property Let of the class
            // currently executing -- the assignment counterpart of the
            // unqualified Property Get read/sibling method call
            // current_instance/current_class_def already give expressions
            // (see REQ-0203's parse_primary_base/parse_call_statement).
            if (type_character == '\0') {
                if (auto* const instance = current_instance()) {
                    if (const auto* const class_def = current_class_def()) {
                        const auto letter_iterator =
                            class_def->property_let.find(identifier);
                        if (letter_iterator != class_def->property_let.end()) {
                            return invoke_property_let_or_set(
                                *instance, *class_def, letter_iterator->second, identifier,
                                identifier_offset);
                        }
                    }
                }
            }
            if (type_character == '\0') {
                const auto letter = procedures_.find("wfclet_" + identifier);
                if (letter != procedures_.end()) {
                    std::vector<CallArgument> arguments;
                    skip_horizontal_whitespace();
                    if (!at_end() && current() == '(') {
                        auto parsed = parse_call_argument_list();
                        if (!parsed.has_value()) return false;
                        arguments = std::move(*parsed);
                    }
                    skip_horizontal_whitespace();
                    if (!consume('=')) {
                        set_error("WFC0014", "expected assignment operator", offset_);
                        return false;
                    }
                    skip_horizontal_whitespace();
                    auto value = parse_expression();
                    if (!value.has_value()) return false;
                    arguments.push_back(CallArgument{std::move(*value), nullptr});
                    return invoke_definition(
                               letter->second, identifier, std::move(arguments),
                               identifier_offset, source_, nullptr)
                        .has_value();
                }
            }
            if (!strict_declarations_ && !in_with_identifier(identifier) &&
                !procedures_.contains(identifier)) {
                // REQ-0265: without Option Explicit an assignment declares
                // the variable implicitly (a Variant, or the suffix's type).
                Value initial = Value{Empty{}};
                bool variant = true;
                if (type_character != '\0') {
                    initial = zero_value_for_index(type_character_index(type_character));
                    variant = false;
                } else if (const auto default_type = default_type_for(identifier)) {
                    initial = zero_value_for_index(*default_type);
                    variant = false;
                }
                current_scope().variables.emplace(identifier, std::move(initial));
                if (variant) {
                    current_scope().variant_variables.insert(identifier);
                }
                return parse_assignment(std::move(identifier), type_character);
            }
            set_error("WFC0015", "undeclared variable", identifier_offset);
            return false;
        }
        if (!type_character_matches(*variable.value, type_character, identifier_offset)) {
            return false;
        }
        if (variable.scope->constants.contains(identifier)) {
            set_error("WFC0062", "cannot assign to constant", identifier_offset);
            return false;
        }
        if (variable.scope->object_variables.contains(identifier)) {
            const auto declared = variable.scope->object_class_names.find(identifier);
            if (declared != variable.scope->object_class_names.end() &&
                is_udt_class(declared->second)) {
                skip_horizontal_whitespace();
                if (!consume('=')) {
                    set_error("WFC0014", "expected assignment operator", offset_);
                    return false;
                }
                skip_horizontal_whitespace();
                auto source = parse_expression();
                if (!source.has_value()) {
                    return false;
                }
                return assign_udt(*variable.value, *source, identifier_offset);
            }
            set_error("WFC0108", "object assignment requires Set", identifier_offset);
            return false;
        }

        skip_horizontal_whitespace();
        if (!consume('=')) {
            set_error("WFC0014", "expected assignment operator", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        if (execute_) {
            if (auto* const target = std::get_if<std::string>(variable.value);
                target != nullptr && !variable.scope->fixed_string_lengths.contains(identifier)) {
                const auto outcome = try_append_assignment(identifier, *target);
                if (outcome != AppendOutcome::not_applicable) {
                    return outcome == AppendOutcome::done;
                }
            }
        }
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        if (variable.scope->variant_variables.contains(identifier)) {
            // A Variant-declared variable freely accepts any value type,
            // retyping itself on each assignment (the agreed scalar-Variant
            // scope: no fixed-type enforcement for these variables).
            if (execute_) {
                if (!terminate_if_last_reference(*variable.value)) {
                    return false;
                }
                *variable.value = copy_if_udt(std::move(*value));
            }
            return true;
        }
        // Byte() <-> String assignment copies the string's UCS-2 bytes (VB6 semantics).
        if (auto* target_array = std::get_if<ArrayValue>(variable.value);
            target_array != nullptr && target_array->element_type_index == Value{Byte{}}.index() &&
            std::holds_alternative<std::string>(*value)) {
            if (execute_) {
                const auto& text = std::get<std::string>(*value);
                target_array->elements.clear();
                for (const char16_t unit : to_utf16_units(text)) {
                    target_array->elements.emplace_back(static_cast<Byte>(unit & 0xFFU));
                    target_array->elements.emplace_back(static_cast<Byte>(unit >> 8U));
                }
                target_array->lower_bound = 0;
                target_array->dimensions.clear();
                target_array->is_allocated = true;
            }
            return true;
        }
        if (std::holds_alternative<std::string>(*variable.value)) {
            if (const auto* source_array = std::get_if<ArrayValue>(&*value);
                source_array != nullptr &&
                source_array->element_type_index == Value{Byte{}}.index()) {
                if (execute_) {
                    std::u16string units;
                    const auto byte_at = [&](const std::size_t i) -> unsigned {
                        const auto* byte = std::get_if<Byte>(&source_array->elements[i]);
                        return byte != nullptr ? *byte : static_cast<unsigned>('?');
                    };
                    for (std::size_t i = 0; i < source_array->elements.size(); i += 2U) {
                        const unsigned high = i + 1U < source_array->elements.size() ? byte_at(i + 1U) : 0U;
                        units.push_back(static_cast<char16_t>(byte_at(i) | (high << 8U)));
                    }
                    *variable.value = from_utf16_units(units);
                }
                return true;
            }
        }
        if (!coerce_numeric_value(*value, variable.value->index(), identifier_offset)) {
            return false;
        }
        if (variable.value->index() != value->index()) {
            set_error("WFC0016", "assignment type mismatch", identifier_offset);
            return false;
        }
        if (execute_) {
            if (const auto fixed = variable.scope->fixed_string_lengths.find(identifier);
                fixed != variable.scope->fixed_string_lengths.end()) {
                if (auto* text = std::get_if<std::string>(&*value)) {
                    fit_to_units(*text, fixed->second);
                }
            }
            if (std::holds_alternative<ArrayValue>(*value)) {
                deep_copy_udt_values(*value);
            }
            // Assigning an array to a dynamic array leaves the target dynamic (resizable).
            const bool target_was_dynamic = [&] {
                const auto* previous = std::get_if<ArrayValue>(variable.value);
                return previous != nullptr && previous->is_dynamic;
            }();
            *variable.value = std::move(*value);
            if (target_was_dynamic) {
                std::get<ArrayValue>(*variable.value).is_dynamic = true;
            }
        }
        return true;
    }

    // Assigns an object reference into `target`, the way `Set` always does:
    // `source` must itself be Nothing or a live instance (`WFC0106`
    // otherwise), and, when `declared_class_name` is non-empty (a `Dim`/
    // field declared `As ClassName` rather than the generic `As Object`),
    // an `ObjectInstance` source's own class must match it exactly
    // (`WFC0137`). Shared by parse_set_statement (a plain variable target)
    // and parse_member_set_assignment's plain-field-target branch (a
    // class-typed or `As Object` field with no Property Set accessor,
    // under REQ-0205).
    // REQ-0233: whether an instance of `actual_class_name` may be used
    // wherever `declared_class_name` is required -- either directly (the
    // same class) or because `actual_class_name`'s own class `Implements`
    // `declared_class_name` (an interface is just an ordinary class in
    // real VB6; any class it names in an `Implements` statement is one
    // this check accepts in its place). Shared by every "does this Set
    // source/argument match the declared class" check, so a class-typed
    // target/parameter accepts an implementing instance the same way it
    // already accepts an exact match.
    [[nodiscard]] bool class_satisfies(
        const std::string& actual_class_name, const std::string& declared_class_name) const {
        if (actual_class_name == declared_class_name) {
            return true;
        }
        const auto class_iterator = class_definitions_.find(actual_class_name);
        if (class_iterator == class_definitions_.end()) {
            return false;
        }
        for (const auto& implemented : class_iterator->second.implements) {
            if (implemented == declared_class_name) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool assign_object_reference(
        Value& target,
        const std::string& declared_class_name,
        Value source,
        const std::size_t offset) {
        if (execute_ && !std::holds_alternative<Nothing>(source) &&
            !std::holds_alternative<ObjectInstance>(source)) {
            set_error("WFC0106", "Set requires an object reference", offset);
            return false;
        }
        if (!declared_class_name.empty() && std::holds_alternative<ObjectInstance>(source) &&
            !class_satisfies(
                std::get<ObjectInstance>(source).data->class_name, declared_class_name)) {
            set_error(
                "WFC0137", "Set source does not match the target's declared class", offset);
            return false;
        }
        if (execute_) {
            if (!terminate_if_last_reference(target)) {
                return false;
            }
            target = std::move(source);
        }
        return true;
    }

    // Parses the remainder of a Property Let/Set-routed assignment once
    // `definition` has already been resolved: an optional parenthesized
    // index-argument list (an indexed property -- `Property Let/Set
    // Name(index [, ...], value)`, `Property Get Name(index [, ...]) As
    // Type`; see REQ-0205), the assignment operator, and the value
    // expression, then invokes `definition` with the index arguments
    // followed by the value as its final argument. Shared by an
    // unqualified sibling write (parse_assignment), `obj.Prop[(args)] =
    // expr` (parse_member_assignment), and `Set obj.Prop[(args)] = expr`
    // (parse_member_set_assignment).
    [[nodiscard]] bool invoke_property_let_or_set(
        InstanceData& instance,
        const ClassDef& class_def,
        const ProcedureDef& definition,
        const std::string& property_name,
        const std::size_t property_offset) {
        std::vector<CallArgument> arguments;
        skip_horizontal_whitespace();
        if (!at_end() && current() == '(') {
            auto parsed = parse_call_argument_list();
            if (!parsed.has_value()) {
                return false;
            }
            arguments = std::move(*parsed);
        }
        skip_horizontal_whitespace();
        if (!consume('=')) {
            set_error("WFC0014", "expected assignment operator", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        arguments.push_back(CallArgument{std::move(*value), nullptr});
        auto result = invoke_definition(
            definition, property_name, std::move(arguments), property_offset, class_def.source,
            &instance);
        return result.has_value();
    }

    // `Set obj.Prop = expression` assigns through a Property Set accessor
    // when the class declares one for `Prop`; otherwise, for a class-typed
    // or `As Object` field with no accessor, directly to that field (see
    // REQ-0205 -- a plain, non-object field has no `Set` target at all,
    // since it is always read/written through ordinary `=`). `base` is the
    // already-evaluated object reference the member is accessed on; its
    // own '.' has already been consumed.
    [[nodiscard]] bool parse_member_set_assignment(const Value base, const std::size_t base_offset) {
        skip_horizontal_whitespace();
        const auto member_offset = offset_;
        char type_character{};
        auto member_name = parse_identifier(&type_character);
        if (!member_name.has_value() || type_character != '\0') {
            set_error("WFC0011", "expected member name after '.'", member_offset);
            return false;
        }
        if (std::holds_alternative<Nothing>(base)) {
            if (!execute_) {
                // Dry-run parsing of a not-taken branch (REQ-0229): as
                // parse_member_access_after_dot's identical case explains,
                // there is no class to resolve `.member` against at all
                // when `base` is genuinely `Nothing`. Still parses through
                // the `= expression` that must follow, so the source text
                // and the RHS's own shape are validated, but assigns
                // nothing (there is nowhere to assign it to).
                skip_horizontal_whitespace();
                if (!consume('=')) {
                    set_error("WFC0014", "expected assignment operator", offset_);
                    return false;
                }
                skip_horizontal_whitespace();
                return parse_expression().has_value();
            }
            set_error("WFC0106", "Invalid use of Nothing", base_offset);
            return false;
        }
        InstanceData& instance = *std::get<ObjectInstance>(base).data;
        const auto class_iterator = class_definitions_.find(instance.class_name);
        const ClassDef& class_def = class_iterator->second;
        const auto setter_iterator = class_def.property_set.find(*member_name);
        if (setter_iterator != class_def.property_set.end()) {
            if (!member_accessible(class_def, setter_iterator->second.is_private)) {
                set_error("WFC0142", "member is not accessible outside its class", member_offset);
                return false;
            }
            return invoke_property_let_or_set(
                instance, class_def, setter_iterator->second, *member_name, member_offset);
        }
        const auto field_iterator = instance.fields.variables.find(*member_name);
        if (field_iterator == instance.fields.variables.end() ||
            (!instance.fields.object_variables.contains(*member_name) &&
             !instance.fields.variant_variables.contains(*member_name))) {
            set_error("WFC0135", "unknown member (no Property Set accessor)", member_offset);
            return false;
        }
        const auto field_def_iterator = class_def.fields.find(*member_name);
        if (field_def_iterator != class_def.fields.end() &&
            !member_accessible(class_def, field_def_iterator->second.is_private)) {
            set_error("WFC0142", "member is not accessible outside its class", member_offset);
            return false;
        }
        skip_horizontal_whitespace();
        if (!at_end() && current() == '.') {
            // Chained `Set a.b.c = x`: `a.b` holds the object `c` is set on.
            const Value& next_base = field_iterator->second;
            if (!std::holds_alternative<ObjectInstance>(next_base) &&
                !std::holds_alternative<Nothing>(next_base)) {
                set_error("WFC0136", "member access requires an object reference", member_offset);
                return false;
            }
            advance();
            return parse_member_set_assignment(next_base, member_offset);
        }
        if (!consume('=')) {
            set_error("WFC0014", "expected assignment operator", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        const auto declared_class = instance.fields.object_class_names.find(*member_name);
        const std::string declared_class_name =
            declared_class != instance.fields.object_class_names.end() ? declared_class->second
                                                                         : std::string{};
        return assign_field_reference(
            instance, *member_name, field_iterator->second, declared_class_name,
            std::move(*value), member_offset);
    }

    // `Set identifier = expression` is the only legal way to assign an
    // object reference (see REQ-0200): the target must be a fixed
    // `Object`-typed (or `As ClassName`-typed) variable or a `Variant`-
    // declared one, and the source expression must itself be an object
    // reference -- `Nothing`, or (since REQ-0203) a live `New`-produced
    // instance. A target declared `As ClassName` (as opposed to the generic
    // `As Object`) additionally requires the source instance's own class to
    // match exactly (this evaluator has no class hierarchy/interfaces, so
    // "match" is always exact identity, not a compatible-supertype check).
    [[nodiscard]] bool parse_set_statement() {
        skip_horizontal_whitespace();
        const auto identifier_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value()) {
            set_error("WFC0011", "expected variable name after Set", identifier_offset);
            return false;
        }
        // `Set Me.Prop = expression`.
        if (type_character == '\0' && *identifier == "me") {
            skip_horizontal_whitespace();
            if (!at_end() && current() == '.') {
                auto base = me_value(identifier_offset);
                if (!base.has_value()) {
                    return false;
                }
                advance();
                return parse_member_set_assignment(*base, identifier_offset);
            }
            set_error("WFC0011", "expected member name after '.'", offset_);
            return false;
        }
        auto variable = find_variable_raw(*identifier);
        {
            // `Set a.b = x` / `Set a(i) = x` use `a`'s object, so an auto-new
            // variable is created first; `Set a = x` replaces it untouched.
            std::size_t look = offset_;
            while (look < source_.size() && (source_[look] == ' ' || source_[look] == '\t')) ++look;
            if (look < source_.size() && (source_[look] == '.' || source_[look] == '(')) {
                variable = find_variable(*identifier);
            }
        }
        if (variable.value == nullptr) {
            // An unqualified `Set Prop = expr` for a sibling Property Set
            // of the class currently executing -- the Set counterpart of
            // parse_assignment's unqualified Property Let write (REQ-0205).
            if (type_character == '\0') {
                if (auto* const instance = current_instance()) {
                    if (const auto* const class_def = current_class_def()) {
                        const auto setter_iterator =
                            class_def->property_set.find(*identifier);
                        if (setter_iterator != class_def->property_set.end()) {
                            return invoke_property_let_or_set(
                                *instance, *class_def, setter_iterator->second, *identifier,
                                identifier_offset);
                        }
                    }
                }
            }
            if (type_character == '\0') {
                const auto setter = procedures_.find("wfcset_" + *identifier);
                if (setter != procedures_.end()) {
                    std::vector<CallArgument> arguments;
                    skip_horizontal_whitespace();
                    if (!at_end() && current() == '(') {
                        auto parsed = parse_call_argument_list();
                        if (!parsed.has_value()) return false;
                        arguments = std::move(*parsed);
                    }
                    skip_horizontal_whitespace();
                    if (!consume('=')) {
                        set_error("WFC0014", "expected assignment operator", offset_);
                        return false;
                    }
                    skip_horizontal_whitespace();
                    auto value = parse_expression();
                    if (!value.has_value()) return false;
                    arguments.push_back(CallArgument{std::move(*value), nullptr});
                    return invoke_definition(
                               setter->second, *identifier, std::move(arguments),
                               identifier_offset, source_, nullptr)
                        .has_value();
                }
            }
            set_error("WFC0015", "undeclared variable", identifier_offset);
            return false;
        }

        const auto after_identifier_offset = offset_;
        skip_horizontal_whitespace();
        if (type_character == '\0' && !at_end() && current() == '(' &&
            std::holds_alternative<ObjectInstance>(*variable.value)) {
            auto& instance_data = *std::get<ObjectInstance>(*variable.value).data;
            const auto class_iterator = class_definitions_.find(instance_data.class_name);
            if (class_iterator != class_definitions_.end() &&
                !class_iterator->second.default_member.empty()) {
                const auto& class_def = class_iterator->second;
                const auto setter = class_def.property_set.find(class_def.default_member);
                if (setter != class_def.property_set.end()) {
                    return invoke_property_let_or_set(
                        instance_data, class_def, setter->second, class_def.default_member,
                        identifier_offset);
                }
            }
        }
        if (type_character == '\0' && !at_end() && current() == '.' &&
            (std::holds_alternative<Nothing>(*variable.value) ||
             std::holds_alternative<ObjectInstance>(*variable.value))) {
            const auto base = *variable.value;
            advance();
            return parse_member_set_assignment(base, identifier_offset);
        }
        // `Set arrayName(index [, index...]) = expression`, the Object-
        // element-array (REQ-0212) counterpart of a plain Object variable's
        // `Set name = expression`.
        if (type_character == '\0' && !at_end() && current() == '(' &&
            std::holds_alternative<ArrayValue>(*variable.value)) {
            auto& array = std::get<ArrayValue>(*variable.value);
            if (!array.is_object_element && !array.is_variant_element) {
                set_error(
                    "WFC0109", "Set requires an Object or Variant target", identifier_offset);
                return false;
            }
            advance();  // consume '('
            const auto dimension_count = array_expected_dimension_count(array);
            auto indices = parse_index_list(dimension_count);
            if (!indices.has_value()) {
                return false;
            }
            skip_horizontal_whitespace();
            if (!consume('=')) {
                set_error("WFC0014", "expected assignment operator", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            auto value = parse_expression();
            if (!value.has_value()) {
                return false;
            }
            if (!execute_) {
                return true;
            }
            const auto flat_offset = array_flat_offset(array, *indices);
            if (!flat_offset.has_value()) {
                return false;
            }
            return assign_object_reference(
                array.elements[*flat_offset], array.element_class_name, std::move(*value),
                identifier_offset);
        }
        offset_ = after_identifier_offset;

        if (!type_character_matches(*variable.value, type_character, identifier_offset)) {
            return false;
        }
        if (variable.scope->constants.contains(*identifier)) {
            set_error("WFC0062", "cannot assign to constant", identifier_offset);
            return false;
        }
        if (!variable.scope->object_variables.contains(*identifier) &&
            !variable.scope->variant_variables.contains(*identifier)) {
            set_error(
                "WFC0109", "Set requires an Object or Variant target", identifier_offset);
            return false;
        }

        skip_horizontal_whitespace();
        if (!consume('=')) {
            set_error("WFC0014", "expected assignment operator", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        const auto declared_class = variable.scope->object_class_names.find(*identifier);
        const std::string declared_class_name =
            declared_class != variable.scope->object_class_names.end() ? declared_class->second
                                                                         : std::string{};
        InstanceData* const owner = current_instance();
        if (owner != nullptr && &owner->fields == variable.scope) {
            return assign_field_reference(
                *owner, *identifier, *variable.value, declared_class_name, std::move(*value),
                identifier_offset);
        }
        return assign_object_reference(
            *variable.value, declared_class_name, std::move(*value), identifier_offset);
    }

    // `Set field = x` on an instance's field: like assign_object_reference,
    // but a WithEvents field also moves the instance's event subscription
    // from the old referent to the new one.
    [[nodiscard]] bool assign_field_reference(
        InstanceData& owner,
        const std::string& field_name,
        Value& slot,
        const std::string& declared_class_name,
        Value source,
        const std::size_t offset) {
        bool with_events = false;
        if (const auto class_iterator = class_definitions_.find(owner.class_name);
            class_iterator != class_definitions_.end()) {
            const auto field_def = class_iterator->second.fields.find(field_name);
            with_events = field_def != class_iterator->second.fields.end() &&
                field_def->second.with_events;
        }
        if (!with_events || !execute_) {
            return assign_object_reference(slot, declared_class_name, std::move(source), offset);
        }
        const auto unsubscribe = [&]() {
            if (const auto* old_instance = std::get_if<ObjectInstance>(&slot)) {
                auto& sinks = old_instance->data->event_sinks;
                std::erase_if(sinks, [&](const auto& entry) {
                    return entry.second == field_name && entry.first.lock().get() == &owner;
                });
            }
        };
        unsubscribe();
        const std::shared_ptr<InstanceData> new_data =
            std::holds_alternative<ObjectInstance>(source)
                ? std::get<ObjectInstance>(source).data
                : nullptr;
        if (!assign_object_reference(slot, declared_class_name, std::move(source), offset)) {
            return false;
        }
        if (new_data != nullptr) {
            new_data->event_sinks.emplace_back(owner.weak_from_this(), field_name);
        }
        return true;
    }

    // `RaiseEvent Name[(args)]`: runs every subscribed `field_Name` handler.
    [[nodiscard]] bool parse_raise_event_statement(const std::size_t statement_offset) {
        skip_horizontal_whitespace();
        const auto name_offset = offset_;
        char type_character{};
        const auto event_name = parse_identifier(&type_character);
        if (!event_name.has_value() || type_character != '\0') {
            set_error("WFC0011", "expected event name", name_offset);
            return false;
        }
        InstanceData* const source = current_instance();
        const ClassDef* const source_class = current_class_def();
        if (source == nullptr || source_class == nullptr ||
            !source_class->events.contains(*event_name)) {
            set_error("WFC0015", "event is not declared in this class", statement_offset);
            return false;
        }
        std::vector<CallArgument> arguments;
        skip_horizontal_whitespace();
        if (!at_end() && current() == '(') {
            auto parsed = parse_call_argument_list();
            if (!parsed.has_value()) {
                return false;
            }
            arguments = std::move(*parsed);
        }
        if (!execute_) {
            return true;
        }
        const auto sinks = source->event_sinks;  // handlers may resubscribe
        for (const auto& [weak_sink, field_name] : sinks) {
            const auto sink = weak_sink.lock();
            if (sink == nullptr) {
                continue;
            }
            const auto sink_class = class_definitions_.find(sink->class_name);
            if (sink_class == class_definitions_.end()) {
                continue;
            }
            const std::string handler = field_name + "_" + *event_name;
            const auto method = sink_class->second.methods.find(handler);
            if (method == sink_class->second.methods.end()) {
                continue;
            }
            if (!invoke_definition(
                    method->second, handler, arguments, statement_offset,
                    sink_class->second.source, sink.get())
                     .has_value()) {
                return false;
            }
        }
        return true;
    }

    // `identifier.member = expression` writes through a Property Let
    // accessor when the class declares one for `member`, otherwise directly
    // to that instance field. `base` is the already-evaluated object
    // reference the member is accessed on; its own '.' has already been
    // consumed.
    [[nodiscard]] bool parse_member_assignment(
        const Value base, const std::size_t base_offset,
        const std::string& via_interface_class = {}) {
        skip_horizontal_whitespace();
        const auto member_offset = offset_;
        char type_character{};
        auto member_name = parse_identifier(&type_character);
        if (!member_name.has_value() || type_character != '\0') {
            set_error("WFC0011", "expected member name after '.'", member_offset);
            return false;
        }
        if (std::holds_alternative<Nothing>(base)) {
            if (!execute_) {
                // Dry-run parsing of a not-taken branch (REQ-0229): as
                // parse_member_access_after_dot's identical case explains,
                // there is no class to resolve `.member` against at all
                // when `base` is genuinely `Nothing`. Still parses through
                // the `= expression` that must follow, so the source text
                // and the RHS's own shape are validated, but assigns
                // nothing (there is nowhere to assign it to).
                skip_horizontal_whitespace();
                // Skip a longer chain (`o.Child.Prop(1).Name = x`) down to its last member.
                while (!at_end() && (current() == '.' || current() == '(')) {
                    if (consume('.')) {
                        skip_horizontal_whitespace();
                        char chain_type_character{};
                        if (!parse_identifier(&chain_type_character).has_value()) {
                            set_error("WFC0011", "expected member name after '.'", offset_);
                            return false;
                        }
                    } else {
                        advance();
                        if (!parse_index_list(kAnyDimensionCount).has_value()) {
                            return false;
                        }
                    }
                    skip_horizontal_whitespace();
                }
                if (!consume('=')) {
                    if (at_statement_end() || current() != '=') {
                        // A call statement `obj.Method args`: parse (and ignore) the arguments.
                        skip_horizontal_whitespace();
                        while (!at_statement_end()) {
                            if (!parse_expression().has_value()) return false;
                            skip_horizontal_whitespace();
                            if (!consume(',')) break;
                            skip_horizontal_whitespace();
                        }
                        return true;
                    }
                    set_error("WFC0014", "expected assignment operator", offset_);
                    return false;
                }
                skip_horizontal_whitespace();
                return parse_expression().has_value();
            }
            set_error("WFC0106", "Invalid use of Nothing", base_offset);
            return false;
        }
        InstanceData& instance = *std::get<ObjectInstance>(base).data;
        const auto class_iterator = class_definitions_.find(instance.class_name);
        const ClassDef& class_def = class_iterator->second;

        // `obj.Prop(args).member = x` / `obj.Prop.member = x`: read the property or method
        // first, then write the member of the object it returns.
        if (!instance.fields.variables.contains(*member_name) &&
            !class_def.property_let.contains(*member_name) &&
            (class_def.property_get.contains(*member_name) ||
             class_def.methods.contains(*member_name))) {
            const auto after_member = offset_;
            skip_horizontal_whitespace();
            if (!at_end() && (current() == '.' || (current() == '(' && paren_followed_by_dot(offset_)))) {
                offset_ = member_offset;
                auto chained = parse_member_access_after_dot(
                    base, base_offset, /*require_function=*/true, via_interface_class);
                if (!chained.has_value()) {
                    return false;
                }
                skip_horizontal_whitespace();
                if (!at_end() && current() == '.') {
                    const std::string temp_name = "wfcchain" + std::to_string(chain_counter_++);
                    current_scope().variables.insert_or_assign(
                        temp_name,
                        std::holds_alternative<ObjectInstance>(*chained) ? *chained
                                                                         : Value{Nothing{}});
                    const bool chained_ok = parse_assignment_or_array_element(temp_name, '\0');
                    current_scope().variables.erase(temp_name);
                    return chained_ok;
                }
                set_error("WFC0014", "expected assignment operator", offset_);
                return false;
            }
            offset_ = after_member;
        }

        // REQ-0233: through an interface-typed reference the implementing
        // class's `Interface_Member` accessor is the target.
        if (!via_interface_class.empty()) {
            const std::string prefixed = via_interface_class + "_" + *member_name;
            const bool implements_interface = std::find(
                class_def.implements.begin(), class_def.implements.end(), via_interface_class) !=
                class_def.implements.end();
            if (implements_interface && class_def.property_let.contains(prefixed)) {
                return invoke_property_let_or_set(
                    instance, class_def, class_def.property_let.at(prefixed), prefixed,
                    member_offset);
            }
        }
        const auto letter_iterator = class_def.property_let.find(*member_name);
        if (letter_iterator != class_def.property_let.end()) {
            if (!member_accessible(class_def, letter_iterator->second.is_private)) {
                set_error("WFC0142", "member is not accessible outside its class", member_offset);
                return false;
            }
            return invoke_property_let_or_set(
                instance, class_def, letter_iterator->second, *member_name, member_offset);
        }

        const auto field_iterator = instance.fields.variables.find(*member_name);
        if (field_iterator == instance.fields.variables.end()) {
            set_error("WFC0135", "unknown member", member_offset);
            return false;
        }
        const auto field_def_iterator = class_def.fields.find(*member_name);
        if (field_def_iterator != class_def.fields.end() &&
            !member_accessible(class_def, field_def_iterator->second.is_private)) {
            set_error("WFC0142", "member is not accessible outside its class", member_offset);
            return false;
        }
        if (std::holds_alternative<ArrayValue>(field_iterator->second)) {
            skip_horizontal_whitespace();
            if (!at_end() && current() == '(') {
                return parse_array_element_assignment_on(field_iterator->second, member_offset);
            }
        }
        {
            // Chained write (`o.i.tag = 9`): the field holds the object the
            // next `.member` is written on, so recurse with it as the base.
            const auto chain_offset = offset_;
            skip_horizontal_whitespace();
            if (!at_end() && current() == '.') {
                const Value& next_base = field_iterator->second;
                if (!std::holds_alternative<ObjectInstance>(next_base) &&
                    !std::holds_alternative<Nothing>(next_base)) {
                    set_error(
                        "WFC0136", "member access requires an object reference", member_offset);
                    return false;
                }
                ++offset_;
                return parse_member_assignment(next_base, member_offset);
            }
            offset_ = chain_offset;
        }
        if (instance.fields.object_variables.contains(*member_name)) {
            const auto declared = instance.fields.object_class_names.find(*member_name);
            if (declared != instance.fields.object_class_names.end() &&
                is_udt_class(declared->second)) {
                skip_horizontal_whitespace();
                if (!consume('=')) {
                    set_error("WFC0014", "expected assignment operator", offset_);
                    return false;
                }
                skip_horizontal_whitespace();
                auto source = parse_expression();
                if (!source.has_value()) {
                    return false;
                }
                return assign_udt(field_iterator->second, *source, member_offset);
            }
            set_error("WFC0108", "object assignment requires Set", member_offset);
            return false;
        }
        skip_horizontal_whitespace();
        if (!consume('=')) {
            set_error("WFC0014", "expected assignment operator", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        if (instance.fields.variant_variables.contains(*member_name)) {
            if (execute_) {
                if (!terminate_if_last_reference(field_iterator->second)) {
                    return false;
                }
                field_iterator->second = copy_if_udt(std::move(*value));
            }
            return true;
        }
        if (!coerce_numeric_value(*value, field_iterator->second.index(), member_offset)) {
            return false;
        }
        if (field_iterator->second.index() != value->index()) {
            set_error("WFC0016", "assignment type mismatch", member_offset);
            return false;
        }
        if (execute_) {
            if (const auto fixed = instance.fields.fixed_string_lengths.find(*member_name);
                fixed != instance.fields.fixed_string_lengths.end()) {
                if (auto* text = std::get_if<std::string>(&*value)) {
                    fit_to_units(*text, fixed->second);
                }
            }
            field_iterator->second = std::move(*value);
        }
        return true;
    }

    // Dispatches a bare `identifier = ...`/`identifier(...) = ...`/
    // `identifier.member = ...` statement to array-element assignment,
    // member assignment, or ordinary scalar assignment, in that order,
    // based on what immediately follows `identifier`.
    [[nodiscard]] bool parse_assignment_or_array_element(
        std::string identifier,
        const char type_character = '\0') {
        if (type_character == '\0' && identifier == "me") {
            const auto identifier_offset = offset_ - identifier.size();
            const auto saved_offset = offset_;
            skip_horizontal_whitespace();
            if (!at_end() && current() == '.') {
                auto base = me_value(identifier_offset);
                if (!base.has_value()) {
                    return false;
                }
                advance();
                if (const auto* const me_instance = std::get_if<ObjectInstance>(&*base)) {
                    // `Me.Method args` with no `Call`.
                    const auto& me_class = class_definitions_.at(me_instance->data->class_name);
                    const auto peek_offset = offset_;
                    char peek_type_character{};
                    auto peek_member_name = parse_identifier(&peek_type_character);
                    skip_horizontal_whitespace();
                    const bool bare_statement_end = at_end() || current() == '\r' ||
                        current() == '\n' || current() == ':' || current() == '\'';
                    const bool arguments_follow = !bare_statement_end && current() != '=' &&
                        current() != '(' && current() != '.';
                    const bool is_method = peek_member_name.has_value() &&
                        peek_type_character == '\0' && me_class.methods.contains(*peek_member_name);
                    offset_ = peek_offset;
                    if ((bare_statement_end || arguments_follow) && is_method) {
                        bare_call_arguments_ = arguments_follow;
                        const auto result = parse_member_access_after_dot(
                            *base, identifier_offset, /*require_function=*/false);
                        return result.has_value();
                    }
                }
                return parse_member_assignment(*base, identifier_offset);
            }
            offset_ = saved_offset;
        }
        if (type_character == '\0') {
            const auto variable = find_variable(identifier);
            if (variable.value != nullptr &&
                std::holds_alternative<ArrayValue>(*variable.value)) {
                const auto saved_offset = offset_;
                skip_horizontal_whitespace();
                if (!at_end() && current() == '(') {
                    const auto identifier_offset = saved_offset - identifier.size();
                    return parse_array_element_assignment(identifier, identifier_offset);
                }
                offset_ = saved_offset;
            }
            if (variable.value != nullptr &&
                (std::holds_alternative<ObjectInstance>(*variable.value) ||
                 (!execute_ && std::holds_alternative<Nothing>(*variable.value)))) {
                // `obj(args).member ...` (for example `items(1).Name = x` on a Collection):
                // run the default-member call, then treat its object as the statement's base.
                {
                    const auto call_offset = offset_;
                    skip_horizontal_whitespace();
                    if (!at_end() && current() == '(' && paren_followed_by_dot(offset_)) {
                        offset_ = call_offset - identifier.size();
                        auto chained = parse_primary_base();
                        if (!chained.has_value()) {
                            return false;
                        }
                        skip_horizontal_whitespace();
                        if (at_end() || current() != '.') {
                            set_error("WFC0004", "unexpected trailing input", offset_);
                            return false;
                        }
                        const std::string temp_name = "wfcchain" + std::to_string(chain_counter_++);
                        current_scope().variables.insert_or_assign(
                            temp_name,
                            std::holds_alternative<ObjectInstance>(*chained) ? *chained
                                                                             : Value{Nothing{}});
                        const bool chained_ok = parse_assignment_or_array_element(temp_name, '\0');
                        current_scope().variables.erase(temp_name);
                        return chained_ok;
                    }
                    offset_ = call_offset;
                }
            }
            if (variable.value != nullptr && std::holds_alternative<ObjectInstance>(*variable.value)) {
                // `obj(args) = value` through the class's default member
                // (a Property Let).
                const auto saved_offset = offset_;
                skip_horizontal_whitespace();
                if (!at_end() && current() == '(') {
                    auto& instance_data = *std::get<ObjectInstance>(*variable.value).data;
                    const auto class_iterator = class_definitions_.find(instance_data.class_name);
                    if (class_iterator != class_definitions_.end() &&
                        !class_iterator->second.default_member.empty()) {
                        const auto& class_def = class_iterator->second;
                        const auto letter = class_def.property_let.find(class_def.default_member);
                        if (letter != class_def.property_let.end()) {
                            return invoke_property_let_or_set(
                                instance_data, class_def, letter->second, class_def.default_member,
                                saved_offset);
                        }
                    }
                }
                offset_ = saved_offset;
            }
            if (variable.value != nullptr &&
                (std::holds_alternative<Nothing>(*variable.value) ||
                 std::holds_alternative<ObjectInstance>(*variable.value))) {
                const auto saved_offset = offset_;
                skip_horizontal_whitespace();
                if (!at_end() && current() == '.') {
                    const auto identifier_offset = saved_offset - identifier.size();
                    const auto base = *variable.value;
                    advance();
                    // `obj.Method` with no `Call` and no parentheses (zero
                    // arguments; REQ-0217): checked only when nothing else
                    // follows the bare member name on this statement (no
                    // `=`, no `(`), so it never competes with `obj.Prop =
                    // expr` or `obj.Method(args) = ...` (an indexed
                    // Property Let/Set), both still handled by
                    // parse_member_assignment below.
                    if (const auto* const instance_base = std::get_if<ObjectInstance>(&base)) {
                        // REQ-0233: `obj`'s own declared class, so a
                        // bare `obj.Method` reaching an interface member
                        // is recognized here the same way `obj.Method`
                        // through `parse_primary`'s expression path
                        // already is -- checked against the
                        // interface-prefixed name below, not just the
                        // bare one.
                        std::string declared_interface_class;
                        const auto declared_class =
                            variable.scope->object_class_names.find(identifier);
                        if (declared_class != variable.scope->object_class_names.end()) {
                            declared_interface_class = declared_class->second;
                        }
                        const auto& instance_class_def =
                            class_definitions_.at(instance_base->data->class_name);
                        bool implements_interface = false;
                        for (const auto& implemented : instance_class_def.implements) {
                            if (implemented == declared_interface_class) {
                                implements_interface = true;
                                break;
                            }
                        }
                        const auto peek_offset = offset_;
                        char peek_type_character{};
                        auto peek_member_name = parse_identifier(&peek_type_character);
                        skip_horizontal_whitespace();
                        const bool bare_statement_end = at_end() || current() == '\r' ||
                            current() == '\n' || current() == ':' || current() == '\'';
                        const bool arguments_follow = !bare_statement_end && current() != '=' &&
                            current() != '(' && current() != '.';
                        const bool is_method = peek_member_name.has_value() &&
                            peek_type_character == '\0' &&
                            (instance_class_def.methods.contains(*peek_member_name) ||
                             (implements_interface &&
                              instance_class_def.methods.contains(
                                  declared_interface_class + "_" + *peek_member_name)));
                        ParenGroup paren_group;
                        if (is_method && !at_end() && current() == '(') {
                            paren_group = scan_statement_paren_group(offset_);
                        }
                        offset_ = peek_offset;
                        if (paren_group.ends_statement && is_method) {
                            bare_call_arguments_ = !paren_group.is_list;
                            const auto result = parse_member_access_after_dot(
                                base, identifier_offset, /*require_function=*/false,
                                declared_interface_class);
                            bare_call_arguments_ = false;
                            return result.has_value();
                        }
                        if ((bare_statement_end || arguments_follow) && is_method) {
                            bare_call_arguments_ = arguments_follow;
                            const auto result = parse_member_access_after_dot(
                                base, identifier_offset, /*require_function=*/false,
                                declared_interface_class);
                            return result.has_value();
                        }
                    }
                    const auto declared_for_assignment =
                        variable.scope->object_class_names.find(identifier);
                    return parse_member_assignment(
                        base, identifier_offset,
                        declared_for_assignment != variable.scope->object_class_names.end()
                            ? declared_for_assignment->second
                            : std::string{});
                }
                offset_ = saved_offset;
            }
        }
        return parse_assignment(std::move(identifier), type_character);
    }

    // The number of dimensions `array` currently has, or (REQ-0219) will
    // have once its first `ReDim` allocates it. Once allocated,
    // `dimensions` (or its absence, for an ordinary 1-D array) is
    // authoritative; before that, a dynamic array's pre-declared
    // `dynamic_dimension_count` (from a comma-only `Dim`, e.g. `Dim
    // arr(,)`) takes precedence when set, so an index-count check against
    // an as-yet-unallocated multi-dimensional array reports the array's
    // real expected count instead of always assuming 1-D.
    [[nodiscard]] static std::size_t array_expected_dimension_count(
        const ArrayValue& array) noexcept {
        if (!array.dimensions.empty()) {
            return array.dimensions.size();
        }
        if (!array.is_allocated && array.dynamic_dimension_count > 0U) {
            return array.dynamic_dimension_count;
        }
        return 1U;
    }

    // Parses a comma-separated index-expression list "i1, i2, ..." (the
    // caller has already consumed the opening '('), coercing each to
    // `Long`, and leaves `offset_` just past the matching ')'. Always runs
    // regardless of `execute_` so the parser advances correctly even
    // during a dry-run pass; range-checking and flat-offset computation
    // are the caller's job (via `array_flat_offset`), skipped during a dry
    // run the same way every other runtime check in this evaluator is.
    // Shared by `parse_array_index` (read) and
    // `parse_array_element_assignment` (write). REQ-0210.
    static constexpr std::size_t kAnyDimensionCount = static_cast<std::size_t>(-1);

    [[nodiscard]] std::optional<std::vector<std::pair<Integer, std::size_t>>> parse_index_list(
        const std::size_t dimension_count) {
        std::vector<std::pair<Integer, std::size_t>> indices;
        while (true) {
            skip_horizontal_whitespace();
            const auto index_offset = offset_;
            auto index_value = parse_expression();
            if (!index_value.has_value()) {
                return std::nullopt;
            }
            const auto index = coerce_long(*index_value, index_offset);
            if (!index.has_value()) {
                return std::nullopt;
            }
            indices.emplace_back(*index, index_offset);
            skip_horizontal_whitespace();
            if (!at_end() && current() == ',') {
                advance();
                continue;
            }
            break;
        }
        if (!consume(')')) {
            set_error("WFC0005", "expected closing parenthesis", offset_);
            return std::nullopt;
        }
        if (dimension_count != kAnyDimensionCount && indices.size() != dimension_count) {
            set_error(
                "WFC0115", "index count does not match array dimensions", indices.front().second);
            return std::nullopt;
        }
        return indices;
    }

    // Computes the flat storage offset for `indices` into `array` (already
    // count-matched by `parse_index_list`), range-checking each dimension
    // against its declared bounds. Only meaningful under real execution --
    // callers skip this during a dry run. REQ-0210.
    [[nodiscard]] std::optional<std::size_t> array_flat_offset(
        const ArrayValue& array, const std::vector<std::pair<Integer, std::size_t>>& indices) {
        if (array.dimensions.empty()) {
            const Integer upper_bound =
                array.lower_bound + static_cast<Integer>(array.elements.size()) - 1;
            if (indices[0].first < array.lower_bound || indices[0].first > upper_bound) {
                set_error("WFC0111", "array subscript out of range", indices[0].second);
                return std::nullopt;
            }
            return static_cast<std::size_t>(indices[0].first - array.lower_bound);
        }
        std::size_t flat_offset = 0;
        for (std::size_t dim = 0; dim < array.dimensions.size(); ++dim) {
            const auto& dimension_bound = array.dimensions[dim];
            const auto& index_entry = indices[dim];
            if (index_entry.first < dimension_bound.first ||
                index_entry.first > dimension_bound.second) {
                set_error("WFC0111", "array subscript out of range", index_entry.second);
                return std::nullopt;
            }
            const auto dimension_size =
                static_cast<std::size_t>(dimension_bound.second - dimension_bound.first) + 1U;
            flat_offset = flat_offset * dimension_size +
                static_cast<std::size_t>(index_entry.first - dimension_bound.first);
        }
        return flat_offset;
    }

    [[nodiscard]] bool parse_array_element_assignment(
        const std::string& identifier,
        const std::size_t identifier_offset) {
        const auto variable = find_variable(identifier);
        return parse_array_element_assignment_on(*variable.value, identifier_offset);
    }

    // Element assignment against an array held in `slot` (a variable or a
    // class-instance array field, REQ-0251); positioned at the '('.
    [[nodiscard]] bool parse_array_element_assignment_on(
        Value& slot, const std::size_t identifier_offset) {
        advance();  // consume '('
        auto& array = std::get<ArrayValue>(slot);
        const auto dimension_count = array_expected_dimension_count(array);
        auto indices = parse_index_list(dimension_count);
        if (!indices.has_value()) {
            return false;
        }
        // `v(0).Name = x`: write a member of the object a Variant element holds.
        if (array.is_variant_element && !at_end() && current() == '.') {
            Value base{Nothing{}};
            if (execute_) {
                const auto flat_offset = array_flat_offset(array, *indices);
                if (!flat_offset.has_value()) {
                    return false;
                }
                base = array.elements[*flat_offset];
                if (!std::holds_alternative<ObjectInstance>(base)) {
                    set_error("WFC0136", "member access requires an object reference",
                              identifier_offset);
                    return false;
                }
            }
            advance();
            return parse_member_assignment(base, identifier_offset);
        }
        // `jag(1)(2) = x`: assign into an array held by a Variant element.
        if (array.is_variant_element && !at_end() && current() == '(') {
            if (!execute_) {
                while (!at_end() && current() == '(') {
                    advance();
                    if (!parse_index_list(kAnyDimensionCount).has_value()) return false;
                }
                skip_horizontal_whitespace();
                if (!consume('=')) {
                    set_error("WFC0014", "expected assignment operator", offset_);
                    return false;
                }
                skip_horizontal_whitespace();
                return parse_expression().has_value();
            }
            const auto flat_offset = array_flat_offset(array, *indices);
            if (!flat_offset.has_value()) {
                return false;
            }
            Value& inner = array.elements[*flat_offset];
            if (!std::holds_alternative<ArrayValue>(inner)) {
                set_error("WFC0136", "indexing requires an array", identifier_offset);
                return false;
            }
            return parse_array_element_assignment_on(inner, identifier_offset);
        }
        // An Object-element array (REQ-0212) matches a scalar Object
        // variable's own rule: a plain `=` is rejected outright, requiring
        // `Set arr(i) = ...` instead (see parse_member_set_assignment's
        // array-element branch).
        if (array.is_object_element) {
            skip_horizontal_whitespace();
            const bool member_write = !at_end() && current() == '.';
            const bool udt_write = !member_write && is_udt_class(array.element_class_name);
            if (!member_write && !udt_write) {
                set_error("WFC0108", "object assignment requires Set", identifier_offset);
                return false;
            }
            Value* element = nullptr;
            if (execute_) {
                const auto flat_offset = array_flat_offset(array, *indices);
                if (!flat_offset.has_value()) {
                    return false;
                }
                element = &array.elements[*flat_offset];
            }
            Value placeholder{Nothing{}};
            if (member_write) {
                advance();
                if (element != nullptr) {
                    if (const auto* const instance_base = std::get_if<ObjectInstance>(element)) {
                        // `arr(i).Method args` with no `Call`: reaches
                        // the method directly, or through the array's
                        // declared interface (REQ-0233).
                        const auto& instance_class_def =
                            class_definitions_.at(instance_base->data->class_name);
                        const std::string& interface_name = array.element_class_name;
                        bool implements_interface = false;
                        for (const auto& implemented : instance_class_def.implements) {
                            if (implemented == interface_name) implements_interface = true;
                        }
                        const auto peek_offset = offset_;
                        char peek_type_character{};
                        auto peek_member_name = parse_identifier(&peek_type_character);
                        skip_horizontal_whitespace();
                        const bool bare_statement_end = at_end() || current() == '\r' ||
                            current() == '\n' || current() == ':' || current() == '\'';
                        const bool arguments_follow = !bare_statement_end && current() != '=' &&
                            current() != '(' && current() != '.';
                        const bool is_method = peek_member_name.has_value() &&
                            peek_type_character == '\0' &&
                            (instance_class_def.methods.contains(*peek_member_name) ||
                             (implements_interface &&
                              instance_class_def.methods.contains(
                                  interface_name + "_" + *peek_member_name)));
                        offset_ = peek_offset;
                        if ((bare_statement_end || arguments_follow) && is_method) {
                            bare_call_arguments_ = arguments_follow;
                            const auto base = *element;
                            const auto result = parse_member_access_after_dot(
                                base, identifier_offset, /*require_function=*/false,
                                interface_name);
                            return result.has_value();
                        }
                    }
                }
                return parse_member_assignment(
                    element != nullptr ? *element : placeholder, identifier_offset);
            }
            if (!consume('=')) {
                set_error("WFC0014", "expected assignment operator", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            auto source = parse_expression();
            if (!source.has_value()) {
                return false;
            }
            return assign_udt(element != nullptr ? *element : placeholder, *source, identifier_offset);
        }

        skip_horizontal_whitespace();
        if (!consume('=')) {
            set_error("WFC0014", "expected assignment operator", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }

        // A Variant-element array (REQ-0212) retypes freely per element,
        // the same as a scalar Variant variable -- no fixed-type
        // enforcement.
        if (!array.is_variant_element) {
            const auto element_type_index = array.element_type_index;
            if (!coerce_numeric_value(*value, element_type_index, identifier_offset)) {
                return false;
            }
            if (element_type_index != value->index()) {
                set_error("WFC0016", "assignment type mismatch", identifier_offset);
                return false;
            }
        }
        if (!execute_) {
            return true;
        }
        const auto flat_offset = array_flat_offset(array, *indices);
        if (!flat_offset.has_value()) {
            return false;
        }
        if (array.element_fixed_length != 0U) {
            if (auto* text = std::get_if<std::string>(&*value)) {
                fit_to_units(*text, array.element_fixed_length);
            }
        }
        array.elements[*flat_offset] = copy_if_udt(std::move(*value));
        return true;
    }

    [[nodiscard]] std::optional<Value> parse_expression() {
        return parse_implication();
    }

    [[nodiscard]] std::optional<Value> parse_implication() {
        auto left = parse_equivalence();
        if (!left.has_value()) {
            return std::nullopt;
        }
        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            if (!consume_keyword("imp")) {
                return left;
            }
            skip_horizontal_whitespace();
            auto right = parse_equivalence();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = logical_binary(*left, *right, 'I', operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_equivalence() {
        auto left = parse_exclusive_or();
        if (!left.has_value()) {
            return std::nullopt;
        }
        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            if (!consume_keyword("eqv")) {
                return left;
            }
            skip_horizontal_whitespace();
            auto right = parse_exclusive_or();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = logical_binary(*left, *right, 'E', operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_exclusive_or() {
        auto left = parse_or();
        if (!left.has_value()) {
            return std::nullopt;
        }
        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            if (!consume_keyword("xor")) {
                return left;
            }
            skip_horizontal_whitespace();
            auto right = parse_or();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = logical_binary(*left, *right, 'X', operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_or() {
        auto left = parse_and();
        if (!left.has_value()) {
            return std::nullopt;
        }
        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            if (!consume_keyword("or")) {
                return left;
            }
            skip_horizontal_whitespace();
            auto right = parse_and();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = logical_binary(*left, *right, 'O', operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_and() {
        auto left = parse_not();
        if (!left.has_value()) {
            return std::nullopt;
        }
        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            if (!consume_keyword("and")) {
                return left;
            }
            skip_horizontal_whitespace();
            auto right = parse_not();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = logical_binary(*left, *right, 'A', operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_not() {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        if (!consume_keyword("not")) {
            return parse_comparison();
        }
        skip_horizontal_whitespace();
        auto value = parse_not();
        if (!value.has_value()) {
            return std::nullopt;
        }
        if (const auto* integer = std::get_if<Integer>(&*value)) {
            return Value{static_cast<Integer>(~*integer)};
        }
        if (const auto* short_integer = std::get_if<Int16>(&*value)) {
            return Value{static_cast<Int16>(~*short_integer)};
        }
        if (const auto* byte = std::get_if<Byte>(&*value)) {
            return Value{static_cast<Byte>(~*byte)};
        }
        const auto operand = coerce_ternary_operand(*value, operator_offset);
        if (!operand.has_value()) {
            return std::nullopt;
        }
        if (operand->is_null) {
            return Value{Null{}};
        }
        return Value{!operand->value};
    }

    // VB `Like`: ? any char, * any run, # digit, [list]/[!list] with ranges.
    template <class Text>
    [[nodiscard]] static bool like_match_units(
        const Text& text, const Text& pattern, const bool fold) {
        using Unit = typename Text::value_type;
        const auto lower = [](const Unit c) -> Unit {
            if constexpr (sizeof(Unit) == 1U) {
                return static_cast<Unit>(ascii_lower(static_cast<char>(c)));
            } else {
                return static_cast<Unit>(unit_to_lower(c));
            }
        };
        const auto same = [&](const Unit a, const Unit b) {
            return fold ? lower(a) == lower(b) : a == b;
        };
        std::function<bool(std::size_t, std::size_t)> match = [&](std::size_t t, std::size_t p) {
            while (p < pattern.size()) {
                const Unit c = pattern[p];
                if (c == Unit{'*'}) {
                    while (p < pattern.size() && pattern[p] == Unit{'*'}) ++p;
                    if (p == pattern.size()) return true;
                    for (std::size_t k = t; k <= text.size(); ++k) {
                        if (match(k, p)) return true;
                    }
                    return false;
                }
                if (t >= text.size()) return false;
                if (c == Unit{'?'}) {
                    ++t; ++p;
                } else if (c == Unit{'#'}) {
                    if (!(text[t] >= Unit{'0'} && text[t] <= Unit{'9'})) return false;
                    ++t; ++p;
                } else if (c == Unit{'['}) {
                    const auto close = pattern.find(Unit{']'}, p + 2);
                    if (close == Text::npos) return false;
                    std::size_t q = p + 1;
                    bool negate = false;
                    if (q < close && pattern[q] == Unit{'!'}) { negate = true; ++q; }
                    bool hit = false;
                    while (q < close) {
                        if (q + 2 < close && pattern[q + 1] == Unit{'-'}) {
                            const Unit lo = fold ? lower(pattern[q]) : pattern[q];
                            const Unit hi = fold ? lower(pattern[q + 2]) : pattern[q + 2];
                            const Unit ch = fold ? lower(text[t]) : text[t];
                            if (ch >= lo && ch <= hi) hit = true;
                            q += 3;
                        } else {
                            if (same(pattern[q], text[t])) hit = true;
                            ++q;
                        }
                    }
                    if (hit == negate) return false;
                    ++t;
                    p = close + 1;
                } else {
                    if (!same(c, text[t])) return false;
                    ++t; ++p;
                }
            }
            return t == text.size();
        };
        return match(0, 0);
    }

    [[nodiscard]] static bool like_match(
        const std::string& text, const std::string& pattern, const bool fold) {
        if (is_ascii_text(text) && is_ascii_text(pattern)) {
            return like_match_units(text, pattern, fold);
        }
        return like_match_units(to_utf16_units(text), to_utf16_units(pattern), fold);
    }

    [[nodiscard]] std::optional<Value> parse_comparison() {
        auto left = parse_concatenation();
        if (!left.has_value()) {
            return std::nullopt;
        }

        while (true) {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        if (consume_keyword("is")) {
            skip_horizontal_whitespace();
            auto right = parse_concatenation();
            if (!right.has_value()) {
                return std::nullopt;
            }
            // `Is` compares two object references for identity: Nothing Is
            // Nothing is always True; two live instances are the same
            // object exactly when they share the same underlying
            // InstanceData (Value's variant-generated operator== already
            // does the right thing for both cases, since ObjectInstance's
            // own operator== compares the shared_ptr, not field contents).
            // Both operands must still be object references, matching real
            // VB6's requirement that Is only accepts object operands.
            if (!is_object_reference(*left) || !is_object_reference(*right)) {
                if (!execute_) {
                    left = Value{false};  // placeholder operand of a not-taken branch
                    continue;
                }
                set_error("WFC0107", "Is requires object operands", operator_offset);
                return std::nullopt;
            }
            left = Value{*left == *right};
            continue;
        }
        if (consume_keyword("like")) {
            skip_horizontal_whitespace();
            auto pattern = parse_concatenation();
            if (!pattern.has_value()) {
                return std::nullopt;
            }
            const auto* text = std::get_if<std::string>(&*left);
            const auto* mask = std::get_if<std::string>(&*pattern);
            if ((text == nullptr || mask == nullptr) && !execute_) {
                left = Value{false};
                continue;
            }
            if (text == nullptr || mask == nullptr) {
                set_error("WFC0018", "Like requires String operands", operator_offset);
                return std::nullopt;
            }
            left = Value{execute_ ? like_match(*text, *mask, option_compare_text_) : false};
            continue;
        }
        std::string_view operation;
        if (consume('=')) {
            operation = "=";
        } else if (consume('<')) {
            if (consume('=')) {
                operation = "<=";
            } else if (consume('>')) {
                operation = "<>";
            } else {
                operation = "<";
            }
        } else if (consume('>')) {
            operation = consume('=') ? ">=" : ">";
        } else {
            return left;
        }

        skip_horizontal_whitespace();
        auto right = parse_concatenation();
        if (!right.has_value()) {
            return std::nullopt;
        }
        left = compare(*left, *right, operation, operator_offset);
        if (!left.has_value()) {
            return std::nullopt;
        }
        }
    }

    [[nodiscard]] std::optional<Value> parse_concatenation() {
        auto left = parse_additive();
        if (!left.has_value()) {
            return std::nullopt;
        }

        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            if (!consume('&')) {
                return left;
            }
            skip_horizontal_whitespace();
            auto right = parse_additive();
            if (!right.has_value()) {
                return std::nullopt;
            }
            // Concatenating two Nulls raises "Invalid use of Null" (verified
            // against the local VB6 6.00.8176 reference); a single Null
            // operand instead concatenates as an empty string (also
            // verified: Null & "x" = "x", with no error).
            const bool left_null = std::holds_alternative<Null>(*left);
            const bool right_null = std::holds_alternative<Null>(*right);
            if (left_null && right_null) {
                set_error("WFC0104", "Invalid use of Null", operator_offset);
                return std::nullopt;
            }
            if (left_null || right_null) {
                left = Value{(left_null ? std::string{} : render(*left)) +
                             (right_null ? std::string{} : render(*right))};
                continue;
            }
            if (!resolve_default_value(*left, operator_offset) ||
                !resolve_default_value(*right, operator_offset)) {
                return std::nullopt;
            }
            if (is_object_reference(*left) || is_object_reference(*right) ||
                std::holds_alternative<ArrayValue>(*left) ||
                std::holds_alternative<ArrayValue>(*right)) {
                set_error(
                    "WFC0020", "concatenation requires String or Long operands", operator_offset);
                return std::nullopt;
            }
            if (auto* text = std::get_if<std::string>(&*left)) {
                if (const auto* tail = std::get_if<std::string>(&*right)) {
                    text->append(*tail);  // extend in place instead of copying `left` again
                    continue;
                }
            }
            left = Value{render(*left) + render(*right)};
        }
    }

    [[nodiscard]] std::optional<Value> parse_additive() {
        auto left = parse_multiplicative();
        if (!left.has_value()) {
            return std::nullopt;
        }

        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            char operation = '\0';
            if (consume('+')) {
                operation = '+';
            } else if (consume('-')) {
                operation = '-';
            } else {
                return left;
            }

            skip_horizontal_whitespace();
            auto right = parse_multiplicative();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = numeric_binary(*left, *right, operation, operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_star_slash() {
        auto left = parse_unary();
        if (!left.has_value()) {
            return std::nullopt;
        }

        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            char operation = '\0';
            bool integer_only = false;
            if (consume('*')) {
                operation = '*';
            } else if (consume('/')) {
                operation = '/';
            } else {
                return left;
            }

            skip_horizontal_whitespace();
            auto right = parse_unary();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = integer_only
                ? integer_binary(*left, *right, operation, operator_offset)
                : numeric_binary(*left, *right, operation, operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    // REQ-0244: VB precedence is `* /` > `\` > `Mod` > `+ -`.
    [[nodiscard]] std::optional<Value> parse_integer_division() {
        auto left = parse_star_slash();
        if (!left.has_value()) {
            return std::nullopt;
        }
        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            if (!consume('\\')) {
                return left;
            }
            skip_horizontal_whitespace();
            auto right = parse_star_slash();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = integer_binary(*left, *right, '\\', operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_multiplicative() {
        auto left = parse_integer_division();
        if (!left.has_value()) {
            return std::nullopt;
        }
        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            if (!consume_keyword("mod")) {
                return left;
            }
            skip_horizontal_whitespace();
            auto right = parse_integer_division();
            if (!right.has_value()) {
                return std::nullopt;
            }
            left = integer_binary(*left, *right, '%', operator_offset);
            if (!left.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_unary() {
        skip_horizontal_whitespace();
        const auto operator_offset = offset_;
        if (consume('+')) {
            auto value = parse_unary();
            if (!value.has_value()) {
                return std::nullopt;
            }
            if (const auto* flag = std::get_if<bool>(&*value)) {
                value = Value{static_cast<Int16>(*flag ? -1 : 0)};
            }
            // Unary +/- follow the same Null-propagates,
            // Empty-coerces-to-zero rule already verified for the binary
            // arithmetic operators.
            if (std::holds_alternative<Null>(*value)) {
                return value;
            }
            if (std::holds_alternative<Empty>(*value)) {
                return Value{Integer{0}};
            }
            if (!std::holds_alternative<double>(*value) &&
                !std::holds_alternative<float>(*value) &&
                !std::holds_alternative<Currency>(*value) &&
                !std::holds_alternative<Decimal>(*value) &&
                !std::holds_alternative<Int16>(*value) &&
                require_integer(*value, operator_offset) == nullptr) {
                return std::nullopt;
            }
            return value;
        }
        if (consume('-')) {
            skip_horizontal_whitespace();
            if (!at_end() &&
                (std::isdigit(static_cast<unsigned char>(current())) != 0 ||
                 (current() == '.' &&
                  std::isdigit(static_cast<unsigned char>(peek(1))) != 0))) {
                // `-2 ^ 2` is -(2 ^ 2): only a bare literal takes the
                // negative-literal path.
                const auto literal_start = offset_;
                const bool parsed_literal = parse_number().has_value();
                skip_horizontal_whitespace();
                const bool followed_by_power = parsed_literal && !at_end() && current() == '^';
                offset_ = literal_start;
                error_ = wfc::Evaluation{};
                if (!followed_by_power) {
                    return parse_negative_number();
                }
            }

            auto value = parse_unary();
            if (!value.has_value()) {
                return std::nullopt;
            }
            if (const auto* flag = std::get_if<bool>(&*value)) {
                value = Value{static_cast<Int16>(*flag ? -1 : 0)};
            }
            if (const auto* byte = std::get_if<Byte>(&*value)) {
                value = Value{static_cast<Int16>(*byte)};  // negated below as an Integer
            }
            if (std::holds_alternative<Null>(*value)) {
                return value;
            }
            if (std::holds_alternative<Empty>(*value)) {
                return execute_ ? Value{Integer{0}} : Value{Integer{}};
            }
            if (const auto* number = std::get_if<double>(&*value)) {
                return execute_ ? Value{-*number} : Value{0.0};
            }
            if (const auto* single = std::get_if<float>(&*value)) {
                return execute_ ? Value{-*single} : Value{0.0f};
            }
            if (const auto* decimal = std::get_if<Decimal>(&*value)) {
                if (!execute_) {
                    return Value{Decimal{}};
                }
                Decimal negated = *decimal;
                if (!is_zero_big(negated.mantissa)) {
                    negated.negative = !negated.negative;
                }
                return Value{negated};
            }
            if (const auto* currency = std::get_if<Currency>(&*value)) {
                if (!execute_) {
                    return Value{Currency{}};
                }
                if (currency->scaled == std::numeric_limits<std::int64_t>::min()) {
                    set_error("WFC0009", "numeric overflow", operator_offset);
                    return std::nullopt;
                }
                return Value{Currency{-currency->scaled}};
            }
            if (const auto* short_integer = std::get_if<Int16>(&*value)) {
                if (!execute_) {
                    return Value{Int16{}};
                }
                if (*short_integer == std::numeric_limits<Int16>::min()) {
                    set_error("WFC0009", "integer overflow", operator_offset);
                    return std::nullopt;
                }
                return Value{static_cast<Int16>(-*short_integer)};
            }
            if (const auto* byte = std::get_if<Byte>(&*value)) {
                return Value{static_cast<Integer>(-static_cast<Integer>(*byte))};
            }
            const auto* integer = require_integer(*value, operator_offset);
            if (integer == nullptr) {
                return std::nullopt;
            }
            if (!execute_) {
                return Value{Integer{}};
            }
            if (*integer == std::numeric_limits<Integer>::min()) {
                set_error("WFC0009", "integer overflow", operator_offset);
                return std::nullopt;
            }
            return Value{static_cast<Integer>(-*integer)};
        }
        return parse_power();
    }

    // REQ-0244: `a ^ b` (left-associative, binds tighter than unary minus).
    [[nodiscard]] std::optional<Value> parse_power() {
        auto left = parse_primary();
        if (!left.has_value()) {
            return std::nullopt;
        }
        while (true) {
            skip_horizontal_whitespace();
            const auto operator_offset = offset_;
            if (!consume('^')) {
                return left;
            }
            skip_horizontal_whitespace();
            bool negate = false;
            if (consume('-')) {
                negate = true;
                skip_horizontal_whitespace();
            } else {
                static_cast<void>(consume('+'));
                skip_horizontal_whitespace();
            }
            auto right = parse_primary();
            if (!right.has_value()) {
                return std::nullopt;
            }
            if (std::holds_alternative<Null>(*left) || std::holds_alternative<Null>(*right)) {
                left = Value{Null{}};
                continue;
            }
            const auto operand = [](const Value& v) -> std::optional<double> {
                if (std::holds_alternative<Empty>(v)) return 0.0;
                if (is_number(v)) return as_double(v);
                if (const auto* d = std::get_if<DateValue>(&v)) return d->serial;
                return std::nullopt;
            };
            const auto base = operand(*left);
            auto exponent = operand(*right);
            if (!base.has_value() || !exponent.has_value()) {
                set_error("WFC0007", "operator requires numeric operands", operator_offset);
                return std::nullopt;
            }
            if (negate) {
                exponent = -*exponent;
            }
            if (!execute_) {
                left = Value{0.0};
                continue;
            }
            if (*base == 0.0 && *exponent < 0.0) {
                set_error("WFC0008", "division by zero", operator_offset);
                return std::nullopt;
            }
            const double result = std::pow(*base, *exponent);
            if (!std::isfinite(result)) {
                set_error("WFC0009", "numeric overflow", operator_offset);
                return std::nullopt;
            }
            left = Value{result};
        }
    }

    // A class instantiation, either from `New ClassName` or (eagerly, a
    // documented simplification of VB6's lazy auto-instantiation semantics
    // for `Dim x As New ClassName`; see REQ-0203's Scope) from a `Dim`
    // declaration. Every declared field is initialized to its type's zero
    // value, matching a fixed-type variable's own default, before
    // `Class_Initialize` (if the class declares one) runs against the new,
    // fully field-initialized instance.
    // The size in bytes of a UDT as stored in a Binary/Random file (and
    // reported by `Len`): fixed strings are their length, a variable String
    // is its 2-byte descriptor plus text, scalars are their natural width.
    [[nodiscard]] std::size_t udt_byte_size(const Value& value) const {
        if (std::holds_alternative<Integer>(value)) return 4;
        if (std::holds_alternative<Int16>(value) || std::holds_alternative<bool>(value)) return 2;
        if (std::holds_alternative<Byte>(value)) return 1;
        if (std::holds_alternative<float>(value)) return 4;
        if (std::holds_alternative<double>(value) || std::holds_alternative<Currency>(value) ||
            std::holds_alternative<DateValue>(value)) {
            return 8;
        }
        if (const auto* array = std::get_if<ArrayValue>(&value)) {
            std::size_t total = 0;
            for (const auto& element : array->elements) total += udt_byte_size(element);
            return total;
        }
        if (const auto* instance = std::get_if<ObjectInstance>(&value)) {
            const auto class_iterator = class_definitions_.find(instance->data->class_name);
            if (class_iterator == class_definitions_.end()) return 0;
            std::size_t total = 0;
            for (const auto& name : class_iterator->second.field_order) {
                const auto field = instance->data->fields.variables.find(name);
                if (field == instance->data->fields.variables.end()) continue;
                if (const auto* text = std::get_if<std::string>(&field->second)) {
                    total += instance->data->fields.fixed_string_lengths.contains(name)
                        ? text->size()
                        : 2U + text->size();
                } else {
                    total += udt_byte_size(field->second);
                }
            }
            return total;
        }
        if (const auto* text = std::get_if<std::string>(&value)) return text->size();
        return 16;  // Variant / Decimal
    }

    [[nodiscard]] std::optional<Value> instantiate_class(
        const std::string& class_name, const std::size_t offset) {
        const auto class_iterator = class_definitions_.find(class_name);
        if (class_iterator == class_definitions_.end()) {
            set_error("WFC0134", "unknown class name", offset);
            return std::nullopt;
        }
        if (constant_expression_) {
            set_error(
                "WFC0074", "constant initializer cannot call a procedure", offset);
            return std::nullopt;
        }
        auto instance = std::make_shared<InstanceData>();
        instance->class_name = class_name;
        for (const auto& [constant_name, constant_value] : class_iterator->second.constants) {
            instance->fields.variables.emplace(constant_name, constant_value);
            instance->fields.constants.insert(constant_name);
        }
        for (const auto& [field_name, field_def] : class_iterator->second.fields) {
            Value initial_value;
            if (field_def.is_array) {
                Value element_default = field_def.is_variant ? Value{Empty{}}
                    : field_def.is_object                    ? Value{Nothing{}}
                                                             : array_element_default(field_def.type_index);
                const std::size_t element_type_index = element_default.index();
                ArrayValue array{
                    /*elements=*/{}, /*lower_bound=*/0, /*is_dynamic=*/field_def.dimensions.empty(),
                    /*is_allocated=*/!field_def.dimensions.empty(), element_type_index,
                    field_def.dimensions.size() > 1U ? field_def.dimensions
                                                     : std::vector<std::pair<Integer, Integer>>{},
                    field_def.is_variant, field_def.is_object, field_def.class_name};
                if (!field_def.dimensions.empty()) {
                    std::size_t total = 1U;
                    for (const auto& dimension : field_def.dimensions) {
                        total *= static_cast<std::size_t>(dimension.second - dimension.first) + 1U;
                    }
                    array.lower_bound = field_def.dimensions.front().first;
                    const bool udt_elements = field_def.is_object &&
                        !field_def.class_name.empty() && is_udt_class(field_def.class_name);
                    for (std::size_t i = 0; i < total; ++i) {
                        if (udt_elements) {
                            auto nested = instantiate_class(field_def.class_name, offset);
                            if (!nested.has_value()) {
                                return std::nullopt;
                            }
                            array.elements.push_back(std::move(*nested));
                        } else {
                            array.elements.push_back(element_default);
                        }
                    }
                }
                instance->fields.variables.emplace(field_name, Value{std::move(array)});
                continue;
            }
            if (field_def.is_object && !field_def.class_name.empty() &&
                is_udt_class(field_def.class_name)) {
                auto nested = instantiate_class(field_def.class_name, offset);
                if (!nested.has_value()) {
                    return std::nullopt;
                }
                initial_value = std::move(*nested);
            } else if (field_def.is_object && field_def.auto_new && !field_def.class_name.empty()) {
                auto nested = instantiate_class(field_def.class_name, offset);
                if (!nested.has_value()) {
                    return std::nullopt;
                }
                initial_value = std::move(*nested);
            } else if (field_def.is_object) {
                initial_value = Value{Nothing{}};
            } else if (field_def.is_variant) {
                initial_value = Value{Empty{}};
            } else if (field_def.fixed_length != 0U) {
                initial_value = Value{std::string(field_def.fixed_length, ' ')};
                instance->fields.fixed_string_lengths[field_name] = field_def.fixed_length;
            } else {
                initial_value = zero_value_for_index(field_def.type_index);
            }
            instance->fields.variables.emplace(field_name, std::move(initial_value));
            if (field_def.is_variant) {
                instance->fields.variant_variables.insert(field_name);
            } else if (field_def.is_object) {
                instance->fields.object_variables.insert(field_name);
                if (!field_def.class_name.empty()) {
                    instance->fields.object_class_names.emplace(field_name, field_def.class_name);
                }
            }
        }
        const auto initializer_iterator = class_iterator->second.methods.find("class_initialize");
        if (initializer_iterator != class_iterator->second.methods.end()) {
            // invoke_definition's own !execute_ short-circuit already skips
            // actually running the body during a dry-run/type-check-only
            // pass (an unreached If branch, ...), so New still allocates a
            // correctly-typed placeholder instance there without invoking
            // any Class_Initialize side effect.
            if (!invoke_definition(
                     initializer_iterator->second, "class_initialize", {}, offset,
                     class_iterator->second.source, instance.get())
                     .has_value()) {
                return std::nullopt;
            }
        }
        return Value{ObjectInstance{std::move(instance)}};
    }

    // Wraps parse_primary_base with postfix `.member`/`.member(args)`
    // handling, applied in a loop so a member that itself evaluates to an
    // object reference (a Variant-typed field or a Variant-returning
    // Function/Property Get holding one -- this evaluator's only routes to
    // an object-valued result besides a plain variable, since a dedicated
    // "As ClassName" method/property return type is deferred; see REQ-0203's
    // Scope) chains further (`a.b.c`), without every parse_primary_base exit
    // path needing to know about member access itself.
    [[nodiscard]] std::optional<Value> parse_primary() {
        const auto base_offset = offset_;
        // REQ-0233: if the very next primary is a bare identifier naming a
        // variable with a specific declared class (REQ-0203/REQ-0228's
        // `object_class_names`), remember that class name so the *first*
        // `.member` access immediately following it can dispatch through
        // that class's own `Implements` list (see
        // `parse_member_access_after_dot`'s own `via_interface_class`)
        // when it names an interface the live instance's actual class
        // implements. A speculative lookahead-then-rewind:
        // `parse_primary_base` below still does the real parse of
        // whatever this turns out to be, unaffected by this peek.
        std::string declared_interface_class;
        {
            const auto lookahead_offset = offset_;
            skip_horizontal_whitespace();
            char lookahead_type_character{};
            auto lookahead_identifier = parse_identifier(&lookahead_type_character);
            if (lookahead_identifier.has_value() && lookahead_type_character == '\0') {
                const auto variable = find_variable(*lookahead_identifier);
                if (variable.value != nullptr) {
                    const auto declared_class =
                        variable.scope->object_class_names.find(*lookahead_identifier);
                    if (declared_class != variable.scope->object_class_names.end()) {
                        declared_interface_class = declared_class->second;
                    } else if (const auto* array = std::get_if<ArrayValue>(variable.value)) {
                        // An array of interface-typed references dispatches
                        // through the element class the same way.
                        if (array->is_object_element) {
                            declared_interface_class = array->element_class_name;
                        }
                    }
                }
            }
            offset_ = lookahead_offset;
        }
        auto value = parse_primary_base();
        if (!value.has_value()) {
            return std::nullopt;
        }
        bool is_first_member_access = true;
        while (true) {
            // `jag(2)(1)`, `Split(s)(0)`: index the array a previous primary produced.
            if (!at_end() && current() == '(' && std::holds_alternative<ArrayValue>(*value)) {
                value = parse_array_index(*value);
                if (!value.has_value()) {
                    return std::nullopt;
                }
                continue;
            }
            if (!execute_ && !at_end() && current() == '(' && !is_object_reference(*value)) {
                // A not-taken branch: the placeholder stands for an array element's array.
                advance();
                if (!parse_index_list(kAnyDimensionCount).has_value()) {
                    return std::nullopt;
                }
                value = Value{Empty{}};
                continue;
            }
            skip_horizontal_whitespace();
            if (at_end() || current() != '.') {
                return value;
            }
            advance();
            value = parse_member_access_after_dot(
                std::move(*value), base_offset, /*require_function=*/true,
                is_first_member_access ? declared_interface_class : std::string{});
            is_first_member_access = false;
            if (!value.has_value()) {
                return std::nullopt;
            }
        }
    }

    [[nodiscard]] std::optional<Value> parse_primary_base() {
        skip_horizontal_whitespace();
        if (at_end() || current() == '\r' || current() == '\n' || current() == ':' ||
            current() == '\'') {
            set_error("WFC0002", "expected expression", offset_);
            return std::nullopt;
        }

        if (current() == '"') {
            return parse_string();
        }
        if (current() == '&' && (ascii_lower(peek(1)) == 'h' || ascii_lower(peek(1)) == 'o')) {
            // REQ-0244: `&HFF` / `&O17` (optionally `&`-suffixed for Long).
            const bool hex = ascii_lower(peek(1)) == 'h';
            const auto literal_offset = offset_;
            offset_ += 2;
            std::uint64_t magnitude = 0;
            std::size_t digits = 0;
            while (!at_end()) {
                const char c = ascii_lower(current());
                int digit = -1;
                if (c >= '0' && c <= '9') digit = c - '0';
                else if (hex && c >= 'a' && c <= 'f') digit = c - 'a' + 10;
                if (digit < 0 || (!hex && digit > 7)) break;
                magnitude = magnitude * (hex ? 16U : 8U) + static_cast<std::uint64_t>(digit);
                if (magnitude > 0xFFFFFFFFULL) {
                    set_error("WFC0006", "integer literal out of range", literal_offset);
                    return std::nullopt;
                }
                ++digits;
                advance();
            }
            if (digits == 0) {
                set_error("WFC0006", "invalid numeric literal", literal_offset);
                return std::nullopt;
            }
            const bool long_suffix = !at_end() && current() == '&';
            if (long_suffix) {
                advance();
            }
            if (!long_suffix && magnitude <= 0xFFFFULL) {
                return Value{static_cast<Int16>(static_cast<std::uint16_t>(magnitude))};
            }
            return Value{static_cast<Integer>(static_cast<std::uint32_t>(magnitude))};
        }
        if (current() == '#') {
            // REQ-0242: a Date literal `#m/d/yyyy h:mm:ss AM#`.
            const auto literal_offset = offset_;
            {
                // `#3` in an argument list (`Input(5, #1)`) is a file number.
                std::size_t look = offset_ + 1;
                while (look < source_.size() &&
                       std::isdigit(static_cast<unsigned char>(source_[look])) != 0) {
                    ++look;
                }
                std::size_t after = look;
                while (after < source_.size() && (source_[after] == ' ' || source_[after] == '\t')) ++after;
                if (look > offset_ + 1 &&
                    (after >= source_.size() || source_[after] == ')' || source_[after] == ',')) {
                    Integer file_number = 0;
                    for (std::size_t k = offset_ + 1; k < look; ++k) {
                        file_number = file_number * 10 + (source_[k] - '0');
                    }
                    offset_ = look;
                    return Value{file_number};
                }
            }
            advance();
            const auto text_start = offset_;
            while (!at_end() && current() != '#' && current() != '\r' && current() != '\n') {
                advance();
            }
            if (at_end() || current() != '#') {
                set_error("WFC0006", "unterminated Date literal", literal_offset);
                return std::nullopt;
            }
            const auto parsed = parse_date_text(source_.substr(text_start, offset_ - text_start));
            advance();
            if (!parsed.has_value()) {
                set_error("WFC0006", "invalid Date literal", literal_offset);
                return std::nullopt;
            }
            return Value{DateValue{*parsed}};
        }
        if (std::isdigit(static_cast<unsigned char>(current())) != 0 ||
            (current() == '.' &&
             std::isdigit(static_cast<unsigned char>(peek(1))) != 0)) {
            return parse_number();
        }
        if (consume_keyword("true")) {
            return Value{true};
        }
        if (consume_keyword("false")) {
            return Value{false};
        }
        if (consume_keyword("null")) {
            return Value{Null{}};
        }
        if (consume_keyword("empty")) {
            return Value{Empty{}};
        }
        if (consume_keyword("nothing")) {
            return Value{Nothing{}};
        }
        {
            const auto me_offset = offset_;
            if (consume_keyword("me")) {
                return me_value(me_offset);
            }
        }
        {
            // REQ-0250: `TypeOf obj Is ClassName` and `Erl`.
            const auto typeof_offset = offset_;
            if (consume_keyword("typeof")) {
                skip_horizontal_whitespace();
                auto operand = parse_concatenation();
                if (!operand.has_value()) {
                    return std::nullopt;
                }
                skip_horizontal_whitespace();
                if (!consume_keyword("is")) {
                    set_error("WFC0010", "expected Is after TypeOf operand", offset_);
                    return std::nullopt;
                }
                skip_horizontal_whitespace();
                const auto class_offset = offset_;
                auto class_name = parse_identifier();
                if (!class_name.has_value()) {
                    set_error("WFC0011", "expected class name after Is", class_offset);
                    return std::nullopt;
                }
                if (*class_name != "object" && !class_definitions_.contains(*class_name)) {
                    set_error("WFC0134", "unknown class name", class_offset);
                    return std::nullopt;
                }
                if (!is_object_reference(*operand)) {
                    set_error("WFC0107", "TypeOf requires an object operand", typeof_offset);
                    return std::nullopt;
                }
                const auto* instance = std::get_if<ObjectInstance>(&*operand);
                if (instance == nullptr || !execute_) {
                    return Value{false};
                }
                return Value{*class_name == "object" ||
                             class_satisfies(instance->data->class_name, *class_name)};
            }
            if (consume_keyword("erl")) {
                return Value{erl_};
            }
        }
        {
            const auto err_start = offset_;
            if (consume_keyword("err")) {
                if (consume('.')) {
                    const auto member_offset = offset_;
                    const auto member = parse_identifier();
                    if (member == "number") {
                        return Value{err_number_};
                    }
                    if (member == "description") {
                        return Value{err_description_};
                    }
                    if (member == "source") {
                        return Value{err_source_};
                    }
                    if (member == "helpfile") {
                        return Value{std::string{}};
                    }
                    if (member == "helpcontext" || member == "lastdllerror") {
                        return Value{Integer{0}};
                    }
                    set_error("WFC0135", "unknown member", member_offset);
                    return std::nullopt;
                }
                offset_ = err_start;
            }
        }
        if (consume_keyword("new")) {
            skip_horizontal_whitespace();
            const auto class_name_offset = offset_;
            char class_type_character{};
            auto class_name = parse_identifier(&class_type_character);
            if (!class_name.has_value() || class_type_character != '\0') {
                set_error("WFC0011", "expected class name after New", class_name_offset);
                return std::nullopt;
            }
            return instantiate_class(*class_name, class_name_offset);
        }
        if (consume('(')) {
            auto value = parse_expression();
            if (!value.has_value()) {
                return std::nullopt;
            }
            skip_horizontal_whitespace();
            if (!consume(')')) {
                set_error("WFC0005", "expected closing parenthesis", offset_);
                return std::nullopt;
            }
            return value;
        }
        if (is_identifier_start(current()) || at_with_member()) {
            const auto identifier_offset = offset_;
            char type_character{};
            auto identifier = parse_identifier(&type_character);
            skip_horizontal_whitespace();
            if (!at_end() && current() == '(') {
                const auto array_variable = find_variable(*identifier);
                if (array_variable.value != nullptr &&
                    std::holds_alternative<ArrayValue>(*array_variable.value)) {
                    if (type_character != '\0') {
                        set_error(
                            "WFC0016",
                            "identifier type-declaration character mismatch",
                            identifier_offset);
                        return std::nullopt;
                    }
                    return parse_array_index(*array_variable.value);
                }
                if (array_variable.value != nullptr && type_character == '\0') {
                    // REQ-0253/0257: a class's default member, `obj(1)`.
                    if (const auto* holder = std::get_if<ObjectInstance>(array_variable.value)) {
                        const auto class_iterator =
                            class_definitions_.find(holder->data->class_name);
                        if (class_iterator != class_definitions_.end() &&
                            !class_iterator->second.default_member.empty()) {
                            const auto& class_def = class_iterator->second;
                            const auto& member = class_def.default_member;
                            if (class_def.methods.contains(member)) {
                                return call_class_method(
                                    *holder->data, class_def, member, identifier_offset,
                                    /*require_function=*/true);
                            }
                            const auto getter = class_def.property_get.find(member);
                            if (getter != class_def.property_get.end()) {
                                auto arguments = parse_call_argument_list();
                                if (!arguments.has_value()) {
                                    return std::nullopt;
                                }
                                return invoke_definition(
                                    getter->second, member, std::move(*arguments),
                                    identifier_offset, class_def.source, holder->data.get());
                            }
                        }
                    }
                }
                if (array_variable.value != nullptr && type_character == '\0' &&
                    std::holds_alternative<Nothing>(*array_variable.value) &&
                    !procedures_.contains(*identifier) &&
                    !(current_class_def() != nullptr &&
                      (current_class_def()->methods.contains(*identifier) ||
                       current_class_def()->property_get.contains(*identifier)))) {
                    // `obj(args)` through an unset Object variable.
                    auto ignored = parse_call_argument_list();
                    if (!ignored.has_value()) {
                        return std::nullopt;
                    }
                    if (execute_) {
                        static_cast<void>(raise_runtime(
                            91, "Object variable or With block variable not set", identifier_offset));
                        return std::nullopt;
                    }
                    return Value{Empty{}};
                }
                if (procedures_.contains(*identifier) &&
                    (type_character == '\0' || procedures_.at(*identifier).is_function)) {
                    return parse_procedure_call(*identifier, identifier_offset);
                }
                // An unqualified call to a sibling method of the class
                // currently executing (including a method calling itself,
                // for recursion) -- the implicit-Me equivalent of
                // `Me.Method(args)`, which this evaluator does not require
                // spelling out since a class method never sees a same-named
                // module-level procedure anyway (its scope is isolated from
                // the module; see find_variable).
                if (type_character == '\0') {
                    if (auto* const instance = current_instance()) {
                        if (const auto* const class_def = current_class_def()) {
                            if (class_def->methods.contains(*identifier)) {
                                return call_class_method(
                                    *instance, *class_def, *identifier, identifier_offset,
                                    /*require_function=*/true);
                            }
                            // An unqualified indexed Property Get read
                            // (REQ-0205): `Item(0)` reaches the same
                            // parenthesized shape a sibling method call
                            // would.
                            const auto getter_iterator =
                                class_def->property_get.find(*identifier);
                            if (getter_iterator != class_def->property_get.end()) {
                                auto arguments = parse_call_argument_list();
                                if (!arguments.has_value()) {
                                    return std::nullopt;
                                }
                                return invoke_definition(
                                    getter_iterator->second, *identifier, std::move(*arguments),
                                    identifier_offset, class_def->source, instance);
                            }
                        }
                    }
                }
                if (type_character != '\0') {
                    identifier->push_back(type_character);
                }
                return parse_function_call(*identifier, identifier_offset);
            }
            // A declared variable shadows the built-in constants.
            const bool names_variable =
                allow_identifiers_ && find_variable_raw(*identifier).value != nullptr;
            if (!names_variable) {
                if (const auto global = global_class_constants_.find(*identifier);
                    global != global_class_constants_.end() && type_character == '\0') {
                    return global->second;
                }
                if (const auto constant = vba_constant_value(*identifier)) {
                    Value value{*constant};
                    if (!type_character_matches(value, type_character, identifier_offset)) {
                        return std::nullopt;
                    }
                    return value;
                }
                if (auto text = vba_string_constant(*identifier)) {
                    Value value{std::move(*text)};
                    if (!type_character_matches(value, type_character, identifier_offset)) {
                        return std::nullopt;
                    }
                    return value;
                }
            }
            if (!allow_identifiers_) {
                set_error("WFC0002", "expected expression", identifier_offset);
                return std::nullopt;
            }
            const auto variable = find_variable(*identifier);
            if (variable.value == nullptr) {
                // An unqualified read of the current class's own Property
                // Get (no parentheses, like a field read), the same
                // implicit-Me convenience as the method-call branch above.
                // Checked only once find_variable has already found nothing
                // -- a local parameter/variable of the same name (a Property
                // Let's own value parameter commonly shares its property's
                // name, e.g. `Property Let V(v As Long)`) must shadow it,
                // matching ordinary lexical scoping.
                if (type_character == '\0') {
                    if (auto* const instance = current_instance()) {
                        if (const auto* const class_def = current_class_def()) {
                            const auto getter_iterator =
                                class_def->property_get.find(*identifier);
                            if (getter_iterator != class_def->property_get.end()) {
                                return invoke_definition(
                                    getter_iterator->second, *identifier, {}, identifier_offset,
                                    class_def->source, instance);
                            }
                        }
                    }
                }
                if (type_character == '\0' && *identifier == "app" &&
                    class_definitions_.contains("wfcapp")) {
                    if (!app_instance_.has_value()) {
                        app_instance_ = instantiate_class("wfcapp", identifier_offset);
                        if (!app_instance_.has_value()) {
                            return std::nullopt;
                        }
                    }
                    return *app_instance_;
                }
                // A parenthesis-free, zero-argument call to a known
                // module-level Function or class-sibling Function/method
                // (REQ-0213) -- e.g. bare `Print NextId`. Checked only
                // once every variable/Property-Get possibility above has
                // found nothing, so a local name always shadows a
                // same-named procedure, matching ordinary lexical scoping.
                if (procedures_.contains(*identifier) &&
                    (type_character == '\0' || procedures_.at(*identifier).is_function)) {
                    return parse_procedure_call(*identifier, identifier_offset);
                }
                if (type_character == '\0') {
                    if (auto* const instance = current_instance()) {
                        if (const auto* const class_def = current_class_def()) {
                            if (class_def->methods.contains(*identifier)) {
                                return call_class_method(
                                    *instance, *class_def, *identifier, identifier_offset,
                                    /*require_function=*/true);
                            }
                        }
                    }
                }
                // A parenthesis-free, zero-argument intrinsic function call
                // (REQ-0213) -- e.g. bare `Print Rnd`. parse_function_call
                // itself reports WFC0071 "unsupported function" for any
                // name it does not recognize, before consuming any input;
                // that specific, no-input-consumed failure is
                // indistinguishable from "this was never a function-call
                // attempt at all", so it falls through to the ordinary
                // undeclared-variable diagnostic below instead of
                // surfacing the less helpful "unsupported function"
                // message for what is more likely a typo'd variable name.
                {
                    std::string call_identifier = *identifier;
                    if (type_character != '\0') {
                        call_identifier.push_back(type_character);
                    }
                    const auto saved_offset = offset_;
                    auto result = parse_function_call(call_identifier, identifier_offset);
                    if (result.has_value() || offset_ != saved_offset ||
                        error_.diagnostic.rfind("WFC0071 ", 0) != 0) {
                        return result;
                    }
                    error_ = wfc::Evaluation{};
                }
                if (!strict_declarations_ && type_character == '\0' && !constant_expression_ &&
                    !in_with_identifier(*identifier)) {
                    return Value{Empty{}};  // REQ-0265: an undeclared name reads as Empty
                }
                set_error("WFC0015", "undeclared variable", identifier_offset);
                return std::nullopt;
            }
            if (!type_character_matches(*variable.value, type_character, identifier_offset)) {
                return std::nullopt;
            }
            if (constant_expression_ && !variable.scope->constants.contains(*identifier)) {
                set_error(
                    "WFC0064",
                    "constant initializer cannot reference a variable",
                    identifier_offset);
                return std::nullopt;
            }
            if (variable.scope->variant_variables.contains(*identifier)) {
                variant_operand_seen_ = true;
                if (std::holds_alternative<std::string>(*variable.value)) {
                    variant_string_seen_ = true;
                } else if (is_number(*variable.value)) {
                    variant_number_seen_ = true;
                }
            }
            return *variable.value;
        }

        set_error("WFC0002", "expected expression", offset_);
        return std::nullopt;
    }

    // Reads `array_variable(index)`. `array_variable` is the array's
    // current Value (a reference into `variables_`, stable across this call
    // since expression parsing never inserts into that map).
    // The zero/default value for one of the fixed scalar type indices a
    // procedure parameter or Function return type can name (mirrors
    // `parse_type_keyword`'s non-Variant results).
    [[nodiscard]] static Value zero_value_for_index(const std::size_t type_index) {
        if (type_index == Value{Integer{}}.index()) {
            return Value{Integer{}};
        }
        if (type_index == Value{Int16{}}.index()) {
            return Value{Int16{}};
        }
        if (type_index == Value{0.0}.index()) {
            return Value{0.0};
        }
        if (type_index == Value{0.0f}.index()) {
            return Value{0.0f};
        }
        if (type_index == Value{Currency{}}.index()) {
            return Value{Currency{}};
        }
        if (type_index == Value{std::string{}}.index()) {
            return Value{std::string{}};
        }
        if (type_index == Value{DateValue{}}.index()) {
            return Value{DateValue{}};
        }
        if (type_index == Value{Byte{}}.index()) {
            return Value{Byte{}};
        }
        if (type_index == Value{Decimal{}}.index()) {
            return Value{Decimal{}};
        }
        // REQ-0228: an omitted Optional object-reference parameter with no
        // explicit `= Nothing` default (`parameter.has_default` false)
        // reaches here via `parameter.type_index` -- `Nothing` is that
        // type's own zero value, not the `Boolean` fallback below.
        if (type_index == Value{Nothing{}}.index()) {
            return Value{Nothing{}};
        }
        return Value{false};
    }

    // Parses one call argument. A bare identifier naming a declared
    // variable, with nothing else in its own argument slot, is captured as
    // a possible ByRef target; anything else (a literal, an operator
    // expression, `Not x`, `arr(i)`, an intrinsic/procedure call, ...)
    // parses as an ordinary expression with no write-back target.
    [[nodiscard]] std::optional<CallArgument> parse_call_argument() {
        skip_horizontal_whitespace();
        if (!at_end() && is_identifier_start(current())) {
            // `name:=value`
            const auto before_name = offset_;
            char name_type_character{};
            auto argument_name = parse_identifier(&name_type_character);
            skip_horizontal_whitespace();
            if (argument_name.has_value() && name_type_character == '\0' && !at_end() &&
                current() == ':' && peek(1) == '=') {
                offset_ += 2;
                auto named = parse_call_argument();
                if (named.has_value()) {
                    named->name = std::move(*argument_name);
                }
                return named;
            }
            offset_ = before_name;
        }
        if (!at_end() && (is_identifier_start(current()) || at_with_member())) {
            const auto saved_offset = offset_;
            char type_character{};
            auto identifier = parse_identifier(&type_character);
            skip_horizontal_whitespace();
            const bool bare_candidate = type_character == '\0' &&
                (at_end() || current() == ',' || current() == ')' || current() == '\r' ||
                 current() == '\n' || current() == ':' || current() == '\'');
            if (bare_candidate) {
                const auto variable = find_variable(*identifier);
                if (variable.value != nullptr) {
                    return CallArgument{*variable.value, variable.value};
                }
            } else if (type_character == '\0' && execute_ && !at_end() &&
                       (current() == '(' || current() == '.')) {
                // `arr(i)`, `rec.Field`, `objs(i).Field` alone in an argument slot are
                // ByRef targets too.
                const auto variable = find_variable(*identifier);
                if (variable.value != nullptr &&
                    (std::holds_alternative<ArrayValue>(*variable.value) ||
                     std::holds_alternative<ObjectInstance>(*variable.value))) {
                    const auto saved_error = error_;
                    offset_ = saved_offset;
                    LValue target;
                    if (parse_lvalue_path(target)) {
                        skip_horizontal_whitespace();
                        if (target.ptr != nullptr &&
                            (at_end() || current() == ',' || current() == ')' ||
                             current() == '\r' || current() == '\n' || current() == ':' ||
                             current() == '\'')) {
                            return CallArgument{*target.ptr, target.ptr};
                        }
                    }
                    error_ = saved_error;
                }
            }
            offset_ = saved_offset;
        }
        auto value = parse_expression();
        if (!value.has_value()) {
            return std::nullopt;
        }
        return CallArgument{std::move(*value), nullptr};
    }

    // Runs statements from the current offset_ until reaching `body_end`
    // (a procedure's own "End Sub"/"End Function" position, as recorded by
    // `scan_procedures`) or an Exit Sub/Exit Function. Mirrors `evaluate`'s
    // own top-level statement loop.
    [[nodiscard]] bool run_procedure_body(const std::size_t body_end) {
        while (true) {
            skip_program_leading_trivia();
            if (offset_ >= body_end || at_end()) {
                return true;
            }
            if (!parse_statement()) {
                if (take_pending_jump()) {
                    continue;
                }
                return false;
            }
            if (!consume_statement_end()) {
                return false;
            }
            if (exit_sub_requested_ || exit_function_requested_) {
                exit_sub_requested_ = false;
                exit_function_requested_ = false;
                return true;
            }
        }
    }

    // Parses `(arg, arg, ...)` (already-consumed opening keyword/name), used
    // by both a module-level procedure call and a `.method(args)` call. A
    // missing `(` is treated as a parenthesis-free call with zero
    // arguments (REQ-0213) -- e.g. `Call NextId`, or the module-level/
    // sibling-method niladic-call fallback in `parse_primary_base` -- not
    // an error; the callee's own arity check rejects it with `WFC0072` if
    // it actually requires one or more arguments. A parenthesis-free call
    // *with* arguments (`Foo 5, 6`) remains unsupported, avoiding the
    // classic ambiguity between that form and other statement/expression
    // shapes; see REQ-0213's Scope.
    [[nodiscard]] std::optional<std::vector<CallArgument>> parse_call_argument_list() {
        // REQ-0243: `Name a, b` / `obj.Method a, b` (no parentheses).
        const bool bare_arguments = bare_call_arguments_;
        bare_call_arguments_ = false;
        skip_horizontal_whitespace();
        if (bare_arguments) {
            std::vector<CallArgument> arguments;
            while (true) {
                skip_horizontal_whitespace();
                std::optional<CallArgument> argument;
                if (!at_end() && current() == ',') {
                    argument = CallArgument{Value{Empty{}}, nullptr, true};  // omitted slot
                } else {
                    argument = parse_call_argument();
                }
                if (!argument.has_value()) {
                    return std::nullopt;
                }
                arguments.push_back(std::move(*argument));
                skip_horizontal_whitespace();
                if (!consume(',')) {
                    return arguments;
                }
                skip_horizontal_whitespace();
            }
        }
        if (!consume('(')) {
            return std::vector<CallArgument>{};
        }
        skip_horizontal_whitespace();
        std::vector<CallArgument> arguments;
        if (!consume(')')) {
            while (true) {
                skip_horizontal_whitespace();
                std::optional<CallArgument> argument;
                if (!at_end() && (current() == ',' || current() == ')')) {
                    argument = CallArgument{Value{Empty{}}, nullptr, true};
                } else {
                    argument = parse_call_argument();
                }
                if (!argument.has_value()) {
                    return std::nullopt;
                }
                arguments.push_back(std::move(*argument));
                skip_horizontal_whitespace();
                if (consume(')')) {
                    break;
                }
                if (!consume(',')) {
                    set_error("WFC0005", "expected closing parenthesis", offset_);
                    return std::nullopt;
                }
                skip_horizontal_whitespace();
            }
        }
        return arguments;
    }

    // Binds already-evaluated `arguments` to `definition`'s parameters into
    // a new local scope (widening/narrowing each ByVal-or-typed argument the
    // same way a `Dim`-typed assignment would, passing a Variant parameter
    // through unchanged, and accepting only an object reference for a
    // Property Set's `is_object_reference` parameter), runs the body against
    // `body_source` (the module's own source for a plain procedure, or a
    // class's own source for a method/property -- see scan_classes),
    // optionally within `instance_scope`'s field scope (see find_variable),
    // copies ByRef results back to their callers' variables, and returns the
    // call's result (the `binding_name` slot for a Function/Property Get,
    // `Empty` otherwise). Shared by call_procedure (module-level calls) and
    // parse_member_access_after_dot (method/property calls); the two differ
    // in how the callee is looked up and how its argument list is parsed
    // (parenthesized for a method call, a single already-evaluated
    // expression for a Property Let/Set), which is why they remain separate
    // callers rather than one further-generalized entry point.
    [[nodiscard]] std::optional<Value> invoke_definition(
        const ProcedureDef& definition,
        const std::string& binding_name,
        std::vector<CallArgument> arguments,
        const std::size_t identifier_offset,
        const std::string_view body_source,
        InstanceData* const instance) {
        // REQ-0206: Optional parameters make the required argument count a
        // range rather than a fixed number; a trailing ParamArray removes
        // the upper bound entirely (every argument from its position
        // onward is collected into it, including zero of them).
        if (definition.is_external) {
            if (!execute_) {
                return definition.is_function ? Value{Empty{}} : Value{Empty{}};
            }
            // A few ubiquitous Win32 timing calls are emulated natively.
            if (binding_name == "gettickcount" || binding_name == "timegettime") {
                const auto ticks = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                return Value{static_cast<Integer>(static_cast<std::uint32_t>(ticks))};
            }
            if ((binding_name == "queryperformancecounter" ||
                 binding_name == "queryperformancefrequency") && arguments.size() == 1U &&
                arguments[0].byref_target != nullptr &&
                std::holds_alternative<Currency>(*arguments[0].byref_target)) {
                // A 10 MHz counter; a Currency receives the raw 64-bit count.
                std::int64_t count = 10000000;
                if (binding_name == "queryperformancecounter") {
                    count = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                std::chrono::steady_clock::now().time_since_epoch()).count() / 100;
                }
                *arguments[0].byref_target = Value{Currency{count}};
                return Value{Integer{1}};
            }
            if (binding_name == "messagebox" && arguments.size() == 4U) {
                // No UI: answer with the default button of the requested set.
                const auto flags = whole_value(arguments[3].value).value_or(0);
                static const std::array<Integer, 6> defaults{1, 1, 3, 6, 6, 4};
                const auto set = static_cast<std::size_t>(flags & 7);
                return Value{set < defaults.size() ? defaults[set] : Integer{1}};
            }
            if (binding_name == "sleep" && arguments.size() == 1U) {
                if (const auto milliseconds = whole_value(arguments[0].value)) {
                    if (*milliseconds > 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(*milliseconds));
                    }
                    return Value{Empty{}};
                }
            }
            static_cast<void>(raise_runtime(453, "Specified DLL function not found", identifier_offset));
            return std::nullopt;
        }
        const auto& parameters = definition.parameters;
        if (std::any_of(arguments.begin(), arguments.end(),
                        [](const CallArgument& a) { return !a.name.empty(); })) {
            // Bind `name:=value` arguments to their parameter positions.
            std::vector<CallArgument> ordered;
            std::size_t positional = 0;
            while (positional < arguments.size() && arguments[positional].name.empty()) {
                ++positional;
            }
            for (std::size_t i = 0; i < positional; ++i) ordered.push_back(std::move(arguments[i]));
            for (std::size_t i = positional; i < arguments.size(); ++i) {
                if (arguments[i].name.empty()) {
                    set_error("WFC0072", "positional argument follows a named argument",
                              identifier_offset);
                    return std::nullopt;
                }
                std::size_t slot = parameters.size();
                for (std::size_t k = 0; k < parameters.size(); ++k) {
                    if (!parameters[k].is_param_array && parameters[k].name == arguments[i].name) {
                        slot = k;
                        break;
                    }
                }
                if (slot == parameters.size()) {
                    static_cast<void>(raise_runtime(
                        448, "Named argument not found", identifier_offset));
                    return std::nullopt;
                }
                while (ordered.size() <= slot) {
                    ordered.push_back(CallArgument{Value{Empty{}}, nullptr, true});
                }
                if (!ordered[slot].omitted) {
                    set_error("WFC0072", "argument specified more than once", identifier_offset);
                    return std::nullopt;
                }
                ordered[slot] = std::move(arguments[i]);
                ordered[slot].name.clear();
                ordered[slot].omitted = false;
            }
            arguments = std::move(ordered);
        }
        const bool has_param_array = !parameters.empty() && parameters.back().is_param_array;
        const std::size_t fixed_and_optional_count =
            has_param_array ? parameters.size() - 1U : parameters.size();
        std::size_t required_count = fixed_and_optional_count;
        for (std::size_t index = 0U; index < fixed_and_optional_count; ++index) {
            if (parameters[index].is_optional) {
                required_count = index;
                break;
            }
        }
        if (arguments.size() < required_count ||
            (!has_param_array && arguments.size() > fixed_and_optional_count)) {
            set_error(
                "WFC0072",
                "procedure received the wrong number of arguments",
                identifier_offset);
            return std::nullopt;
        }

        if (!execute_) {
            if (!definition.is_function) {
                return Value{Empty{}};
            }
            if (definition.return_is_object) {
                return Value{Nothing{}};
            }
            if (definition.return_is_array) {
                return Value{ArrayValue{
                    /*elements=*/{}, /*lower_bound=*/0, /*is_dynamic=*/true,
                    /*is_allocated=*/false, definition.return_type_index}};
            }
            return definition.return_is_variant ? Value{Empty{}}
                                                  : zero_value_for_index(definition.return_type_index);
        }
        // See call_procedure's own identical guard: every nested call
        // recurses through this same C++ function, so unbounded VB6
        // recursion (now including a method calling another method, or
        // itself) must still be bounded to avoid a native stack overflow.
        if (procedure_depth_ >= max_procedure_depth_ || stack_nearly_exhausted()) {
            set_error("WFC0123", "procedure call nesting is too deep", identifier_offset);
            return std::nullopt;
        }

        Scope frame;
        for (std::size_t index = 0U; index < fixed_and_optional_count; ++index) {
            const auto& parameter = parameters[index];
            // An omitted trailing Optional argument binds its own default
            // (or the type's zero value when no `= expr` was written) --
            // this synthesized argument is never a ByRef write-back target,
            // matching a literal/expression argument.
            CallArgument synthesized_argument;
            CallArgument* argument_ptr;
            if (index < arguments.size() && !arguments[index].omitted) {
                argument_ptr = &arguments[index];
            } else {
                if (!parameter.is_optional) {
                    static_cast<void>(raise_runtime(449, "Argument not optional", identifier_offset));
                    return std::nullopt;
                }
                // REQ-0224: only an omitted Optional Variant argument
                // with no explicit default is a candidate for IsMissing
                // -- recorded by name now, while it is still known which
                // parameters were actually supplied, since the frame
                // this scope belongs to is the only place IsMissing can
                // later check it from. A Variant Optional parameter that
                // *does* have an explicit default (`Optional x As
                // Variant = 5`) reports IsMissing = False even when
                // omitted in real VB6, matching REQ-0206's own
                // documented fact: the default value counts as having
                // been supplied.
                if (parameter.is_optional && parameter.is_variant && !parameter.has_default) {
                    frame.missing_parameter_names.insert(parameter.name);
                }
                synthesized_argument.value = parameter.has_default
                    ? parameter.default_value
                    : (parameter.is_variant ? Value{Empty{}}
                                             : zero_value_for_index(parameter.type_index));
                argument_ptr = &synthesized_argument;
            }
            auto& argument = *argument_ptr;
            if (parameter.is_object_reference) {
                if (!std::holds_alternative<Nothing>(argument.value) &&
                    !std::holds_alternative<ObjectInstance>(argument.value)) {
                    set_error(
                        "WFC0106", "Object parameter requires an object reference",
                        identifier_offset);
                    return std::nullopt;
                }
                // REQ-0228: a specific-class parameter (`As SomeClassName`,
                // `class_name` non-empty) requires the argument to match
                // exactly, the same check a class-typed field/return
                // already applies at its own assignment point. Threading
                // `class_name` into the callee's own `object_class_names`
                // (mirroring a class-typed field/return's identical
                // bookkeeping) means a `Set param = ...` inside the body is
                // then class-checked by the existing, unmodified Set
                // machinery too -- no separate enforcement needed there.
                if (!parameter.class_name.empty() &&
                    std::holds_alternative<ObjectInstance>(argument.value) &&
                    !class_satisfies(
                        std::get<ObjectInstance>(argument.value).data->class_name,
                        parameter.class_name)) {
                    set_error(
                        "WFC0137", "argument does not match the parameter's declared class",
                        identifier_offset);
                    return std::nullopt;
                }
                if (!parameter.class_name.empty() && is_udt_class(parameter.class_name) &&
                    (parameter.by_val || argument.byref_target == nullptr)) {
                    // REQ-0241: a UDT passed ByVal is a private copy.
                    if (const auto* udt = std::get_if<ObjectInstance>(&argument.value)) {
                        argument.value = Value{ObjectInstance{clone_udt(*udt->data)}};
                    }
                }
                frame.variables.emplace(parameter.name, std::move(argument.value));
                frame.object_variables.insert(parameter.name);
                if (!parameter.class_name.empty()) {
                    frame.object_class_names.emplace(parameter.name, parameter.class_name);
                }
                continue;
            }
            if (parameter.is_array_parameter) {
                const auto* array = std::get_if<ArrayValue>(&argument.value);
                // REQ-0215: a Variant-/Object-element array parameter
                // matches only an argument array of that same element
                // kind (never a fixed-scalar-typed one, and vice versa);
                // a fixed-type parameter matches only a fixed-type
                // argument array with the same element type.
                const bool element_kind_matches = array != nullptr &&
                    (parameter.is_variant_array_parameter
                         ? array->is_variant_element
                         : parameter.is_object_array_parameter
                               ? (array->is_object_element &&
                                  (parameter.class_name.empty() ||
                                   array->element_class_name == parameter.class_name))
                               : (!array->is_variant_element && !array->is_object_element &&
                                  array->element_type_index == parameter.type_index));
                if (!element_kind_matches) {
                    set_error("WFC0016", "argument type mismatch", identifier_offset);
                    return std::nullopt;
                }
                if (argument.byref_target == nullptr) {
                    set_error(
                        "WFC0149", "array argument must be a variable", identifier_offset);
                    return std::nullopt;
                }
                frame.variables.emplace(parameter.name, std::move(argument.value));
                continue;
            }
            if (parameter.is_variant) {
                frame.variables.emplace(parameter.name, copy_if_udt(std::move(argument.value)));
                frame.variant_variables.insert(parameter.name);
                continue;
            }
            if (argument.byref_target != nullptr && !parameter.by_val &&
                argument.value.index() != parameter.type_index) {
                // VB6: "ByRef argument type mismatch" -- a variable passed by
                // reference must already have the parameter's exact type
                // (REQ-0270); convert it first (CLng(x)) or declare ByVal.
                set_error("WFC0016", "ByRef argument type mismatch", identifier_offset);
                return std::nullopt;
            }
            if (!coerce_numeric_value(argument.value, parameter.type_index, identifier_offset)) {
                return std::nullopt;
            }
            if (argument.value.index() != parameter.type_index) {
                set_error("WFC0016", "argument type mismatch", identifier_offset);
                return std::nullopt;
            }
            frame.variables.emplace(parameter.name, std::move(argument.value));
        }
        if (has_param_array) {
            const auto& param_array_parameter = parameters.back();
            std::vector<Value> elements;
            for (std::size_t index = fixed_and_optional_count; index < arguments.size();
                 ++index) {
                Value element_value = std::move(arguments[index].value);
                if (!param_array_parameter.is_variant) {
                    if (!coerce_numeric_value(
                            element_value, param_array_parameter.type_index, identifier_offset)) {
                        return std::nullopt;
                    }
                    if (element_value.index() != param_array_parameter.type_index) {
                        set_error("WFC0016", "argument type mismatch", identifier_offset);
                        return std::nullopt;
                    }
                }
                elements.push_back(std::move(element_value));
            }
            ArrayValue param_array{
                std::move(elements), 0, /*is_dynamic=*/false, /*is_allocated=*/true,
                param_array_parameter.type_index};
            param_array.is_variant_element = param_array_parameter.is_variant;
            frame.variables.emplace(param_array_parameter.name, Value{std::move(param_array)});
        }
        if (definition.is_function) {
            frame.is_function_frame = true;
            Value initial_return_value;
            if (definition.return_is_object && !definition.return_class_name.empty() &&
                is_udt_class(definition.return_class_name)) {
                auto fresh = instantiate_class(definition.return_class_name, identifier_offset);
                if (!fresh.has_value()) {
                    return std::nullopt;
                }
                initial_return_value = std::move(*fresh);
            } else if (definition.return_is_object) {
                initial_return_value = Value{Nothing{}};
            } else if (definition.return_is_variant) {
                initial_return_value = Value{Empty{}};
            } else if (definition.return_is_array) {
                // Starts as an unallocated dynamic array (REQ-0216), the
                // same as `Dim identifier() As Type`: the body may either
                // assign a whole array to its own name (`Foo = someArray`)
                // or `ReDim`/`ReDim Preserve` it directly, both through
                // existing, unmodified array machinery.
                initial_return_value = ArrayValue{
                    /*elements=*/{}, /*lower_bound=*/0, /*is_dynamic=*/true,
                    /*is_allocated=*/false, definition.return_type_index};
            } else {
                initial_return_value = zero_value_for_index(definition.return_type_index);
            }
            frame.variables.emplace(binding_name, std::move(initial_return_value));
            if (definition.return_is_variant) {
                frame.variant_variables.insert(binding_name);
            } else if (definition.return_is_object) {
                // The return-value slot behaves exactly like an Object-
                // typed local (REQ-0205): plain `Name = expr` inside the
                // body is rejected (WFC0108, via parse_assignment's
                // existing object_variables check) -- only `Set Name =
                // expr` may assign it, reusing REQ-0200's existing Set
                // machinery (including the class-match check when
                // return_class_name is non-empty) with no new code.
                frame.object_variables.insert(binding_name);
                if (!definition.return_class_name.empty()) {
                    frame.object_class_names.emplace(binding_name, definition.return_class_name);
                }
            }
        }

        scopes_.push_back(std::move(frame));
        if (instance != nullptr) {
            instance_scopes_.push_back(instance);
        }
        const auto saved_offset = offset_;
        const auto saved_source = source_;
        const auto enclosing_execution = execute_;
        const auto* const enclosing_procedure_def = current_procedure_def_;
        current_procedure_def_ = &definition;
        ++procedure_depth_;
        offset_ = definition.body_start;
        source_ = body_source;
        execute_ = true;
        const bool enclosing_declaration_permission = allow_declarations_;
        allow_declarations_ = true;
        const bool ran_ok = run_procedure_body(definition.body_end);
        allow_declarations_ = enclosing_declaration_permission;
        --procedure_depth_;
        execute_ = enclosing_execution;
        offset_ = saved_offset;
        source_ = saved_source;
        current_procedure_def_ = enclosing_procedure_def;
        if (instance != nullptr) {
            instance_scopes_.pop_back();
        }

        // REQ-0206: copy each Static variable's final value in this call's
        // frame back into the procedure's own persistent storage before the
        // frame itself is discarded, so the next call to this same
        // procedure sees it. Done regardless of ran_ok, on the same
        // reasoning Class_Terminate's drain does not run on a failed call:
        // once an error is fatal to the whole program anyway, whether a
        // Static happened to get one more write makes no observable
        // difference, so this simply is not reached for a failed call.
        bool statics_copy_back_ok = true;
        if (ran_ok) {
            for (const auto& name : scopes_.back().static_variable_names) {
                // REQ-0230: the persistent slot being overwritten here may
                // itself be the *last* reference to an ObjectInstance (a
                // `Static` Variant can already hold one via `Set`, REQ-0200)
                // -- for example after `Set v = New C` replaced the frame's
                // own copy mid-call, leaving only this persistent slot
                // holding the original instance. A plain assignment would
                // silently drop that last reference without ever running
                // `Class_Terminate`, the same class of bug REQ-0228 fixed
                // for a ByRef parameter's own write-back.
                auto& persistent = (instance != nullptr ? instance->static_scopes[&definition]
                                                        : definition.statics).variables[name];
                if (!terminate_if_last_reference(persistent)) {
                    statics_copy_back_ok = false;
                    break;
                }
                persistent = scopes_.back().variables.at(name);
            }
        }

        if (!ran_ok || !statics_copy_back_ok) {
            scopes_.pop_back();
            return std::nullopt;
        }

        std::optional<Value> result =
            definition.is_function ? scopes_.back().variables.at(binding_name) : Value{Empty{}};
        // Bounded by fixed_and_optional_count, not arguments.size(): a
        // ParamArray's own collected elements (beyond that point) are
        // always ByVal, with no corresponding `parameters` entry per
        // argument to look up in the first place.
        bool write_back_ok = true;
        for (std::size_t index = 0U; index < std::min(arguments.size(), fixed_and_optional_count);
             ++index) {
            const auto& parameter = parameters[index];
            if (!parameter.by_val && arguments[index].byref_target != nullptr) {
                // REQ-0228: an object-typed ByRef parameter makes this
                // write-back reachable for an ObjectInstance for the first
                // time -- terminate the caller's *old* value first (the
                // same call `Set` already makes before overwriting a
                // target elsewhere), so an instance the caller was still
                // holding here does not silently skip `Class_Terminate`
                // when this call replaces it. A no-op for every other
                // value kind, the same as everywhere else this is called.
                if (!terminate_if_last_reference(*arguments[index].byref_target)) {
                    write_back_ok = false;
                    break;
                }
                *arguments[index].byref_target = scopes_.back().variables.at(parameter.name);
            }
        }
        // The return value and any ByRef write-backs above are already
        // copied out, bumping their use_count, so an instance among them
        // correctly survives this drain rather than being (wrongly) treated
        // as going out of scope here.
        const bool drained_ok = write_back_ok && drain_scope_instances(scopes_.back());
        scopes_.pop_back();
        if (!drained_ok) {
            return std::nullopt;
        }
        return result;
    }

    // Parses `(args)` for a call to the already-looked-up module-level
    // procedure `name`, then runs it via invoke_definition against the
    // module's own source and no instance scope.
    [[nodiscard]] std::optional<Value> call_procedure(
        const std::string& name,
        const std::size_t identifier_offset,
        const bool require_function) {
        const auto definition_iterator = procedures_.find(name);
        const auto& definition = definition_iterator->second;
        if (require_function && !definition.is_function) {
            set_error("WFC0122", "a Sub cannot be used in an expression", identifier_offset);
            return std::nullopt;
        }
        if (constant_expression_) {
            set_error(
                "WFC0074", "constant initializer cannot call a procedure", identifier_offset);
            return std::nullopt;
        }
        auto arguments = parse_call_argument_list();
        if (!arguments.has_value()) {
            return std::nullopt;
        }
        return invoke_definition(
            definition, name, std::move(*arguments), identifier_offset, source_, nullptr);
    }

    [[nodiscard]] std::optional<Value> parse_procedure_call(
        const std::string& name, const std::size_t identifier_offset) {
        return call_procedure(name, identifier_offset, /*require_function=*/true);
    }

    // Whether a method/property call is currently executing, and if so, its
    // instance and class (see instance_scopes_). Used to resolve an
    // unqualified call/read to a sibling member of the class currently
    // executing -- the implicit-Me equivalent VB6 itself provides for a
    // class's own members, without spelling out `Me.`.
    [[nodiscard]] InstanceData* current_instance() noexcept {
        return instance_scopes_.empty() ? nullptr : instance_scopes_.back();
    }
    [[nodiscard]] const ClassDef* current_class_def() {
        auto* const instance = current_instance();
        if (instance == nullptr) {
            return nullptr;
        }
        const auto iterator = class_definitions_.find(instance->class_name);
        return iterator == class_definitions_.end() ? nullptr : &iterator->second;
    }

    // REQ-0206: whether a `Private` field/method/property of `class_def`
    // may be accessed via `.`/`Call ...`/`Set ...` right now. Private
    // visibility in VB6 is per-*class*, not per-instance: code executing
    // inside any method of the *same* class may reach a Private member of
    // *any* instance of that class (including, but not only, `Me`), while
    // code executing at module level or inside a *different* class's
    // method may not. `is_private` members of `class_def` are otherwise
    // fully accessible (this check is a no-op for a Public member).
    // REQ-0233: `bypass_for_interface_dispatch` lets a call reached
    // through an interface-typed reference (`parse_member_access_after_
    // dot`'s own `via_interface_class`) reach a `Private`-declared
    // interface-implementation member -- real VB6 practice, since a
    // `Private Sub IShape_Draw()` is deliberately hidden from *direct*
    // access while still being the whole point of implementing the
    // interface in the first place. Every other caller leaves this
    // `false`, preserving the existing per-class visibility rule
    // unchanged.
    [[nodiscard]] bool member_accessible(
        const ClassDef& class_def, const bool is_private,
        const bool bypass_for_interface_dispatch = false) {
        return !is_private || bypass_for_interface_dispatch || current_class_def() == &class_def;
    }

    // The `Me` keyword: a fresh ObjectInstance sharing the current class
    // member's own instance (see current_instance), via
    // enable_shared_from_this so it shares that instance's existing control
    // block rather than creating a second, independent one. `offset` is the
    // `Me` token's own position, for the "only valid inside a class member"
    // diagnostic.
    [[nodiscard]] std::optional<Value> me_value(const std::size_t offset) {
        auto* const instance = current_instance();
        if (instance == nullptr) {
            set_error("WFC0138", "Me is only valid inside a class member", offset);
            return std::nullopt;
        }
        return Value{ObjectInstance{instance->shared_from_this()}};
    }

    // If `value` currently holds the *only* remaining reference to a live
    // instance (`use_count() == 1`), invokes its class's `Class_Terminate`
    // now, if declared, before `value` is itself overwritten or destroyed by
    // the caller, then cascades the same check into the instance's own
    // fields (REQ-0234): a field that was itself only reachable through this
    // now-dying instance must have its own `Class_Terminate` run too, the
    // same way a real VB6 instance's fields are released, and possibly
    // cascade further, once the instance holding them is freed. Cascading
    // happens whether or not this instance itself declares `Class_Terminate`
    // -- an instance's fields go out of scope along with it regardless.
    // Returns false only when an invoked Class_Terminate body (this
    // instance's own, or a cascaded field's) itself raised an error
    // (propagated as this statement's own failure).
    //
    // Deliberately called only from well-defined, non-reentrant-hazardous
    // points -- Set's overwrite, a Variant's plain-`=` overwrite, and (via
    // drain_scope_instances) a call frame's locals at the end of a call, the
    // module scope at the end of the program, and (recursively, here) a
    // terminating instance's own fields -- never from a C++ destructor.
    // Hooking ~InstanceData itself was considered and rejected: an
    // instance's last shared_ptr reference can be dropped from *inside*
    // another container's own teardown (a Scope's `variables` map
    // destroying its Values as part of `scopes_.pop_back()`, or the
    // Interpreter's own member destruction at the very end of the program),
    // and reentrantly calling back into this evaluator's mutable state
    // (`scopes_.push_back` for the call frame, mid-`pop_back` of that same
    // deque) from within that teardown is undefined behavior. Calling from
    // these explicit points instead means Class_Terminate always runs while
    // the interpreter is fully alive and not mid-teardown of anything; the
    // field cascade below runs from this same well-defined point, while the
    // dying instance's own shared_ptr is still alive and its `fields` Scope
    // still populated, rather than waiting for ~InstanceData to reach them.
    [[nodiscard]] bool terminate_if_last_reference(Value& value) {
        const auto* const instance_value = std::get_if<ObjectInstance>(&value);
        if (instance_value == nullptr || instance_value->data.use_count() != 1) {
            return true;
        }
        InstanceData* const instance = instance_value->data.get();
        const auto class_iterator = class_definitions_.find(instance->class_name);
        if (class_iterator != class_definitions_.end()) {
            const auto terminate_iterator = class_iterator->second.methods.find("class_terminate");
            if (terminate_iterator != class_iterator->second.methods.end()) {
                if (!invoke_definition(
                        terminate_iterator->second, "class_terminate", {}, 0,
                        class_iterator->second.source, instance)
                        .has_value()) {
                    return false;
                }
            }
        }
        if (instance->store) {
            for (auto& stored : instance->store->values) {
                if (!terminate_if_last_reference(stored)) {
                    return false;
                }
            }
            instance->store->values.clear();
        }
        return drain_scope_instances(instance->fields);
    }

    // Drains every ObjectInstance-holding variable in `scope` (a call
    // frame's locals at the end of a call, or the module scope at the end
    // of the program), terminating each one that has become the last
    // reference (see terminate_if_last_reference) and then clearing it to
    // Empty. Clearing as it goes, one variable at a time, rather than
    // checking every variable first and clearing afterward, matters for two
    // reasons: it gives two same-frame variables that alias the same
    // instance an accurate use_count when each is checked in turn (whichever
    // is drained second correctly sees the first's reference already
    // gone), and it means the scope's own destructor (whether that runs via
    // an explicit `scopes_.pop_back()` right after this returns, or the
    // Interpreter's own final teardown for the module scope) never touches
    // a live ObjectInstance, avoiding the reentrancy hazard
    // terminate_if_last_reference's own comment describes. A caller-visible
    // return value (the function's result, or a ByRef argument's write-back
    // target) must already have been copied out before calling this, since
    // a copy bumps use_count and correctly prevents that instance from
    // being treated as terminable here.
    [[nodiscard]] bool drain_scope_instances(Scope& scope) {
        for (auto& [name, value] : scope.variables) {
            if (!terminate_if_last_reference(value)) {
                return false;
            }
            if (std::holds_alternative<ObjectInstance>(value)) {
                value = Value{Empty{}};
            }
        }
        return true;
    }

    // Parses `(args)` for a call to `class_def`'s already-looked-up
    // `member_name` method on `instance`, requiring it to be a Function
    // when `require_function` (an expression-context call; false for a
    // `Call`-statement Sub invocation). Shared by parse_member_access_after_dot
    // (an explicit `obj.Method(args)`) and parse_primary_base/
    // parse_call_statement (an unqualified sibling call resolved via
    // current_instance/current_class_def).
    [[nodiscard]] std::optional<Value> call_class_method(
        InstanceData& instance,
        const ClassDef& class_def,
        const std::string& member_name,
        const std::size_t member_offset,
        const bool require_function,
        const bool bypass_for_interface_dispatch = false) {
        const auto method_iterator = class_def.methods.find(member_name);
        if (method_iterator == class_def.methods.end()) {
            set_error("WFC0135", "unknown member", member_offset);
            return std::nullopt;
        }
        if (!member_accessible(
                class_def, method_iterator->second.is_private, bypass_for_interface_dispatch)) {
            set_error("WFC0142", "member is not accessible outside its class", member_offset);
            return std::nullopt;
        }
        if (require_function && !method_iterator->second.is_function) {
            set_error("WFC0122", "a Sub cannot be used in an expression", member_offset);
            return std::nullopt;
        }
        if (constant_expression_) {
            set_error(
                "WFC0074", "constant initializer cannot call a procedure", member_offset);
            return std::nullopt;
        }
        auto arguments = parse_call_argument_list();
        if (!arguments.has_value()) {
            return std::nullopt;
        }
        return invoke_definition(
            method_iterator->second, member_name, std::move(*arguments), member_offset,
            class_def.source, &instance);
    }

    // Parses `.member` or `.member(args)` immediately after `base`'s own
    // '.' has already been consumed (see parse_primary's postfix loop).
    // `base` must currently be an object reference (Nothing or a live
    // instance); `base_offset` is used for the "requires an object
    // reference"/"Invalid use of Nothing" diagnostics. Dispatches to a
    // method call (a `(` follows the member name), a Property Get, or a
    // plain field read, in that order -- a class cannot declare a field and
    // a Property accessor under the same name (see scan_class_body), so
    // this order is unambiguous.
    [[nodiscard]] std::optional<Value> parse_member_access_after_dot(
        const Value base, const std::size_t base_offset, const bool require_function = true,
        const std::string& via_interface_class = {}) {
        if (!std::holds_alternative<Nothing>(base) &&
            !std::holds_alternative<ObjectInstance>(base)) {
            if (!execute_) {
                // A placeholder from a not-taken branch: parse through it.
                return parse_member_access_after_dot(
                    Value{Nothing{}}, base_offset, require_function, via_interface_class);
            }
            set_error("WFC0136", "member access requires an object reference", base_offset);
            return std::nullopt;
        }
        skip_horizontal_whitespace();
        const auto member_offset = offset_;
        char type_character{};
        auto member_name = parse_identifier(&type_character);
        if (!member_name.has_value() || type_character != '\0') {
            set_error("WFC0011", "expected member name after '.'", member_offset);
            return std::nullopt;
        }
        if (std::holds_alternative<Nothing>(base)) {
            if (!execute_) {
                // Dry-run parsing of a not-taken branch (REQ-0229): `base`
                // really is `Nothing` here, so unlike every dispatch
                // branch below (which always has a live instance's own
                // class to resolve against, dry-run or not), there is
                // nothing at all to resolve `.member` against. Still
                // parses through an optional `(args)` list, so the source
                // text and each argument's own shape are validated the
                // same as they would be for a live instance, but returns
                // a placeholder `Long` rather than erroring -- matching
                // this evaluator's established convention of not raising
                // a value-dependent runtime error for code that will not
                // actually execute, the same as an arithmetic overflow or
                // division by zero in a dead branch.
                skip_horizontal_whitespace();
                if (!at_end() && current() == '(') {
                    if (!parse_call_argument_list().has_value()) {
                        return std::nullopt;
                    }
                }
                return Value{Integer{}};
            }
            set_error("WFC0106", "Invalid use of Nothing", base_offset);
            return std::nullopt;
        }
        InstanceData& instance = *std::get<ObjectInstance>(base).data;
        const auto class_iterator = class_definitions_.find(instance.class_name);
        const ClassDef& class_def = class_iterator->second;

        // REQ-0233: dispatch through an interface-typed reference resolves
        // to the implementing class's own `InterfaceName_MemberName`, not
        // the bare member name (real VB6's mandatory interface-member
        // naming convention) -- but only when the live instance's own
        // class actually `Implements` that interface; a generic `Object`
        // reference, or one declared as a concrete class the instance
        // simply is, leaves `member_name` untouched and resolves normally.
        // REQ-0233: a `Private` interface-implementation method/property
        // (real VB6 practice -- `Private Sub IShape_Draw()` is the norm,
        // since it should only ever be reachable through the interface
        // reference, not directly) is accessible here despite being
        // `Private`, since reaching it through the interface reference is
        // exactly the sanctioned way to call it.
        bool dispatched_via_interface = false;
        if (!via_interface_class.empty()) {
            for (const auto& implemented : class_def.implements) {
                if (implemented == via_interface_class) {
                    member_name = via_interface_class + "_" + *member_name;
                    dispatched_via_interface = true;
                    break;
                }
            }
        }

        skip_horizontal_whitespace();
        if (!at_end() && current() == '(') {
            if (class_def.methods.contains(*member_name)) {
                return call_class_method(
                    instance, class_def, *member_name, member_offset, require_function,
                    dispatched_via_interface);
            }
            // An indexed Property Get (REQ-0205): `obj.Name(args)` reaches
            // the same parenthesized-call shape a method call would, since
            // a class cannot declare both a method and a property under the
            // same name (see scan_class_body's WFC0128 check).
            const auto indexed_getter_iterator = class_def.property_get.find(*member_name);
            if (indexed_getter_iterator != class_def.property_get.end()) {
                if (!member_accessible(
                        class_def, indexed_getter_iterator->second.is_private,
                        dispatched_via_interface)) {
                    set_error(
                        "WFC0142", "member is not accessible outside its class", member_offset);
                    return std::nullopt;
                }
                if (constant_expression_) {
                    set_error(
                        "WFC0074", "constant initializer cannot call a procedure",
                        member_offset);
                    return std::nullopt;
                }
                if (indexed_getter_iterator->second.parameters.empty()) {
                    // `obj.Prop(i)` where Prop takes no index: apply the
                    // parentheses to the object (or array) it returns.
                    auto held = invoke_definition(
                        indexed_getter_iterator->second, *member_name, {}, member_offset,
                        class_def.source, &instance);
                    if (!held.has_value()) {
                        return std::nullopt;
                    }
                    if (const auto* holder = std::get_if<ObjectInstance>(&*held)) {
                        const auto held_class = class_definitions_.find(holder->data->class_name);
                        if (held_class != class_definitions_.end() &&
                            !held_class->second.default_member.empty()) {
                            const auto& default_member = held_class->second.default_member;
                            if (held_class->second.methods.contains(default_member)) {
                                return call_class_method(
                                    *holder->data, held_class->second, default_member, member_offset,
                                    /*require_function=*/true);
                            }
                            const auto default_getter =
                                held_class->second.property_get.find(default_member);
                            if (default_getter != held_class->second.property_get.end()) {
                                auto default_arguments = parse_call_argument_list();
                                if (!default_arguments.has_value()) {
                                    return std::nullopt;
                                }
                                return invoke_definition(
                                    default_getter->second, default_member,
                                    std::move(*default_arguments), member_offset,
                                    held_class->second.source, holder->data.get());
                            }
                        }
                    } else if (std::holds_alternative<ArrayValue>(*held)) {
                        return parse_array_index(*held);
                    }
                    set_error("WFC0135", "unknown member", member_offset);
                    return std::nullopt;
                }
                auto arguments = parse_call_argument_list();
                if (!arguments.has_value()) {
                    return std::nullopt;
                }
                return invoke_definition(
                    indexed_getter_iterator->second, *member_name, std::move(*arguments),
                    member_offset, class_def.source, &instance);
            }
            {
                // REQ-0251: `obj.arrayField(i)`.
                const auto array_field = instance.fields.variables.find(*member_name);
                if (array_field != instance.fields.variables.end() &&
                    std::holds_alternative<ArrayValue>(array_field->second)) {
                    const auto field_def_iterator = class_def.fields.find(*member_name);
                    if (field_def_iterator != class_def.fields.end() &&
                        !member_accessible(class_def, field_def_iterator->second.is_private)) {
                        set_error(
                            "WFC0142", "member is not accessible outside its class", member_offset);
                        return std::nullopt;
                    }
                    return parse_array_index(array_field->second);
                }
            }
            set_error("WFC0135", "unknown member", member_offset);
            return std::nullopt;
        }
        const auto getter_iterator = class_def.property_get.find(*member_name);
        if (getter_iterator != class_def.property_get.end()) {
            if (!member_accessible(
                    class_def, getter_iterator->second.is_private, dispatched_via_interface)) {
                set_error("WFC0142", "member is not accessible outside its class", member_offset);
                return std::nullopt;
            }
            return invoke_definition(
                getter_iterator->second, *member_name, {}, member_offset, class_def.source,
                &instance);
        }
        // `obj.Method` / `Call obj.Method` with no parentheses (REQ-0217):
        // a zero-argument dotted method call, checked after Property Get
        // (a class cannot declare both a method and a property under the
        // same name) and before a field, matching the with-parens branch
        // above's own method-then-property-then-unknown order.
        // call_class_method's own parse_call_argument_list tolerates the
        // missing '(' as zero arguments (REQ-0213), rejecting it with
        // WFC0072 if the method actually requires one or more.
        const auto method_iterator = class_def.methods.find(*member_name);
        if (method_iterator != class_def.methods.end()) {
            return call_class_method(
                instance, class_def, *member_name, member_offset, require_function,
                dispatched_via_interface);
        }
        const auto field_iterator = instance.fields.variables.find(*member_name);
        if (field_iterator != instance.fields.variables.end()) {
            const auto field_def_iterator = class_def.fields.find(*member_name);
            if (field_def_iterator != class_def.fields.end() &&
                !member_accessible(class_def, field_def_iterator->second.is_private)) {
                set_error("WFC0142", "member is not accessible outside its class", member_offset);
                return std::nullopt;
            }
            if (std::holds_alternative<ArrayValue>(field_iterator->second)) {
                const auto after_name = offset_;
                skip_horizontal_whitespace();
                if (!at_end() && current() == '(') {
                    return parse_array_index(field_iterator->second);
                }
                offset_ = after_name;
            }
            return field_iterator->second;
        }
        set_error("WFC0135", "unknown member", member_offset);
        return std::nullopt;
    }

    // `Call name(args)` -- the only supported way to invoke a Sub as a
    // statement, or a Function while discarding its result, matching real
    // VB6's `Call` statement (this evaluator does not support VB6's other,
    // parenthesis-free `name arg1, arg2` statement-call form; see
    // REQ-0202's Scope).
    [[nodiscard]] bool parse_call_statement() {
        skip_horizontal_whitespace();
        const auto identifier_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        if (!identifier.has_value() || type_character != '\0') {
            set_error("WFC0011", "expected procedure name after Call", identifier_offset);
            return false;
        }
        // `Call Me.Method(args)`.
        if (*identifier == "me") {
            skip_horizontal_whitespace();
            if (!at_end() && current() == '.') {
                auto base = me_value(identifier_offset);
                if (!base.has_value()) {
                    return false;
                }
                advance();
                const auto result = parse_member_access_after_dot(
                    *base, identifier_offset, /*require_function=*/false);
                return result.has_value();
            }
            set_error("WFC0015", "undeclared procedure", identifier_offset);
            return false;
        }
        // `Call obj.Method(args)` -- a method call on an object reference,
        // dispatched the same way an expression's `obj.Method(args)` would
        // be, but allowing a Sub (its result is simply discarded, like any
        // other Call target).
        const auto after_identifier_offset = offset_;
        skip_horizontal_whitespace();
        if (!at_end() && current() == '.') {
            const auto variable = find_variable(*identifier);
            if (variable.value != nullptr &&
                (std::holds_alternative<Nothing>(*variable.value) ||
                 std::holds_alternative<ObjectInstance>(*variable.value))) {
                const auto base = *variable.value;
                // REQ-0233: `Call obj.Method(args)` dispatches through
                // `obj`'s own declared class the same way an expression's
                // `obj.Method(args)` does (`parse_primary`'s own
                // lookahead) -- `obj` is already a resolved variable here,
                // so its declared class (if any) is read directly instead
                // of needing a speculative lookahead.
                std::string declared_interface_class;
                const auto declared_class = variable.scope->object_class_names.find(*identifier);
                if (declared_class != variable.scope->object_class_names.end()) {
                    declared_interface_class = declared_class->second;
                }
                advance();
                const auto result = parse_member_access_after_dot(
                    base, identifier_offset, /*require_function=*/false,
                    declared_interface_class);
                return result.has_value();
            }
        }
        offset_ = after_identifier_offset;
        if (!procedures_.contains(*identifier)) {
            // An unqualified `Call Method(args)` for a sibling method of
            // the class currently executing (see current_instance/
            // current_class_def) -- the Call-statement counterpart of the
            // same implicit-Me convenience parse_primary_base gives
            // expressions.
            if (auto* const instance = current_instance()) {
                if (const auto* const class_def = current_class_def()) {
                    if (class_def->methods.contains(*identifier)) {
                        const auto result = call_class_method(
                            *instance, *class_def, *identifier, identifier_offset,
                            /*require_function=*/false);
                        return result.has_value();
                    }
                }
            }
            set_error("WFC0015", "undeclared procedure", identifier_offset);
            return false;
        }
        const auto result = call_procedure(*identifier, identifier_offset, false);
        return result.has_value();
    }

    // Skips over a module-level `Sub`/`Function` declaration during the
    // main top-to-bottom pass: its body only runs when called, not where
    // it's textually written. `scan_procedures` already registered it
    // (including its `declaration_end`) before this pass began.
    // `Property Get|Let|Set Name(...) ... End Property` in a standard module:
    // `Property` has been consumed; skips the whole declaration.
    [[nodiscard]] bool parse_property_declaration_skip(const std::size_t statement_offset) {
        skip_horizontal_whitespace();
        std::string prefix;
        if (consume_keyword("get")) {
        } else if (consume_keyword("let")) {
            prefix = "wfclet_";
        } else if (consume_keyword("set")) {
            prefix = "wfcset_";
        } else {
            set_error("WFC0010", "expected Get, Let or Set after Property", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        const auto name_offset = offset_;
        char type_character{};
        auto name = parse_identifier(&type_character);
        const auto definition =
            name.has_value() ? procedures_.find(prefix + *name) : procedures_.end();
        if (definition == procedures_.end()) {
            set_error("WFC0118", "expected procedure name", name_offset);
            return false;
        }
        static_cast<void>(statement_offset);
        offset_ = definition->second.declaration_end;
        return true;
    }

    [[nodiscard]] bool parse_procedure_declaration_skip(const std::size_t statement_offset) {
        if (!allow_declarations_) {
            set_error(
                "WFC0027",
                "declarations are not supported in conditional blocks",
                statement_offset);
            return false;
        }
        skip_horizontal_whitespace();
        const auto name_offset = offset_;
        char type_character{};
        auto name = parse_identifier(&type_character);
        if (!name.has_value()) {
            set_error("WFC0118", "expected procedure name", name_offset);
            return false;
        }
        const auto definition = procedures_.find(*name);
        if (definition == procedures_.end()) {
            set_error("WFC0118", "expected procedure name", name_offset);
            return false;
        }
        offset_ = definition->second.declaration_end;
        return true;
    }

    [[nodiscard]] std::optional<Value> parse_array_index(const Value& array_variable) {
        const auto& array = std::get<ArrayValue>(array_variable);
        advance();  // consume '('
        const auto dimension_count = array_expected_dimension_count(array);
        auto indices = parse_index_list(dimension_count);
        if (!indices.has_value()) {
            return std::nullopt;
        }
        if (!execute_) {
            // array.elements can legitimately be empty here -- a
            // ParamArray (REQ-0206) called with zero extra arguments, or an
            // unallocated dynamic array before its first ReDim (REQ-0207)
            // -- so a placeholder of the array's own declared element type
            // stands in via array_element_default (ArrayValue.
            // element_type_index) rather than reading a nonexistent first
            // element; this is only ever reached while type-checking an
            // unreached branch, where the actual value is discarded.
            return array_element_default(array.element_type_index);
        }
        const auto flat_offset = array_flat_offset(array, *indices);
        if (!flat_offset.has_value()) {
            return std::nullopt;
        }
        return array.elements[*flat_offset];
    }

    [[nodiscard]] static bool is_misc_function_name(const std::string_view name) {
        static const std::unordered_set<std::string> names{
            "pmt", "fv", "pv", "nper", "ipmt", "ppmt", "npv", "irr", "sln", "syd", "ddb",
            "formatnumber", "formatcurrency", "formatpercent", "partition", "doevents", "command",
            "command$", "cverr", "cvdate", "rate", "mirr", "msgbox", "inputbox", "createobject",
            "getobject", "getsetting", "fileattr", "filedatetime", "getattr", "callbyname", "shell",
            "getallsettings", "objptr", "strptr", "wfcregexmatches", "wfcregexreplace", "wfcstore"};
        return names.contains(std::string(name));
    }

    // REQ-0246: financial functions, FormatNumber/Currency/Percent, Partition.
    [[nodiscard]] std::optional<Value> evaluate_misc_function(
        const std::string_view name, std::vector<Value>& arguments, const std::size_t offset) {
        const auto count = arguments.size();
        const auto arity = [&](const std::size_t low, const std::size_t high) {
            if (count < low || count > high) {
                set_error("WFC0072", "function received the wrong number of arguments", offset);
                return false;
            }
            return true;
        };
        const auto number_at = [&](const std::size_t index, const double fallback) -> std::optional<double> {
            if (index >= count) return fallback;
            const auto& v = arguments[index];
            if (std::holds_alternative<Empty>(v)) return 0.0;
            if (is_number(v) && !std::holds_alternative<Decimal>(v)) return as_double(v);
            if (const auto* flag = std::get_if<bool>(&v)) return *flag ? -1.0 : 0.0;
            return std::nullopt;
        };
        const auto bad_type = [&]() {
            set_error("WFC0073", "Type mismatch", offset);
            return std::nullopt;
        };
        const auto invalid_call = [&]() {
            static_cast<void>(raise_runtime(5, "Invalid procedure call or argument", offset));
            return std::nullopt;
        };
        const auto finite_result = [&](const double v) -> std::optional<Value> {
            if (!std::isfinite(v)) {
                static_cast<void>(raise_runtime(6, "Overflow", offset));
                return std::nullopt;
            }
            return Value{v};
        };
        if (name == "doevents") {
            if (!arity(0, 0)) return std::nullopt;
            return Value{Integer{}};
        }
        if (name == "callbyname") {
            // CallByName(object, name, callType, args...) -- REQ-0263.
            if (count < 3U) {
                set_error("WFC0072", "function received the wrong number of arguments", offset);
                return std::nullopt;
            }
            const auto* holder = std::get_if<ObjectInstance>(&arguments[0]);
            const auto* member_text = std::get_if<std::string>(&arguments[1]);
            const auto call_type = number_at(2, 0.0);
            if (member_text == nullptr || !call_type) return bad_type();
            if (holder == nullptr) {
                if (!execute_) return Value{Empty{}};
                static_cast<void>(raise_runtime(91, "Object variable or With block variable not set", offset));
                return std::nullopt;
            }
            if (!execute_) return Value{Empty{}};
            std::string member;
            for (const char c : *member_text) member.push_back(ascii_lower(c));
            InstanceData& instance = *holder->data;
            const ClassDef& class_def = class_definitions_.at(instance.class_name);
            std::vector<CallArgument> call_arguments;
            for (std::size_t i = 3; i < count; ++i) {
                call_arguments.push_back(CallArgument{arguments[i], nullptr});
            }
            const int kind = static_cast<int>(*call_type);
            if (kind == 1) {  // vbMethod
                const auto method = class_def.methods.find(member);
                if (method == class_def.methods.end()) {
                    static_cast<void>(raise_runtime(438, "Object doesn't support this property or method", offset));
                    return std::nullopt;
                }
                return invoke_definition(
                    method->second, member, std::move(call_arguments), offset, class_def.source, &instance);
            }
            if (kind == 2) {  // vbGet
                const auto getter = class_def.property_get.find(member);
                if (getter != class_def.property_get.end()) {
                    return invoke_definition(
                        getter->second, member, std::move(call_arguments), offset, class_def.source, &instance);
                }
                const auto field = instance.fields.variables.find(member);
                if (field != instance.fields.variables.end()) return field->second;
                const auto method = class_def.methods.find(member);
                if (method != class_def.methods.end()) {
                    return invoke_definition(
                        method->second, member, std::move(call_arguments), offset, class_def.source, &instance);
                }
                static_cast<void>(raise_runtime(438, "Object doesn't support this property or method", offset));
                return std::nullopt;
            }
            if ((kind == 4 || kind == 8) && !call_arguments.empty()) {
                const auto& table = kind == 4 ? class_def.property_let : class_def.property_set;
                const auto setter = table.find(member);
                if (setter != table.end()) {
                    return invoke_definition(
                        setter->second, member, std::move(call_arguments), offset, class_def.source, &instance);
                }
                const auto field = instance.fields.variables.find(member);
                if (field != instance.fields.variables.end() && call_arguments.size() == 1U) {
                    if (kind == 8) {
                        const auto declared = instance.fields.object_class_names.find(member);
                        if (!assign_object_reference(
                                field->second,
                                declared != instance.fields.object_class_names.end() ? declared->second
                                                                                     : std::string{},
                                call_arguments[0].value, offset)) {
                            return std::nullopt;
                        }
                    } else {
                        Value value = call_arguments[0].value;
                        if (!instance.fields.variant_variables.contains(member) &&
                            !coerce_numeric_value(value, field->second.index(), offset)) {
                            return std::nullopt;
                        }
                        field->second = std::move(value);
                    }
                    return Value{Empty{}};
                }
            }
            static_cast<void>(raise_runtime(438, "Object doesn't support this property or method", offset));
            return std::nullopt;
        }
        if (name == "msgbox") {
            if (!arity(1, 5)) return std::nullopt;
            // There is no UI: answer with the dialog's default button.
            std::int64_t style = 0;
            if (count >= 2U && is_number(arguments[1]) && !std::holds_alternative<Decimal>(arguments[1])) {
                style = static_cast<std::int64_t>(as_double(arguments[1]));
            }
            // Result value of the Nth button (0-based) for each button set.
            static const std::array<std::array<Integer, 3>, 6> buttons{{
                {1, 0, 0},   // vbOKOnly
                {1, 2, 0},   // vbOKCancel: OK, Cancel
                {3, 4, 5},   // vbAbortRetryIgnore
                {6, 7, 2},   // vbYesNoCancel
                {6, 7, 0},   // vbYesNo
                {4, 2, 0},   // vbRetryCancel
            }};
            static const std::array<std::size_t, 6> button_counts{1, 2, 3, 3, 2, 2};
            const auto set = static_cast<std::size_t>(style & 7);
            if (set >= buttons.size()) return Value{Integer{1}};
            auto index = static_cast<std::size_t>((style >> 8) & 3);
            if (index >= button_counts[set]) index = 0;
            return Value{buttons[set][index]};
        }
        if (name == "inputbox") {
            if (!arity(1, 7)) return std::nullopt;
            const auto* fallback = count >= 3U ? std::get_if<std::string>(&arguments[2]) : nullptr;
            return Value{fallback != nullptr ? *fallback : std::string{}};
        }
        if (name == "createobject" || name == "getobject") {
            if (!arity(0, 2)) return std::nullopt;
            if (!execute_) return Value{Nothing{}};
            if (name == "createobject") {
                if (const auto* progid = std::get_if<std::string>(&arguments[0])) {
                    std::string lowered;
                    for (const char c : *progid) lowered.push_back(ascii_lower(c));
                    if (lowered == "scripting.dictionary" &&
                        class_definitions_.contains("wfcdictionary")) {
                        return instantiate_class("wfcdictionary", offset);
                    }
                    if ((lowered == "vbscript.regexp" || lowered == "vbscript.regexp.55") &&
                        class_definitions_.contains("wfcregexp")) {
                        return instantiate_class("wfcregexp", offset);
                    }
                    if (lowered == "scripting.filesystemobject" &&
                        class_definitions_.contains("wfcfilesystemobject")) {
                        return instantiate_class("wfcfilesystemobject", offset);
                    }
                }
            }
            static_cast<void>(raise_runtime(429, "ActiveX component can't create object", offset));
            return std::nullopt;
        }
        if (name == "wfcregexmatches" || name == "wfcregexreplace") {
            const bool replacing = name == "wfcregexreplace";
            if (!arity(replacing ? 6 : 5, replacing ? 6 : 5)) return std::nullopt;
            const auto* pattern = std::get_if<std::string>(&arguments[0]);
            const auto* text = std::get_if<std::string>(&arguments[1]);
            if (pattern == nullptr || text == nullptr) return bad_type();
            const std::size_t flag_base = replacing ? 3U : 2U;
            const auto* replacement = replacing ? std::get_if<std::string>(&arguments[2]) : nullptr;
            if (replacing && replacement == nullptr) return bad_type();
            const auto flag = [&](const std::size_t index) {
                const auto* value = std::get_if<bool>(&arguments[index]);
                return value != nullptr && *value;
            };
            const bool ignore_case = flag(flag_base);
            const bool multi_line = flag(flag_base + 1U);
            const bool global = flag(flag_base + 2U);
            if (!execute_) return replacing ? Value{std::string{}} : Value{Empty{}};
            try {
                auto options = std::regex::ECMAScript;
                if (ignore_case) options |= std::regex::icase;
                if (multi_line) {
                    // Older MSVC STLs lack the C++17 multiline flag; there ^/$ match only at the text ends.
                    []<typename Regex>(auto& flags) {
                        if constexpr (requires { Regex::multiline; }) flags |= Regex::multiline;
                    }.template operator()<std::regex>(options);
                }
                const std::regex expression(*pattern, options);
                if (replacing) {
                    return Value{std::regex_replace(
                        *text, expression, *replacement,
                        global ? std::regex_constants::format_default
                               : std::regex_constants::format_first_only)};
                }
                std::vector<Value> found;
                for (auto it = std::sregex_iterator(text->begin(), text->end(), expression);
                     it != std::sregex_iterator(); ++it) {
                    const std::smatch& match = *it;
                    std::vector<Value> row;
                    row.emplace_back(static_cast<Integer>(match.position(0)));
                    row.emplace_back(static_cast<Integer>(match.length(0)));
                    for (std::size_t group = 0; group < match.size(); ++group) {
                        row.emplace_back(match[group].str());
                    }
                    ArrayValue row_array{};
                    row_array.elements = std::move(row);
                    row_array.is_variant_element = true;
                    row_array.element_type_index = Value{Empty{}}.index();
                    found.emplace_back(std::move(row_array));
                    if (!global) break;
                }
                ArrayValue result{};
                result.elements = std::move(found);
                result.is_variant_element = true;
                result.element_type_index = Value{Empty{}}.index();
                return Value{std::move(result)};
            } catch (const std::regex_error&) {
                static_cast<void>(raise_runtime(5017, "Syntax error in regular expression", offset));
                return std::nullopt;
            }
        }
        if (name == "wfcstore") {
            if (arguments.size() < 2U) return bad_type();
            const auto* op = std::get_if<Integer>(&arguments[0]);
            const auto* owner = std::get_if<ObjectInstance>(&arguments[1]);
            if (op == nullptr || owner == nullptr) return bad_type();
            if (!execute_) return Value{Integer{0}};
            auto& slot = owner->data->store;
            if (!slot) slot = std::make_unique<NativeStore>();
            NativeStore& store = *slot;
            const auto canonical = [&](const Value& key) {
                char buffer[40];
                if (const auto* text = std::get_if<std::string>(&key)) {
                    std::string result = "s";
                    result += store.text_compare ? fold_case(*text) : *text;
                    return result;
                }
                if (is_number(key) || std::holds_alternative<bool>(key)) {
                    std::snprintf(buffer, sizeof(buffer), "n%.17g",
                                  std::holds_alternative<bool>(key) ? (std::get<bool>(key) ? -1.0 : 0.0)
                                                                    : as_double(key));
                    return std::string(buffer);
                }
                if (const auto* date = std::get_if<DateValue>(&key)) {
                    std::snprintf(buffer, sizeof(buffer), "d%.17g", date->serial);
                    return std::string(buffer);
                }
                if (const auto* object = std::get_if<ObjectInstance>(&key)) {
                    std::snprintf(buffer, sizeof(buffer), "o%p", static_cast<const void*>(object->data.get()));
                    return std::string(buffer);
                }
                return std::string("e");
            };
            const auto reindex = [&]() {
                store.index.clear();
                store.index.reserve(store.keys.size());
                for (std::size_t i = 0; i < store.keys.size(); ++i) {
                    if (!std::holds_alternative<Empty>(store.keys[i])) {
                        store.index.emplace(canonical(store.keys[i]), i);
                    }
                }
                store.dirty = false;
            };
            const auto position_at = [&](const std::size_t argument) -> std::optional<std::size_t> {
                if (argument >= arguments.size()) return std::nullopt;
                const auto* position = std::get_if<Integer>(&arguments[argument]);
                if (position == nullptr || *position < 1 ||
                    static_cast<std::size_t>(*position) > store.keys.size()) {
                    return std::nullopt;
                }
                return static_cast<std::size_t>(*position) - 1U;
            };
            const auto make_array = [&](std::vector<Value> elements) {
                ArrayValue result{};
                result.elements = std::move(elements);
                result.is_variant_element = true;
                result.element_type_index = Value{Empty{}}.index();
                return Value{std::move(result)};
            };
            switch (*op) {
            case 1: return Value{static_cast<Integer>(store.keys.size())};
            case 2: {
                if (arguments.size() < 3U) return bad_type();
                if (store.dirty) reindex();
                const auto found = store.index.find(canonical(arguments[2]));
                return Value{found == store.index.end() ? Integer{0}
                                                        : static_cast<Integer>(found->second + 1U)};
            }
            case 3: {
                if (arguments.size() < 5U) return bad_type();
                const auto* at = std::get_if<Integer>(&arguments[4]);
                if (at == nullptr) return bad_type();
                const bool append = *at < 1 || static_cast<std::size_t>(*at) > store.keys.size();
                const bool keyed = !std::holds_alternative<Empty>(arguments[2]);
                if (append) {
                    if (!store.dirty && keyed) {
                        store.index.emplace(canonical(arguments[2]), store.keys.size());
                    }
                    store.keys.push_back(arguments[2]);
                    store.values.push_back(arguments[3]);
                } else {
                    const auto where = static_cast<std::ptrdiff_t>(*at - 1);
                    store.keys.insert(store.keys.begin() + where, arguments[2]);
                    store.values.insert(store.values.begin() + where, arguments[3]);
                    store.dirty = true;
                }
                return Value{Integer{0}};
            }
            case 4:
            case 5:
            case 11: {
                const auto at = position_at(2);
                if (!at.has_value()) return *op == 11 ? Value{false} : Value{Empty{}};
                if (*op == 11) return Value{is_object_reference(store.values[*at])};
                return *op == 4 ? store.values[*at] : store.keys[*at];
            }
            case 6: {
                const auto at = position_at(2);
                if (!at.has_value() || arguments.size() < 4U) return bad_type();
                Value old = std::move(store.values[*at]);
                store.values[*at] = arguments[3];
                if (!terminate_if_last_reference(old)) return std::nullopt;
                return Value{Integer{0}};
            }
            case 7: {
                const auto at = position_at(2);
                if (!at.has_value() || arguments.size() < 4U) return bad_type();
                store.keys[*at] = arguments[3];
                store.dirty = true;
                return Value{Integer{0}};
            }
            case 8: {
                const auto at = position_at(2);
                if (!at.has_value()) return bad_type();
                Value old = std::move(store.values[*at]);
                store.keys.erase(store.keys.begin() + static_cast<std::ptrdiff_t>(*at));
                store.values.erase(store.values.begin() + static_cast<std::ptrdiff_t>(*at));
                store.dirty = true;
                if (!terminate_if_last_reference(old)) return std::nullopt;
                return Value{Integer{0}};
            }
            case 9: {
                auto old = std::move(store.values);
                store.values.clear();
                store.keys.clear();
                store.index.clear();
                store.dirty = false;
                for (auto& value : old) {
                    if (!terminate_if_last_reference(value)) return std::nullopt;
                }
                return Value{Integer{0}};
            }
            case 10: {
                if (arguments.size() < 3U) return bad_type();
                const auto* mode = std::get_if<Integer>(&arguments[2]);
                if (mode == nullptr) return bad_type();
                store.text_compare = *mode != 0;
                store.dirty = true;
                return Value{Integer{0}};
            }
            case 12: return make_array(std::vector<Value>(store.values));
            case 13: return make_array(std::vector<Value>(store.keys));
            default: return bad_type();
            }
        }
        if (name == "objptr" || name == "strptr") {
            // Opaque, stable-per-object "addresses" for code that passes them along.
            if (!arity(1, 1)) return std::nullopt;
            if (!execute_) return Value{Integer{}};
            std::uintptr_t address = 0;
            if (const auto* instance = std::get_if<ObjectInstance>(&arguments[0])) {
                address = reinterpret_cast<std::uintptr_t>(instance->data.get());
            } else if (const auto* text = std::get_if<std::string>(&arguments[0])) {
                address = text->empty() ? 0U : reinterpret_cast<std::uintptr_t>(text->data());
            } else if (name == "objptr") {
                return bad_type();
            }
            return Value{static_cast<Integer>(address & 0x7FFFFFFFU)};
        }
        if (name == "shell") {
            if (!arity(1, 2)) return std::nullopt;
            const auto* command = std::get_if<std::string>(&arguments[0]);
            if (command == nullptr) return bad_type();
            if (!execute_) return Value{0.0};
            if (std::system(command->c_str()) != 0) {
                static_cast<void>(raise_runtime(53, "File not found", offset));
                return std::nullopt;
            }
            return Value{1.0};  // a task id; the command has already run to completion
        }
        if (name == "getallsettings") {
            if (!arity(2, 2)) return std::nullopt;
            const auto* application = std::get_if<std::string>(&arguments[0]);
            const auto* section = std::get_if<std::string>(&arguments[1]);
            if (application == nullptr || section == nullptr) return bad_type();
            if (!execute_) return Value{Empty{}};
            const std::string prefix = *application + "\x01" + *section + "\x01";
            std::vector<Value> cells;
            for (const auto& [key, value] : settings_) {
                if (key.rfind(prefix, 0) == 0) {
                    std::string setting_name = key.substr(prefix.size());
                    if (!setting_name.empty() && setting_name.back() == '\x01') setting_name.pop_back();
                    cells.push_back(Value{std::move(setting_name)});
                    cells.push_back(Value{value});
                }
            }
            if (cells.empty()) return Value{Empty{}};
            ArrayValue result{};
            result.elements = std::move(cells);
            result.is_variant_element = true;
            result.element_type_index = Value{Empty{}}.index();
            result.dimensions = {{0, static_cast<Integer>(result.elements.size() / 2U) - 1}, {0, 1}};
            return Value{std::move(result)};
        }
        if (name == "getsetting") {
            if (!arity(3, 4)) return std::nullopt;
            std::string key;
            for (std::size_t i = 0; i < 3U; ++i) {
                const auto* part = std::get_if<std::string>(&arguments[i]);
                if (part == nullptr) return bad_type();
                key += *part + "\x01";
            }
            const auto found = settings_.find(key);
            if (found != settings_.end()) return Value{found->second};
            const auto* fallback = count == 4U ? std::get_if<std::string>(&arguments[3]) : nullptr;
            return Value{fallback != nullptr ? *fallback : std::string{}};
        }
        if (name == "cvdate") {
            if (!arity(1, 1)) return std::nullopt;
            if (const auto* text = std::get_if<std::string>(&arguments[0])) {
                if (const auto parsed = parse_date_text(*text)) return Value{DateValue{*parsed}};
                return bad_type();
            }
            if (std::holds_alternative<DateValue>(arguments[0])) return arguments[0];
            const auto number = number_at(0, 0.0);
            if (!number) return bad_type();
            return Value{DateValue{*number}};
        }
        if (name == "fileattr") {
            if (!arity(2, 2)) return std::nullopt;
            const auto file_number = number_at(0, 0.0);
            const auto kind = number_at(1, 1.0);
            if (!file_number || !kind) return bad_type();
            if (!execute_) return Value{Integer{}};
            auto* const file = find_open_file(static_cast<Integer>(*file_number), offset);
            if (file == nullptr) return std::nullopt;
            if (*kind == 2.0) return Value{static_cast<Integer>(*file_number)};
            constexpr Integer modes[] = {0, 1, 2, 8, 32, 4};
            return Value{modes[file->mode]};
        }
        if (name == "filedatetime" || name == "getattr") {
            if (!arity(1, 1)) return std::nullopt;
            const auto* path = std::get_if<std::string>(&arguments[0]);
            if (path == nullptr) return bad_type();
            if (!execute_) {
                return name == "getattr" ? Value{Integer{}} : Value{DateValue{}};
            }
            std::error_code ec;
            const std::filesystem::path target(*path);
            if (!std::filesystem::exists(target, ec)) {
                static_cast<void>(raise_runtime(53, "File not found", offset));
                return std::nullopt;
            }
            if (name == "getattr") {
                const auto permissions = std::filesystem::status(target, ec).permissions();
                const bool read_only = (permissions & std::filesystem::perms::owner_write) ==
                                       std::filesystem::perms::none;
                const Integer base = std::filesystem::is_directory(target, ec) ? 16 : 32;
                return Value{static_cast<Integer>(base | (read_only ? 1 : 0))};
            }
            const auto stamp = std::filesystem::last_write_time(target, ec);
            const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
                stamp.time_since_epoch()).count();
            // file_clock's epoch is implementation-defined; report the wall
            // clock "now" shifted by the file's age instead.
            const auto age = std::chrono::duration_cast<std::chrono::seconds>(
                std::filesystem::file_time_type::clock::now() - stamp).count();
            static_cast<void>(seconds);
            return Value{DateValue{current_date_serial() - static_cast<double>(age) / 86400.0}};
        }
        if (name == "rate") {
            if (!arity(3, 6)) return std::nullopt;
            const auto n = number_at(0, 0.0);
            const auto payment = number_at(1, 0.0);
            const auto pv = number_at(2, 0.0);
            const auto fv = number_at(3, 0.0);
            const auto type = number_at(4, 0.0);
            const auto guess = number_at(5, 0.1);
            if (!n || !payment || !pv || !fv || !type || !guess) return bad_type();
            if (!execute_) return Value{0.0};
            double r = *guess;
            const auto f = [&](const double rate) {
                if (rate == 0.0) return *pv + *payment * *n + *fv;
                const double g = std::pow(1.0 + rate, *n);
                return *pv * g + *payment * (1.0 + rate * *type) * (g - 1.0) / rate + *fv;
            };
            for (int i = 0; i < 100; ++i) {
                const double value = f(r);
                const double h = 1e-7;
                const double slope = (f(r + h) - value) / h;
                if (slope == 0.0 || !std::isfinite(slope)) break;
                const double next = r - value / slope;
                if (!std::isfinite(next)) break;
                if (std::fabs(next - r) < 1e-10) return finite_result(next);
                r = next;
            }
            return invalid_call();
        }
        if (name == "mirr") {
            if (!arity(3, 3)) return std::nullopt;
            const auto* values = std::get_if<ArrayValue>(&arguments[0]);
            const auto finance = number_at(1, 0.0);
            const auto reinvest = number_at(2, 0.0);
            if (values == nullptr || !finance || !reinvest) return bad_type();
            if (!execute_) return Value{0.0};
            double positive = 0.0;
            double negative = 0.0;
            const auto n = static_cast<double>(values->elements.size());
            for (std::size_t i = 0; i < values->elements.size(); ++i) {
                if (!is_number(values->elements[i])) return bad_type();
                const double v = as_double(values->elements[i]);
                if (v > 0.0) positive += v * std::pow(1.0 + *reinvest, n - 1.0 - static_cast<double>(i));
                if (v < 0.0) negative += v / std::pow(1.0 + *finance, static_cast<double>(i));
            }
            if (positive == 0.0 || negative == 0.0) return invalid_call();
            return finite_result(std::pow(positive / -negative, 1.0 / (n - 1.0)) - 1.0);
        }
        if (name == "cverr") {
            if (!arity(1, 1)) return std::nullopt;
            const auto number = number_at(0, 0.0);
            if (!number || *number < 0.0 || *number > 65535.0) return invalid_call();
            return Value{ErrorValue{static_cast<std::int32_t>(*number)}};
        }
        if (name == "command" || name == "command$") {
            if (!arity(0, 0)) return std::nullopt;
            return Value{std::string{}};
        }
        // pmt/fv/pv/nper share (rate, x, y[, z[, type]]).
        const auto fv_of = [](const double r, const double n, const double pmt, const double pv, const double type) {
            if (r == 0.0) return -(pv + pmt * n);
            const double growth = std::pow(1.0 + r, n);
            return -(pv * growth + pmt * (1.0 + r * type) * (growth - 1.0) / r);
        };
        const auto pmt_of = [](const double r, const double n, const double pv, const double fv, const double type) {
            if (r == 0.0) return -(pv + fv) / n;
            const double growth = std::pow(1.0 + r, n);
            return (-fv - pv * growth) * r / ((1.0 + r * type) * (growth - 1.0));
        };
        if (name == "pmt" || name == "fv" || name == "pv" || name == "nper") {
            if (!arity(3, 5)) return std::nullopt;
            const auto a = number_at(0, 0.0);
            const auto b = number_at(1, 0.0);
            const auto c = number_at(2, 0.0);
            const auto d = number_at(3, 0.0);
            const auto t = number_at(4, 0.0);
            if (!a || !b || !c || !d || !t) return bad_type();
            if (!execute_) return Value{0.0};
            const double r = *a;
            if (name == "pmt") {
                if (*b == 0.0) return invalid_call();
                return finite_result(pmt_of(r, *b, *c, *d, *t));
            }
            if (name == "fv") return finite_result(fv_of(r, *b, *c, *d, *t));
            if (name == "pv") {
                if (r == 0.0) return finite_result(-(*d + *c * *b));
                const double growth = std::pow(1.0 + r, *b);
                return finite_result((-*d - *c * (1.0 + r * *t) * (growth - 1.0) / r) / growth);
            }
            // nper(rate, pmt, pv, fv, type)
            if (r == 0.0) {
                if (*b == 0.0) return invalid_call();
                return finite_result(-(*c + *d) / *b);
            }
            const double top = *b * (1.0 + r * *t) - *d * r;
            const double bottom = *b * (1.0 + r * *t) + *c * r;
            if (top / bottom <= 0.0) return invalid_call();
            return finite_result(std::log(top / bottom) / std::log(1.0 + r));
        }
        if (name == "ipmt" || name == "ppmt") {
            if (!arity(4, 6)) return std::nullopt;
            const auto r = number_at(0, 0.0);
            const auto per = number_at(1, 0.0);
            const auto n = number_at(2, 0.0);
            const auto pv = number_at(3, 0.0);
            const auto fv = number_at(4, 0.0);
            const auto t = number_at(5, 0.0);
            if (!r || !per || !n || !pv || !fv || !t) return bad_type();
            if (!execute_) return Value{0.0};
            if (*per < 1.0 || *per > *n || *n <= 0.0) return invalid_call();
            const double payment = pmt_of(*r, *n, *pv, *fv, *t);
            double interest = 0.0;
            if (*t == 0.0) {
                interest = fv_of(*r, *per - 1.0, payment, *pv, 0.0) * *r;
            } else if (*per > 1.0) {
                interest = fv_of(*r, *per - 2.0, payment, *pv, 1.0) * *r;
            }
            return finite_result(name == "ipmt" ? interest : payment - interest);
        }
        if (name == "npv" || name == "irr") {
            if (!arity(2, name == "npv" ? 2U : 2U) && !(name == "irr" && count == 1U)) {
                return std::nullopt;
            }
            const ArrayValue* values = nullptr;
            const std::size_t array_index = name == "npv" ? 1U : 0U;
            if (array_index < count) {
                values = std::get_if<ArrayValue>(&arguments[array_index]);
            }
            const auto first = number_at(name == "npv" ? 0U : 1U, 0.1);
            if (values == nullptr || !first) return bad_type();
            if (!execute_) return Value{0.0};
            std::vector<double> flows;
            for (const auto& element : values->elements) {
                if (!is_number(element) || std::holds_alternative<Decimal>(element)) return bad_type();
                flows.push_back(as_double(element));
            }
            const auto npv_at = [&](const double r, const double shift) {
                double total = 0.0;
                for (std::size_t i = 0; i < flows.size(); ++i) {
                    total += flows[i] / std::pow(1.0 + r, static_cast<double>(i) + shift);
                }
                return total;
            };
            if (name == "npv") return finite_result(npv_at(*first, 1.0));
            bool has_positive = false, has_negative = false;
            for (const double f : flows) {
                has_positive = has_positive || f > 0.0;
                has_negative = has_negative || f < 0.0;
            }
            if (!has_positive || !has_negative) return invalid_call();
            double r = *first;
            for (int iteration = 0; iteration < 100; ++iteration) {
                const double f = npv_at(r, 0.0);
                const double h = 1e-7;
                const double slope = (npv_at(r + h, 0.0) - f) / h;
                if (slope == 0.0) break;
                const double next = r - f / slope;
                if (!std::isfinite(next) || next <= -1.0) return invalid_call();
                if (std::fabs(next - r) < 1e-10) {
                    return finite_result(next);
                }
                r = next;
            }
            return invalid_call();
        }
        if (name == "sln" || name == "syd" || name == "ddb") {
            if (!arity(name == "sln" ? 3U : 4U, name == "ddb" ? 5U : (name == "sln" ? 3U : 4U))) {
                return std::nullopt;
            }
            const auto cost = number_at(0, 0.0);
            const auto salvage = number_at(1, 0.0);
            const auto life = number_at(2, 0.0);
            const auto period = number_at(3, 1.0);
            const auto factor = number_at(4, 2.0);
            if (!cost || !salvage || !life || !period || !factor) return bad_type();
            if (!execute_) return Value{0.0};
            if (*life <= 0.0) return invalid_call();
            if (name == "sln") return finite_result((*cost - *salvage) / *life);
            if (*period < 1.0 || *period > *life) return invalid_call();
            if (name == "syd") {
                return finite_result(
                    (*cost - *salvage) * (*life - *period + 1.0) * 2.0 / (*life * (*life + 1.0)));
            }
            double total = 0.0;
            double depreciation = 0.0;
            for (int p = 1; p <= static_cast<int>(std::ceil(*period)); ++p) {
                depreciation = std::min((*cost - total) * *factor / *life, *cost - *salvage - total);
                if (depreciation < 0.0) depreciation = 0.0;
                total += depreciation;
            }
            return finite_result(depreciation);
        }
        if (name == "formatnumber" || name == "formatcurrency" || name == "formatpercent") {
            if (!arity(1, 5)) return std::nullopt;
            const auto value = number_at(0, 0.0);
            const auto digits = number_at(1, -1.0);
            if (!value || !digits) return bad_type();
            if (!execute_) return Value{std::string{}};
            const bool percent = name == "formatpercent";
            const bool currency = name == "formatcurrency";
            const int places = *digits < 0.0 ? 2 : static_cast<int>(*digits);
            double scaled = percent ? *value * 100.0 : *value;
            const bool negative = scaled < 0.0;
            char buffer[512];
            std::snprintf(buffer, sizeof(buffer), "%.*f", places, std::fabs(scaled));
            std::string digits_text = buffer;
            const auto point = digits_text.find('.');
            std::string whole = digits_text.substr(0, point);
            const std::string fraction = point == std::string::npos ? "" : digits_text.substr(point);
            if (count >= 3U && whole == "0") {
                bool include_leading = true;
                if (const auto* flag = std::get_if<bool>(&arguments[2])) include_leading = *flag;
                else if (const auto leading = number_at(2, -2.0)) include_leading = *leading != 0.0;
                if (!include_leading && !fraction.empty()) whole.clear();
            }
            bool group = true;
            if (count >= 5U) {
                if (const auto* g = std::get_if<bool>(&arguments[4])) group = *g;
                else if (const auto grouping = number_at(4, -2.0)) group = *grouping != 0.0;
            }
            if (group) {
                std::string grouped;
                for (std::size_t i = 0; i < whole.size(); ++i) {
                    if (i > 0 && (whole.size() - i) % 3 == 0) grouped.push_back(',');
                    grouped.push_back(whole[i]);
                }
                whole = grouped;
            }
            std::string text = whole + fraction;
            if (currency) text = "$" + text;
            if (percent) text += "%";
            bool parens = currency;
            if (count >= 4U) {
                if (const auto* p = std::get_if<bool>(&arguments[3])) parens = *p;
                else if (const auto flag = number_at(3, -2.0)) parens = *flag == -1.0 || *flag == 1.0;
            }
            if (negative && text.find_first_of("123456789") != std::string::npos) {
                text = parens ? "(" + text + ")" : "-" + text;
            }
            return Value{std::move(text)};
        }
        // partition(number, start, stop, interval)
        if (!arity(4, 4)) return std::nullopt;
        const auto number = number_at(0, 0.0);
        const auto start = number_at(1, 0.0);
        const auto stop = number_at(2, 0.0);
        const auto interval = number_at(3, 1.0);
        if (!number || !start || !stop || !interval) return bad_type();
        if (!execute_) return Value{std::string{}};
        if (*start < 0.0 || *stop <= *start || *interval < 1.0) return invalid_call();
        const auto width = std::to_string(static_cast<long long>(*stop) + 1).size();
        const auto pad = [&](const long long v) {
            std::string text = std::to_string(v);
            return std::string(text.size() < width ? width - text.size() : 0U, ' ') + text;
        };
        const std::string blank(width, ' ');
        const long long n = static_cast<long long>(std::floor(*number));
        const long long lo = static_cast<long long>(*start);
        const long long hi = static_cast<long long>(*stop);
        const long long step = static_cast<long long>(*interval);
        if (n < lo) return Value{blank + ":" + pad(lo - 1)};
        if (n > hi) return Value{pad(hi + 1) + ":" + blank};
        const long long lower = lo + (n - lo) / step * step;
        const long long upper = std::min(lower + step - 1, hi);
        return Value{pad(lower) + ":" + pad(upper)};
    }

    [[nodiscard]] static bool is_date_function_name(const std::string_view name) {
        static const std::unordered_set<std::string> names{
            "now", "date", "date$", "time", "time$", "timer", "year", "month", "day", "hour",
            "minute", "second", "weekday", "dateserial", "timeserial", "datevalue", "timevalue",
            "dateadd", "datediff", "datepart", "isdate", "cdate", "monthname", "weekdayname",
            "formatdatetime"};
        return names.contains(std::string(name));
    }

    [[nodiscard]] static double current_date_serial() {
        const std::time_t now = std::time(nullptr);
        std::tm local{};
#ifdef _WIN32
        localtime_s(&local, &now);
#else
        localtime_r(&now, &local);
#endif
        return date_serial(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday) +
               static_cast<double>(local.tm_hour * 3600 + local.tm_min * 60 + local.tm_sec) /
                   86400.0;
    }

    // REQ-0242: Date/Time intrinsics.
    [[nodiscard]] std::optional<Value> evaluate_date_function(
        const std::string_view name, std::vector<Value>& arguments, const std::size_t offset) {
        const auto count = arguments.size();
        const auto arity = [&](const std::size_t low, const std::size_t high) {
            if (count < low || count > high) {
                set_error("WFC0072", "function received the wrong number of arguments", offset);
                return false;
            }
            return true;
        };
        const auto mismatch = [&]() {
            set_error("WFC0073", "Type mismatch", offset);
            return std::nullopt;
        };
        // Any Date/number/parseable-String argument, as a serial.
        const auto serial_at = [&](const std::size_t index) -> std::optional<double> {
            const auto& v = arguments[index];
            if (const auto* d = std::get_if<DateValue>(&v)) return d->serial;
            if (is_number(v) && !std::holds_alternative<Decimal>(v)) return as_double(v);
            if (const auto* t = std::get_if<std::string>(&v)) return parse_date_text(*t);
            return std::nullopt;
        };
        const auto long_at = [&](const std::size_t index) -> std::optional<std::int64_t> {
            const auto& v = arguments[index];
            if (const auto* i = std::get_if<Integer>(&v)) return *i;
            if (const auto* i = std::get_if<Int16>(&v)) return *i;
            if (is_number(v) && !std::holds_alternative<Decimal>(v)) {
                return static_cast<std::int64_t>(std::nearbyint(as_double(v)));
            }
            return std::nullopt;
        };
        static const char* const month_names[] = {"January", "February", "March", "April",
            "May", "June", "July", "August", "September", "October", "November", "December"};
        static const char* const day_names[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
            "Thursday", "Friday", "Saturday"};

        if (name == "now" || name == "date" || name == "date$" || name == "time" ||
            name == "time$" || name == "timer") {
            if (!arity(0, 0)) return std::nullopt;
            if (name == "timer") {
                if (!execute_) return Value{0.0f};
                const double now = current_date_serial();
                return Value{static_cast<float>((now - std::floor(now)) * 86400.0)};
            }
            if (name.back() == '$') {
                if (!execute_) return Value{std::string{}};
                const auto parts = split_date(current_date_serial());
                char text[32];
                if (name[0] == 'd') {
                    std::snprintf(text, sizeof(text), "%02d-%02d-%04d",
                                  static_cast<int>(parts.month), static_cast<int>(parts.day),
                                  static_cast<int>(parts.year));
                } else {
                    std::snprintf(text, sizeof(text), "%02d:%02d:%02d",
                                  static_cast<int>(parts.hour), static_cast<int>(parts.minute),
                                  static_cast<int>(parts.second));
                }
                return Value{std::string{text}};
            }
            if (!execute_) return name[0] == 'n' ? Value{DateValue{}} : Value{DateValue{}};
            const double now = current_date_serial();
            if (name == "now") return Value{DateValue{now}};
            if (name[0] == 'd') return Value{DateValue{std::floor(now)}};
            return Value{DateValue{now - std::floor(now)}};
        }
        if (name == "year" || name == "month" || name == "day" || name == "hour" ||
            name == "minute" || name == "second") {
            if (!arity(1, 1)) return std::nullopt;
            const auto serial = serial_at(0);
            if (!serial.has_value()) return mismatch();
            const auto parts = split_date(*serial);
            const auto value = name == "year" ? parts.year : name == "month" ? parts.month
                : name == "day" ? parts.day : name == "hour" ? parts.hour
                : name == "minute" ? parts.minute : parts.second;
            return Value{static_cast<Integer>(value)};
        }
        if (name == "weekday") {
            if (!arity(1, 2)) return std::nullopt;
            const auto serial = serial_at(0);
            const auto first = count == 2U ? long_at(1) : std::optional<std::int64_t>{1};
            if (!serial.has_value() || !first.has_value()) return mismatch();
            const std::int64_t first_day = *first == 0 ? 1 : *first;
            if (first_day < 1 || first_day > 7) {
                set_error("WFC0101", "Invalid procedure call or argument", offset);
                return std::nullopt;
            }
            return Value{static_cast<Integer>(
                (split_date(*serial).weekday - first_day + 7) % 7 + 1)};
        }
        if (name == "dateserial") {
            if (!arity(3, 3)) return std::nullopt;
            const auto y = long_at(0);
            const auto m = long_at(1);
            const auto d = long_at(2);
            if (!y || !m || !d) return mismatch();
            std::int64_t year = *y;
            if (year >= 0 && year <= 99) year += year < 30 ? 2000 : 1900;
            return Value{DateValue{date_serial(year, *m, *d)}};
        }
        if (name == "timeserial") {
            if (!arity(3, 3)) return std::nullopt;
            const auto h = long_at(0);
            const auto m = long_at(1);
            const auto sec = long_at(2);
            if (!h || !m || !sec) return mismatch();
            const double total = static_cast<double>(*h * 3600 + *m * 60 + *sec) / 86400.0;
            return Value{DateValue{total - std::floor(total)}};
        }
        if (name == "datevalue" || name == "timevalue") {
            if (!arity(1, 1)) return std::nullopt;
            const auto serial = serial_at(0);
            if (!serial.has_value()) return mismatch();
            return Value{DateValue{name == "datevalue" ? std::floor(*serial)
                                                       : *serial - std::floor(*serial)}};
        }
        if (name == "cdate") {
            if (!arity(1, 1)) return std::nullopt;
            if (std::holds_alternative<Null>(arguments[0])) {
                set_error("WFC0104", "Invalid use of Null", offset);
                return std::nullopt;
            }
            const auto serial = serial_at(0);
            if (!serial.has_value()) return mismatch();
            if (*serial < -657435.0 || *serial >= 2958466.0) {
                set_error("WFC0009", "numeric overflow", offset);  // outside year 100..9999
                return std::nullopt;
            }
            return Value{DateValue{*serial}};
        }
        if (name == "isdate") {
            if (!arity(1, 1)) return std::nullopt;
            const auto& v = arguments[0];
            return Value{std::holds_alternative<DateValue>(v) ||
                         (std::holds_alternative<std::string>(v) && serial_at(0).has_value())};
        }
        if (name == "monthname") {
            if (!arity(1, 2)) return std::nullopt;
            const auto m = long_at(0);
            if (!m) return mismatch();
            if (*m < 1 || *m > 12) {
                set_error("WFC0101", "Invalid procedure call or argument", offset);
                return std::nullopt;
            }
            std::string text = month_names[*m - 1];
            if (count == 2U && std::holds_alternative<bool>(arguments[1]) &&
                std::get<bool>(arguments[1])) {
                text.resize(3);
            }
            return Value{std::move(text)};
        }
        if (name == "weekdayname") {
            if (!arity(1, 3)) return std::nullopt;
            const auto w = long_at(0);
            const auto first = count == 3U ? long_at(2) : std::optional<std::int64_t>{1};
            if (!w || !first) return mismatch();
            const std::int64_t first_day = *first == 0 ? 1 : *first;
            if (*w < 1 || *w > 7 || first_day < 1 || first_day > 7) {
                set_error("WFC0101", "Invalid procedure call or argument", offset);
                return std::nullopt;
            }
            std::string text = day_names[(*w - 1 + first_day - 1) % 7];
            if (count >= 2U && std::holds_alternative<bool>(arguments[1]) &&
                std::get<bool>(arguments[1])) {
                text.resize(3);
            }
            return Value{std::move(text)};
        }
        if (name == "formatdatetime") {
            if (!arity(1, 2)) return std::nullopt;
            const auto serial = serial_at(0);
            const auto style = count == 2U ? long_at(1) : std::optional<std::int64_t>{0};
            if (!serial || !style) return mismatch();
            const auto parts = split_date(*serial);
            char buffer[64];
            switch (*style) {
            case 0: return Value{render_date(*serial)};
            case 1:
                return Value{std::string(day_names[parts.weekday - 1]) + ", " +
                             month_names[parts.month - 1] + " " + std::to_string(parts.day) +
                             ", " + std::to_string(parts.year)};
            case 2: return Value{render_date_part(parts)};
            case 3: return Value{render_time_part(parts)};
            case 4:
                std::snprintf(buffer, sizeof(buffer), "%02lld:%02lld",
                              static_cast<long long>(parts.hour),
                              static_cast<long long>(parts.minute));
                return Value{std::string(buffer)};
            default:
                set_error("WFC0101", "Invalid procedure call or argument", offset);
                return std::nullopt;
            }
        }
        // DateAdd / DateDiff / DatePart: interval first.
        if (name == "dateadd" || name == "datediff" || name == "datepart") {
            if (!(name == "dateadd" ? arity(3, 3) : name == "datediff" ? arity(3, 5) : arity(2, 4))) {
                return std::nullopt;
            }
            const auto* interval_text = std::get_if<std::string>(&arguments[0]);
            if (interval_text == nullptr) return mismatch();
            std::string interval;
            for (const char c : *interval_text) interval.push_back(ascii_lower(c));
            static const std::unordered_set<std::string> intervals{
                "yyyy", "q", "m", "y", "d", "w", "ww", "h", "n", "s"};
            if (!intervals.contains(interval)) {
                set_error("WFC0101", "Invalid procedure call or argument", offset);
                return std::nullopt;
            }
            if (name == "dateadd") {
                const auto amount = long_at(1);
                const auto serial = serial_at(2);
                if (!amount || !serial) return mismatch();
                const auto n = *amount;
                if (interval == "yyyy" || interval == "q" || interval == "m") {
                    const auto parts = split_date(*serial);
                    const std::int64_t months = interval == "yyyy" ? n * 12 : interval == "q" ? n * 3 : n;
                    std::int64_t total = parts.year * 12 + (parts.month - 1) + months;
                    const std::int64_t year = total >= 0 ? total / 12 : -((11 - total) / 12);
                    const std::int64_t month = total - year * 12 + 1;
                    const std::int64_t day = std::min(parts.day, days_in_month(year, month));
                    const double time_of_day = *serial - std::floor(*serial);
                    return Value{DateValue{date_serial(year, month, day) + time_of_day}};
                }
                double delta = 0.0;
                if (interval == "d" || interval == "y" || interval == "w") delta = static_cast<double>(n);
                else if (interval == "ww") delta = static_cast<double>(n * 7);
                else if (interval == "h") delta = static_cast<double>(n) / 24.0;
                else if (interval == "n") delta = static_cast<double>(n) / 1440.0;
                else delta = static_cast<double>(n) / 86400.0;
                return Value{DateValue{*serial + delta}};
            }
            if (name == "datediff") {
                const auto first = serial_at(1);
                const auto second = serial_at(2);
                if (!first || !second) return mismatch();
                const auto a = split_date(*first);
                const auto b = split_date(*second);
                std::int64_t result{};
                if (interval == "yyyy") result = b.year - a.year;
                else if (interval == "m") result = (b.year * 12 + b.month) - (a.year * 12 + a.month);
                else if (interval == "q")
                    result = (b.year * 4 + (b.month - 1) / 3) - (a.year * 4 + (a.month - 1) / 3);
                else if (interval == "d" || interval == "y")
                    result = static_cast<std::int64_t>(std::floor(*second) - std::floor(*first));
                else if (interval == "w")
                    result = static_cast<std::int64_t>(std::floor(*second) - std::floor(*first)) / 7;
                else if (interval == "ww") {
                    std::int64_t first_day = 1;
                    if (const auto fd = arguments.size() > 3U ? long_at(3) : std::nullopt) {
                        first_day = *fd == 0 ? 1 : *fd;
                    }
                    const auto week_start = [first_day](const double v) {
                        return std::floor(v) -
                               static_cast<double>((split_date(v).weekday - first_day + 7) % 7);
                    };
                    result = static_cast<std::int64_t>((week_start(*second) - week_start(*first)) / 7.0);
                } else if (interval == "h")
                    result = static_cast<std::int64_t>(std::floor(*second * 24.0 + 1e-9) - std::floor(*first * 24.0 + 1e-9));
                else if (interval == "n")
                    result = static_cast<std::int64_t>(std::floor(*second * 1440.0 + 1e-7) - std::floor(*first * 1440.0 + 1e-7));
                else
                    result = static_cast<std::int64_t>(std::llround(*second * 86400.0) - std::llround(*first * 86400.0));
                return Value{static_cast<Integer>(result)};
            }
            const auto serial = serial_at(1);
            if (!serial) return mismatch();
            const auto parts = split_date(*serial);
            std::int64_t result{};
            const auto day_of_year = static_cast<std::int64_t>(
                std::floor(*serial) - date_serial(parts.year, 1, 1)) + 1;
            if (interval == "yyyy") result = parts.year;
            else if (interval == "q") result = (parts.month - 1) / 3 + 1;
            else if (interval == "m") result = parts.month;
            else if (interval == "y") result = day_of_year;
            else if (interval == "d") result = parts.day;
            else if (interval == "w") result = parts.weekday;
            else if (interval == "ww") {
                std::int64_t first_day = 1;
                std::int64_t first_week = 1;
                if (const auto fd = arguments.size() > 2U ? long_at(2) : std::nullopt) {
                    first_day = *fd == 0 ? 1 : *fd;
                }
                if (const auto fw = arguments.size() > 3U ? long_at(3) : std::nullopt) {
                    first_week = *fw == 0 ? 1 : *fw;
                }
                if (first_day < 1 || first_day > 7 || first_week < 1 || first_week > 3) {
                    set_error("WFC0101", "Invalid procedure call or argument", offset);
                    return std::nullopt;
                }
                result = vb_week_of_year(*serial, first_day, first_week);
            } else if (interval == "h") result = parts.hour;
            else if (interval == "n") result = parts.minute;
            else result = parts.second;
            return Value{static_cast<Integer>(result)};
        }
        set_error("WFC0071", "unsupported function", offset);
        return std::nullopt;
    }

    [[nodiscard]] std::optional<Value> parse_function_call(
        const std::string_view identifier,
        const std::size_t identifier_offset) {
        const bool dry_run = !execute_;
        auto result = parse_function_call_impl(identifier, identifier_offset);
        if (dry_run && !result.has_value()) {
            // A not-taken branch can hold placeholder arguments (a member
            // of `Nothing`), so a value-type complaint there is not an
            // error; syntax and arity errors still are.
            const std::string_view code = std::string_view(error_.diagnostic).substr(0, 7);
            if (code == "WFC0073" || code == "WFC0095" || code == "WFC0018" ||
                code == "WFC0016" || code == "WFC0007" || code == "WFC0101" ||
                (code != "WFC0300" && code != "WFC0072" && code != "WFC0071" &&
                 runtime_error_number() != 0)) {
                error_ = wfc::Evaluation{};
                static const std::set<std::string, std::less<>> string_results = {
                    "left", "right", "mid", "trim", "ltrim", "rtrim", "lcase", "ucase", "replace",
                    "string", "space", "chr", "hex", "oct", "str", "cstr", "format", "join",
                    "strreverse", "left$", "right$", "mid$", "trim$", "ltrim$", "rtrim$", "lcase$",
                    "ucase$", "chr$", "hex$", "oct$", "str$", "format$", "space$", "string$",
                    "typename", "strconv", "formatnumber", "formatcurrency", "formatpercent",
                    "formatdatetime", "monthname", "weekdayname", "environ", "environ$"};
                if (string_results.contains(std::string(identifier))) {
                    return Value{std::string{}};
                }
                return Value{Integer{}};
            }
        }
        // REQ-0247: CByte's range-checked Long result is a Byte.
        if (identifier == "cbyte" && result.has_value()) {
            if (const auto* number = std::get_if<Integer>(&*result)) {
                return Value{static_cast<Byte>(*number)};
            }
        }
        return result;
    }

    [[nodiscard]] std::optional<Value> parse_function_call_impl(
        const std::string_view identifier,
        const std::size_t identifier_offset) {
        const bool is_len = identifier == "len" || identifier == "lenb";
        const bool is_lower = identifier == "lcase" || identifier == "lcase$";
        const bool is_upper = identifier == "ucase" || identifier == "ucase$";
        const bool is_left_trim = identifier == "ltrim" || identifier == "ltrim$";
        const bool is_right_trim = identifier == "rtrim" || identifier == "rtrim$";
        const bool is_trim = identifier == "trim" || identifier == "trim$";
        const bool is_left = identifier == "left" || identifier == "left$" ||
                             identifier == "leftb" || identifier == "leftb$";
        const bool is_right = identifier == "right" || identifier == "right$" ||
                              identifier == "rightb" || identifier == "rightb$";
        const bool is_mid = identifier == "mid" || identifier == "mid$" ||
                            identifier == "midb" || identifier == "midb$";
        const bool is_asc = identifier == "asc" || identifier == "ascb" ||
                            identifier == "ascw";
        const bool is_chr_b = identifier == "chrb" || identifier == "chrb$";
        const bool is_chr = identifier == "chr" || identifier == "chr$" ||
                            identifier == "chrw" || identifier == "chrw$" || is_chr_b;
        const bool is_reverse = identifier == "strreverse";
        const bool is_space = identifier == "space" || identifier == "space$";
        const bool is_string = identifier == "string" || identifier == "string$";
        const bool is_instr = identifier == "instr" || identifier == "instrb";
        const bool is_instr_rev = identifier == "instrrev";
        const bool is_strcomp = identifier == "strcomp";
        const bool is_replace = identifier == "replace";
        const bool is_hex = identifier == "hex" || identifier == "hex$";
        const bool is_oct = identifier == "oct" || identifier == "oct$";
        const bool is_str = identifier == "str" || identifier == "str$";
        const bool is_val = identifier == "val";
        const bool is_abs = identifier == "abs";
        const bool is_sgn = identifier == "sgn";
        const bool is_cstr = identifier == "cstr";
        const bool is_clng = identifier == "clng";
        const bool is_cbool = identifier == "cbool";
        const bool is_cbyte = identifier == "cbyte";
        const bool is_cint = identifier == "cint";
        const bool is_cdbl = identifier == "cdbl";
        const bool is_csng = identifier == "csng";
        const bool is_ccur = identifier == "ccur";
        const bool is_cvar = identifier == "cvar";
        const bool is_cdec = identifier == "cdec";
        const bool is_macid = identifier == "macid";
        const bool is_error_message = identifier == "error" || identifier == "error$";
        const bool is_isnumeric = identifier == "isnumeric";
        const bool is_typename = identifier == "typename";
        const bool is_vartype = identifier == "vartype";
        const bool is_iif = identifier == "iif";
        const bool is_choose = identifier == "choose";
        const bool is_switch = identifier == "switch";
        const bool is_int = identifier == "int";
        const bool is_fix = identifier == "fix";
        const bool is_round = identifier == "round";
        const bool is_sqr = identifier == "sqr";
        const bool is_sin = identifier == "sin";
        const bool is_cos = identifier == "cos";
        const bool is_tan = identifier == "tan";
        const bool is_atn = identifier == "atn";
        const bool is_exp = identifier == "exp";
        const bool is_log = identifier == "log";
        const bool is_float_math =
            is_sqr || is_sin || is_cos || is_tan || is_atn || is_exp || is_log;
        const bool is_isarray = identifier == "isarray";
        const bool is_isobject = identifier == "isobject";
        const bool is_isnull = identifier == "isnull";
        const bool is_isempty = identifier == "isempty";
        const bool is_iserror = identifier == "iserror";
        const bool is_ismissing = identifier == "ismissing";
        const bool is_constant_false_predicate = is_iserror;
        const bool is_lbound = identifier == "lbound";
        const bool is_ubound = identifier == "ubound";
        const bool is_qbcolor = identifier == "qbcolor";
        const bool is_rgb = identifier == "rgb";
        const bool is_strconv = identifier == "strconv";
        const bool is_format = identifier == "format" || identifier == "format$";
        const bool is_rnd = identifier == "rnd";
        const bool is_array_fn = identifier == "array";
        const bool is_split = identifier == "split";
        const bool is_join = identifier == "join";
        const bool is_filter = identifier == "filter";
        const bool is_date_fn = is_date_function_name(identifier);
        const bool is_file_fn = is_file_function_name(identifier);
        const bool is_misc_fn = is_misc_function_name(identifier);
        if (!is_len && !is_lower && !is_upper && !is_left_trim && !is_right_trim &&
            !is_trim && !is_left && !is_right && !is_mid && !is_asc && !is_chr &&
            !is_reverse && !is_space && !is_string && !is_instr && !is_strcomp &&
            !is_instr_rev && !is_replace && !is_hex && !is_oct && !is_str && !is_val &&
            !is_abs && !is_sgn && !is_cstr && !is_clng && !is_cbool && !is_cbyte &&
            !is_cint && !is_isnumeric && !is_typename && !is_vartype && !is_iif &&
            !is_choose && !is_switch && !is_int && !is_fix &&
            !is_constant_false_predicate && !is_qbcolor && !is_rgb && !is_strconv &&
            !is_round && !is_cdbl && !is_csng && !is_ccur && !is_cvar && !is_macid &&
            !is_error_message && !is_float_math && !is_format && !is_rnd &&
            !is_isnull && !is_isempty && !is_cdec && !is_ismissing &&
            !is_isarray && !is_isobject && !is_lbound && !is_ubound &&
            !is_array_fn && !is_split && !is_join && !is_filter && !is_date_fn && !is_file_fn && !is_misc_fn) {
            set_error("WFC0071", "unsupported function", identifier_offset);
            return std::nullopt;
        }
        if (constant_expression_) {
            set_error(
                "WFC0074",
                "constant initializer cannot call a function",
                identifier_offset);
            return std::nullopt;
        }

        // `IsMissing(paramName)` (REQ-0224) takes its own parameter's bare
        // *name*, not an evaluated expression -- unlike every other
        // intrinsic function's arguments, parsed generically below. An
        // omitted Optional Variant argument is bound to a real (default)
        // value indistinguishable from a caller-supplied one by the time
        // it would reach the generic `parse_expression()` loop, so the
        // only way to answer correctly is to look the name up directly
        // against `missing_parameter_names`, recorded by `invoke_definition`
        // at the moment the omission was still known. A parameter that
        // is not Optional/Variant, or an identifier that does not name a
        // parameter of the current procedure at all (including at module
        // level), answers `False` -- matching this evaluator's existing
        // constant-`False` stub for every case except the one this
        // requirement now gives a real answer for.
        if (is_ismissing) {
            skip_horizontal_whitespace();
            if (!consume('(')) {
                set_error(
                    "WFC0072", "function received the wrong number of arguments",
                    identifier_offset);
                return std::nullopt;
            }
            skip_horizontal_whitespace();
            if (!at_end() && current() == ')') {
                set_error(
                    "WFC0072", "function received the wrong number of arguments", offset_);
                return std::nullopt;
            }
            const auto parameter_offset = offset_;
            char type_character{};
            auto parameter_name = parse_identifier(&type_character);
            if (!parameter_name.has_value() || type_character != '\0') {
                set_error("WFC0011", "IsMissing requires a parameter name", parameter_offset);
                return std::nullopt;
            }
            skip_horizontal_whitespace();
            if (!at_end() && current() == ',') {
                set_error(
                    "WFC0072", "function received the wrong number of arguments", offset_);
                return std::nullopt;
            }
            if (!consume(')')) {
                set_error("WFC0005", "expected closing parenthesis", offset_);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{false};
            }
            // Membership in `missing_parameter_names` already implies
            // Optional/Variant/no-default (see where it is populated
            // above); confirming the name is at least a real parameter
            // of the current procedure first (rather than checking the
            // set directly) keeps `IsMissing(someUnrelatedName)` a plain
            // False instead of an accidental True from a stale name.
            bool is_missing = false;
            if (in_procedure() && current_procedure_def_ != nullptr) {
                for (const auto& parameter : current_procedure_def_->parameters) {
                    if (parameter.name == *parameter_name) {
                        is_missing = current_scope().missing_parameter_names.contains(*parameter_name);
                        break;
                    }
                }
            }
            return Value{is_missing};
        }

        // A missing `(` is a parenthesis-free, zero-argument call (REQ-0213)
        // -- e.g. `Print Rnd` -- reachable only through
        // parse_primary_base's bare-identifier intrinsic-function fallback,
        // never through the ordinary `identifier(...)` call site (which
        // only reaches here once it has already confirmed `(`). The
        // function's own arity check below rejects this with `WFC0072` if
        // it actually requires one or more arguments.
        std::vector<Value> arguments;
        if (consume('(')) {
            skip_horizontal_whitespace();
            if (!consume(')')) {
                while (true) {
                    if (!at_end() && current() == ',') {
                        // An omitted optional slot of Replace/InStr/InStrRev
                        // takes that parameter's default.
                        const auto slot = arguments.size();
                        const Integer compare_default = option_compare_text_ ? 1 : 0;
                        std::optional<Integer> default_value;
                        if (identifier == "replace") {
                            if (slot == 3U) default_value = 1;
                            else if (slot == 4U) default_value = -1;
                            else if (slot == 5U) default_value = compare_default;
                        } else if (identifier == "instr") {
                            if (slot == 0U) default_value = 1;
                            else if (slot == 3U) default_value = compare_default;
                        } else if (identifier == "instrrev") {
                            if (slot == 2U) default_value = -1;
                            else if (slot == 3U) default_value = compare_default;
                        } else if (identifier == "formatnumber" || identifier == "formatcurrency" ||
                                   identifier == "formatpercent") {
                            if (slot == 1U) default_value = -1;
                            else if (slot >= 2U && slot <= 4U) default_value = -2;  // vbUseDefault
                        }
                        if (!default_value.has_value()) {
                            set_error("WFC0072", "function received an empty argument", offset_);
                            return std::nullopt;
                        }
                        arguments.push_back(Value{*default_value});
                        advance();
                        skip_horizontal_whitespace();
                        continue;
                    }
                    auto argument = parse_expression();
                    if (!argument.has_value()) {
                        return std::nullopt;
                    }
                    arguments.push_back(std::move(*argument));
                    skip_horizontal_whitespace();
                    if (consume(')')) {
                        break;
                    }
                    if (!consume(',')) {
                        set_error("WFC0005", "expected closing parenthesis", offset_);
                        return std::nullopt;
                    }
                    skip_horizontal_whitespace();
                    if (consume(')')) {
                        set_error(
                            "WFC0072", "function received an empty argument", offset_ - 1U);
                        return std::nullopt;
                    }
                }
            }
        }

        {
            // Numeric strings are accepted where a math routine wants a number.
            static const std::set<std::string, std::less<>> math_functions = {
                "abs", "sgn", "int", "fix", "sqr", "sin", "cos", "tan", "atn", "exp", "log",
                "round"};
            if (!arguments.empty() && math_functions.contains(std::string(identifier))) {
                if (const auto* text = std::get_if<std::string>(&arguments[0])) {
                    const auto parsed = parse_numeric_string(*text);
                    if (parsed.status == NumericStringStatus::valid) {
                        arguments[0] = Value{parsed.value};
                    }
                }
            }
            if (identifier == "strcomp" && execute_ && arguments.size() >= 2U &&
                (std::holds_alternative<Null>(arguments[0]) ||
                 std::holds_alternative<Null>(arguments[1]))) {
                return Value{Null{}};
            }
        }
        {
            // `Integer` (Int16) and `Byte` arguments reach library routines as
            // Long, except for functions whose result depends on the subtype.
            static const std::set<std::string, std::less<>> keep_subtype = {
                "typename", "vartype", "hex", "hex$", "oct", "oct$", "isnumeric", "isempty",
                "isnull", "isobject", "isarray", "isdate", "iserror", "ismissing", "cvar", "cstr",
                "cbool", "cbyte", "cint", "clng", "csng", "cdbl", "ccur", "cdec", "cdate", "cvdate",
                "cverr", "abs", "sgn", "int", "fix", "format", "format$", "str", "str$", "len",
                "lenb", "array", "iif", "switch", "isnumeric", "varptr", "formatdatetime"};
            const bool is_choose_fn = identifier == "choose";
            const bool keep_first = identifier == "round";  // Round(Integer) stays Integer
            if (!keep_subtype.contains(std::string(identifier))) {
                for (std::size_t index = keep_first ? 1U : 0U; index < arguments.size(); ++index) {
                    if (is_choose_fn && index > 0U) break;
                    if (const auto* short_value = std::get_if<Int16>(&arguments[index])) {
                        arguments[index] = Value{static_cast<Integer>(*short_value)};
                    } else if (const auto* byte_value = std::get_if<Byte>(&arguments[index])) {
                        arguments[index] = Value{static_cast<Integer>(*byte_value)};
                    }
                }
            }
        }
        if (is_date_fn) {
            static const std::set<std::string, std::less<>> null_propagating_dates = {
                "year", "month", "day", "hour", "minute", "second", "weekday", "dateadd",
                "datediff", "datepart"};
            if (execute_ && null_propagating_dates.contains(std::string(identifier)) &&
                std::any_of(arguments.begin(), arguments.end(), [](const Value& value) {
                    return std::holds_alternative<Null>(value);
                })) {
                return Value{Null{}};
            }
            return evaluate_date_function(identifier, arguments, identifier_offset);
        }
        if (is_file_fn) {
            return evaluate_file_function(identifier, arguments, identifier_offset);
        }
        if (is_misc_fn) {
            return evaluate_misc_function(identifier, arguments, identifier_offset);
        }
        // REQ-0247: functions other than the type probes see a Byte as a Long.
        if (!is_typename && !is_vartype && !is_cvar && !is_iif && !is_choose && !is_switch &&
            !is_isnumeric && !is_isarray && !is_isobject && !is_isnull && !is_isempty) {
            for (auto& argument : arguments) {
                if (const auto* byte = std::get_if<Byte>(&argument)) {
                    argument = Value{static_cast<Integer>(*byte)};
                }
            }
        }
        // REQ-0259: an error-subtype Variant reaching a function that needs a
        // real value raises the error it carries.
        if (!is_typename && !is_vartype && !is_cvar && !is_iif && !is_choose && !is_switch &&
            !is_isnumeric && !is_isarray && !is_isobject && !is_isnull && !is_isempty &&
            !is_constant_false_predicate && !is_cstr && !is_ismissing && execute_) {
            for (const auto& argument : arguments) {
                if (const auto* error_value = std::get_if<ErrorValue>(&argument)) {
                    static_cast<void>(raise_runtime(
                        error_value->code == 0 ? 5 : error_value->code,
                        vb_error_description(error_value->code), identifier_offset));
                    return std::nullopt;
                }
            }
        }
        // REQ-0242: numeric conversions/functions see a Date as its serial.
        if (!arguments.empty() && std::holds_alternative<DateValue>(arguments[0]) &&
            (is_cdbl || is_csng || is_clng || is_cint || is_ccur || is_cdec || is_cbyte ||
             is_cbool || is_int || is_fix || is_round || is_abs || is_sgn)) {
            arguments[0] = Value{std::get<DateValue>(arguments[0]).serial};
        }
        bool valid_arity{};
        if (is_array_fn) {
            valid_arity = true;
        } else if (is_split || is_filter) {
            valid_arity = arguments.size() >= (is_split ? 1U : 2U) && arguments.size() <= 4U;
        } else if (is_join) {
            valid_arity = arguments.size() == 1U || arguments.size() == 2U;
        } else if (is_error_message || is_rnd) {
            valid_arity = arguments.size() <= 1U;
        } else if (is_mid) {
            valid_arity = arguments.size() == 2U || arguments.size() == 3U;
        } else if (is_instr || is_instr_rev) {
            valid_arity = arguments.size() >= 2U && arguments.size() <= 4U;
        } else if (is_replace) {
            valid_arity = arguments.size() >= 3U && arguments.size() <= 6U;
        } else if (is_strcomp) {
            valid_arity = arguments.size() == 2U || arguments.size() == 3U;
        } else if (is_round || is_format || is_lbound || is_ubound) {
            valid_arity = arguments.size() == 1U || arguments.size() == 2U;
        } else if (is_iif || is_rgb) {
            valid_arity = arguments.size() == 3U;
        } else if (is_choose) {
            valid_arity = arguments.size() >= 2U;
        } else if (is_switch) {
            valid_arity = arguments.size() >= 2U && arguments.size() % 2U == 0U;
        } else if (is_left || is_right || is_string || is_strconv) {
            valid_arity = arguments.size() == 2U;
        } else {
            valid_arity = arguments.size() == 1U;
        }
        if (!valid_arity) {
            set_error(
                "WFC0072",
                "function received the wrong number of arguments",
                identifier_offset);
            return std::nullopt;
        }


        // REQ-0239: Array/Split/Join/Filter.
        if (is_array_fn) {
            ArrayValue result{
                std::move(arguments), /*lower_bound=*/option_base_one_ ? 1 : 0,
                /*is_dynamic=*/false, /*is_allocated=*/true, Value{Empty{}}.index()};
            result.is_variant_element = true;
            return Value{std::move(result)};
        }
        if (is_split || is_join || is_filter) {
            const auto make_string_array = [](std::vector<std::string> parts) {
                std::vector<Value> elements;
                elements.reserve(parts.size());
                for (auto& part : parts) {
                    elements.emplace_back(std::move(part));
                }
                return Value{ArrayValue{
                    std::move(elements), /*lower_bound=*/0, /*is_dynamic=*/false,
                    /*is_allocated=*/true, Value{std::string{}}.index()}};
            };
            const auto text_argument = [&](const std::size_t index) -> const std::string* {
                return std::get_if<std::string>(&arguments[index]);
            };
            if (!execute_) {
                return is_join ? Value{std::string{}} : make_string_array({});
            }
            if (is_split) {
                const auto* text = text_argument(0);
                std::string delimiter = " ";
                if (arguments.size() >= 2U) {
                    const auto* delimiter_argument = text_argument(1);
                    if (delimiter_argument == nullptr) {
                        set_error("WFC0073", "Split requires a String delimiter", identifier_offset);
                        return std::nullopt;
                    }
                    delimiter = *delimiter_argument;
                }
                Integer limit = -1;
                if (arguments.size() >= 3U) {
                    const auto* limit_argument = std::get_if<Integer>(&arguments[2]);
                    if (limit_argument == nullptr) {
                        set_error("WFC0073", "Split limit must be Long", identifier_offset);
                        return std::nullopt;
                    }
                    limit = *limit_argument;
                }
                if (text == nullptr) {
                    set_error("WFC0073", "Split requires a String argument", identifier_offset);
                    return std::nullopt;
                }
                if (text->empty() || limit == 0) {
                    return make_string_array({});
                }
                bool text_compare = option_compare_text_;
                if (arguments.size() >= 4U) {
                    if (const auto* mode = std::get_if<Integer>(&arguments[3])) {
                        text_compare = *mode == 1 || (*mode == -1 && option_compare_text_);
                    } else if (const auto* mode16 = std::get_if<Int16>(&arguments[3])) {
                        text_compare = *mode16 == 1 || (*mode16 == -1 && option_compare_text_);
                    }
                }
                std::string search_text = *text;
                if (text_compare) {
                    search_text = fold_case(search_text);
                    delimiter = fold_case(delimiter);
                }
                std::vector<std::string> parts;
                if (delimiter.empty()) {
                    parts.push_back(*text);
                } else {
                    std::size_t position = 0;
                    while (limit < 0 || static_cast<Integer>(parts.size()) < limit - 1) {
                        const auto found = search_text.find(delimiter, position);
                        if (found == std::string::npos) {
                            break;
                        }
                        parts.push_back(text->substr(position, found - position));
                        position = found + delimiter.size();
                    }
                    parts.push_back(text->substr(position));
                }
                return make_string_array(std::move(parts));
            }
            const auto* array = std::get_if<ArrayValue>(&arguments[0]);
            if (array == nullptr || !array->dimensions.empty()) {
                set_error(
                    "WFC0073", "function requires a one-dimensional array argument",
                    identifier_offset);
                return std::nullopt;
            }
            if (is_join) {
                std::string delimiter = " ";
                if (arguments.size() == 2U) {
                    const auto* delimiter_argument = text_argument(1);
                    if (delimiter_argument == nullptr) {
                        set_error("WFC0073", "Join requires a String delimiter", identifier_offset);
                        return std::nullopt;
                    }
                    delimiter = *delimiter_argument;
                }
                std::string joined;
                bool first = true;
                for (const auto& element : array->elements) {
                    if (std::holds_alternative<Null>(element) || is_object_reference(element) ||
                        std::holds_alternative<ArrayValue>(element)) {
                        set_error("WFC0073", "Join element must be a scalar", identifier_offset);
                        return std::nullopt;
                    }
                    if (!first) {
                        joined += delimiter;
                    }
                    first = false;
                    joined += render(element);
                }
                return Value{std::move(joined)};
            }
            const auto* match = text_argument(1);
            if (match == nullptr) {
                set_error("WFC0073", "Filter requires a String match", identifier_offset);
                return std::nullopt;
            }
            bool include = true;
            if (arguments.size() >= 3U) {
                const auto* include_argument = std::get_if<bool>(&arguments[2]);
                if (include_argument == nullptr) {
                    set_error("WFC0073", "Filter include must be Boolean", identifier_offset);
                    return std::nullopt;
                }
                include = *include_argument;
            }
            bool text_compare = option_compare_text_;
            if (arguments.size() == 4U) {
                const auto* compare_argument = std::get_if<Integer>(&arguments[3]);
                if (compare_argument == nullptr) {
                    set_error("WFC0073", "Filter compare must be Long", identifier_offset);
                    return std::nullopt;
                }
                text_compare = *compare_argument == 1;
            }
            const auto fold = [&](std::string text) {
                if (text_compare) {
                    text = fold_case(text);
                }
                return text;
            };
            const std::string needle = fold(*match);
            std::vector<std::string> kept;
            for (const auto& element : array->elements) {
                const std::string text = render(element);
                if ((fold(text).find(needle) != std::string::npos) == include) {
                    kept.push_back(text);
                }
            }
            return make_string_array(std::move(kept));
        }

        if (is_chr) {
            const auto* character_code = std::get_if<Integer>(&arguments[0]);
            if (character_code == nullptr) {
                set_error("WFC0073", "Chr requires a Long argument", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            const bool wide = identifier == "chrw" || identifier == "chrw$";
            const Integer maximum = wide ? 65535 : 255;
            if (*character_code < 0 || *character_code > maximum) {
                set_error(
                    "WFC0078",
                    is_chr_b ? "ChrB code must be in the byte range"
                             : "Chr code must be in the character range",
                    identifier_offset);
                return std::nullopt;
            }
            if (*character_code >= 0x80 && !is_chr_b) {
                // Chr maps through Windows-1252; ChrW is the code unit itself.
                const auto code = static_cast<unsigned>(*character_code);
                std::string utf8;
                append_utf8_unit(utf8, wide ? code : ansi_to_unicode(code));
                return Value{std::move(utf8)};
            }
            return Value{std::string(1U, static_cast<char>(*character_code))};
        }

        if (is_abs || is_sgn) {
            if (!is_number(arguments[0])) {
                set_error(
                    "WFC0073",
                    is_abs ? "Abs requires a numeric argument" : "Sgn requires a numeric argument",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return is_abs ? arguments[0] : Value{Integer{}};
            }
            if (is_sgn) {
                const double number = as_double(arguments[0]);
                return Value{static_cast<Integer>((number > 0.0) - (number < 0.0))};
            }
            if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
                if (*integer == std::numeric_limits<Integer>::min()) {
                    set_error("WFC0009", "integer overflow", identifier_offset);
                    return std::nullopt;
                }
                return Value{*integer < 0 ? static_cast<Integer>(-*integer) : *integer};
            }
            if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
                if (*short_integer == std::numeric_limits<Int16>::min()) {
                    set_error("WFC0009", "integer overflow", identifier_offset);
                    return std::nullopt;
                }
                return Value{
                    *short_integer < 0 ? static_cast<Int16>(-*short_integer) : *short_integer};
            }
            if (const auto* single = std::get_if<float>(&arguments[0])) {
                return Value{std::abs(*single)};
            }
            if (const auto* currency = std::get_if<Currency>(&arguments[0])) {
                if (currency->scaled == std::numeric_limits<std::int64_t>::min()) {
                    set_error("WFC0009", "integer overflow", identifier_offset);
                    return std::nullopt;
                }
                return Value{Currency{currency->scaled < 0 ? -currency->scaled : currency->scaled}};
            }
            if (const auto* decimal = std::get_if<Decimal>(&arguments[0])) {
                Decimal result = *decimal;
                result.negative = false;
                return Value{result};
            }
            return Value{std::abs(std::get<double>(arguments[0]))};
        }

        if (is_qbcolor) {
            const auto* color = std::get_if<Integer>(&arguments[0]);
            if (color == nullptr) {
                set_error("WFC0073", "QBColor requires a Long color index", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{Integer{}};
            }
            if (*color < 0 || *color > 15) {
                set_error("WFC0092", "QBColor index must be from 0 through 15", identifier_offset);
                return std::nullopt;
            }
            constexpr Integer colors[] = {
                0x000000, 0x800000, 0x008000, 0x808000,
                0x000080, 0x800080, 0x008080, 0xC0C0C0,
                0x808080, 0xFF0000, 0x00FF00, 0xFFFF00,
                0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF};
            return Value{colors[*color]};
        }

        if (is_rgb) {
            Integer component[3]{};
            for (std::size_t index = 0U; index < 3U; ++index) {
                const auto* value = std::get_if<Integer>(&arguments[index]);
                if (value == nullptr) {
                    set_error(
                        "WFC0073",
                        "RGB requires Long red, green, and blue components",
                        identifier_offset);
                    return std::nullopt;
                }
                component[index] = *value;
            }
            if (!execute_) {
                return Value{Integer{}};
            }
            for (Integer& value : component) {
                if (value < 0) {
                    set_error(
                        "WFC0091",
                        "RGB component must be non-negative",
                        identifier_offset);
                    return std::nullopt;
                }
                if (value > 255) {
                    value = 255;  // VB6 assumes any component above 255 is 255.
                }
            }
            return Value{static_cast<Integer>(
                component[0] + component[1] * 256 + component[2] * 65536)};
        }

        if (is_constant_false_predicate) {
            return Value{execute_ && std::holds_alternative<ErrorValue>(arguments[0])};
        }
        if (is_isnull) {
            return Value{execute_ && std::holds_alternative<Null>(arguments[0])};
        }

        if (is_isempty) {
            return Value{execute_ && std::holds_alternative<Empty>(arguments[0])};
        }

        if (is_isarray) {
            return Value{execute_ && std::holds_alternative<ArrayValue>(arguments[0])};
        }

        if (is_isobject) {
            // Verified VB6 fact: IsObject(Nothing) is True -- Nothing is
            // still an object reference (just an unset one), distinct from
            // Null/Empty. A live instance is of course an object too.
            return Value{execute_ && is_object_reference(arguments[0])};
        }

        if (is_lbound || is_ubound) {
            const auto* array = std::get_if<ArrayValue>(&arguments[0]);
            if (array == nullptr) {
                set_error(
                    "WFC0073",
                    is_lbound ? "LBound requires an array argument"
                              : "UBound requires an array argument",
                    identifier_offset);
                return std::nullopt;
            }
            // The optional second argument is a 1-based dimension number
            // (REQ-0210), defaulting to 1 -- the only meaningful value for
            // an ordinary 1-D array.
            Integer dimension = 1;
            if (arguments.size() == 2U) {
                const auto* dimension_argument = std::get_if<Integer>(&arguments[1]);
                if (dimension_argument == nullptr) {
                    set_error(
                        "WFC0073", "LBound/UBound dimension must be Long", identifier_offset);
                    return std::nullopt;
                }
                dimension = *dimension_argument;
            }
            if (!execute_) {
                return Value{Integer{}};
            }
            if (!array->is_allocated) {
                // Matches real VB6: LBound/UBound on a dynamic array before
                // its first ReDim raises the same "subscript out of range"
                // error an out-of-bounds index does, rather than silently
                // answering from an empty [0, -1] range. Checked before the
                // dimension-count range check below, since an unallocated
                // array's own dimension count may itself only be
                // provisional (REQ-0219's `dynamic_dimension_count`), not
                // yet the authoritative shape a dimension argument should
                // be validated against.
                set_error("WFC0111", "array subscript out of range", identifier_offset);
                return std::nullopt;
            }
            const std::size_t dimension_count =
                array->dimensions.empty() ? std::size_t{1} : array->dimensions.size();
            if (dimension < 1 || static_cast<std::size_t>(dimension) > dimension_count) {
                set_error(
                    "WFC0148", "LBound/UBound dimension is out of range", identifier_offset);
                return std::nullopt;
            }
            if (array->dimensions.empty()) {
                return Value{
                    is_lbound
                        ? array->lower_bound
                        : array->lower_bound + static_cast<Integer>(array->elements.size()) - 1};
            }
            const auto& dimension_bound =
                array->dimensions[static_cast<std::size_t>(dimension) - 1U];
            return Value{is_lbound ? dimension_bound.first : dimension_bound.second};
        }

        if (is_cdec) {
            if (const auto* decimal = std::get_if<Decimal>(&arguments[0])) {
                return Value{execute_ ? *decimal : Decimal{}};
            }
            if (std::holds_alternative<Null>(arguments[0])) {
                set_error("WFC0104", "Invalid use of Null", identifier_offset);
                return std::nullopt;
            }
            if (is_object_reference(arguments[0]) ||
                std::holds_alternative<ArrayValue>(arguments[0])) {
                set_error("WFC0105", "CDec requires a numeric value", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{Decimal{}};
            }
            if (std::holds_alternative<Empty>(arguments[0])) {
                return Value{Decimal{}};
            }
            if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
                Decimal result;
                result.negative = *integer < 0;
                result.mantissa = big_from_u32(static_cast<std::uint32_t>(
                    *integer < 0 ? -static_cast<std::int64_t>(*integer) : *integer));
                return Value{result};
            }
            if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
                Decimal result;
                result.negative = *short_integer < 0;
                result.mantissa = big_from_u32(static_cast<std::uint32_t>(
                    *short_integer < 0 ? -static_cast<std::int32_t>(*short_integer)
                                       : *short_integer));
                return Value{result};
            }
            if (const auto* currency = std::get_if<Currency>(&arguments[0])) {
                Decimal result;
                result.negative = currency->scaled < 0;
                const auto magnitude = currency->scaled < 0
                    ? (~static_cast<std::uint64_t>(currency->scaled) + 1ULL)
                    : static_cast<std::uint64_t>(currency->scaled);
                result.mantissa.limb[0] = static_cast<std::uint32_t>(magnitude);
                result.mantissa.limb[1] = static_cast<std::uint32_t>(magnitude >> 32U);
                result.scale = 4U;
                return Value{result};
            }
            if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
                Decimal result;
                result.negative = *boolean;
                result.mantissa = big_from_u32(*boolean ? 1U : 0U);
                return Value{result};
            }
            double numeric_value{};
            if (const auto* number = std::get_if<double>(&arguments[0])) {
                numeric_value = *number;
            } else if (const auto* single = std::get_if<float>(&arguments[0])) {
                numeric_value = static_cast<double>(*single);
            } else {
                const auto parsed =
                    parse_decimal_string(std::get<std::string>(arguments[0]));
                if (parsed.status == NumericStringStatus::out_of_range) {
                    set_error("WFC0009", "numeric overflow", identifier_offset);
                    return std::nullopt;
                }
                if (parsed.status != NumericStringStatus::valid) {
                    set_error("WFC0105", "CDec requires a numeric value", identifier_offset);
                    return std::nullopt;
                }
                return Value{parsed.value};
            }
            const auto converted = decimal_from_double(numeric_value);
            if (!converted.has_value()) {
                set_error("WFC0009", "numeric overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{*converted};
        }

        if (is_int || is_fix) {
            if (!is_number(arguments[0])) {
                set_error(
                    "WFC0073",
                    is_int ? "Int requires a numeric argument" : "Fix requires a numeric argument",
                    identifier_offset);
                return std::nullopt;
            }
            if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
                return Value{execute_ ? *integer : Integer{}};
            }
            if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
                return Value{execute_ ? *short_integer : Int16{}};
            }
            if (!execute_) {
                return Value{0.0};
            }
            if (const auto* single = std::get_if<float>(&arguments[0])) {
                return Value{is_int ? std::floor(*single) : std::trunc(*single)};
            }
            if (const auto* currency = std::get_if<Currency>(&arguments[0])) {
                const std::int64_t scaled = currency->scaled;
                const std::int64_t remainder = scaled % 10000;
                std::int64_t truncated = scaled - remainder;
                if (is_int && remainder != 0 && scaled < 0) {
                    truncated -= 10000;
                }
                return Value{Currency{truncated}};
            }
            if (const auto* decimal = std::get_if<Decimal>(&arguments[0])) {
                if (decimal->scale == 0U) {
                    return Value{*decimal};
                }
                BigUInt quotient;
                BigUInt remainder;
                divide_big(decimal->mantissa, power_of_ten_big(decimal->scale), quotient, remainder);
                Decimal truncated;
                truncated.negative = decimal->negative;
                truncated.mantissa = quotient;
                if (is_int && decimal->negative && !is_zero_big(remainder)) {
                    truncated.mantissa = add_big(truncated.mantissa, big_from_u32(1U));
                }
                if (is_zero_big(truncated.mantissa)) {
                    truncated.negative = false;
                }
                return Value{truncated};
            }
            const double number = std::get<double>(arguments[0]);
            return Value{is_int ? std::floor(number) : std::trunc(number)};
        }

        if (is_float_math) {
            if (!is_number(arguments[0])) {
                set_error(
                    "WFC0073",
                    "math function requires a numeric argument",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{0.0};
            }
            const double argument = as_double(arguments[0]);
            double result{};
            if (is_sqr) {
                if (argument < 0.0) {
                    set_error("WFC0096", "Sqr argument must be non-negative", identifier_offset);
                    return std::nullopt;
                }
                result = std::sqrt(argument);
            } else if (is_log) {
                if (argument <= 0.0) {
                    set_error("WFC0096", "Log argument must be positive", identifier_offset);
                    return std::nullopt;
                }
                result = std::log(argument);
            } else if (is_sin) {
                result = std::sin(argument);
            } else if (is_cos) {
                result = std::cos(argument);
            } else if (is_tan) {
                result = std::tan(argument);
            } else if (is_atn) {
                result = std::atan(argument);
            } else {
                result = std::exp(argument);  // is_exp
            }
            if (!std::isfinite(result)) {
                set_error("WFC0009", "numeric overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{result};
        }

        if (is_round) {
            if (!is_number(arguments[0])) {
                set_error("WFC0073", "Round requires a numeric argument", identifier_offset);
                return std::nullopt;
            }
            Integer digits = 0;
            if (arguments.size() == 2U) {
                const auto* requested_digits = std::get_if<Integer>(&arguments[1]);
                if (requested_digits == nullptr) {
                    set_error(
                        "WFC0073",
                        "Round requires a Long digit count",
                        identifier_offset);
                    return std::nullopt;
                }
                digits = *requested_digits;
                if (execute_ && digits < 0) {
                    set_error(
                        "WFC0094",
                        "Round digit count must be non-negative",
                        identifier_offset);
                    return std::nullopt;
                }
            }
            if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
                return Value{execute_ ? *integer : Integer{}};
            }
            if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
                return Value{execute_ ? *short_integer : Int16{}};
            }
            if (!execute_) {
                return Value{0.0};
            }
            if (const auto* single = std::get_if<float>(&arguments[0])) {
                if (digits >= std::numeric_limits<float>::max_digits10) {
                    return Value{*single};
                }
                const float scale = std::pow(10.0f, static_cast<float>(digits));
                if (std::abs(*single) > std::numeric_limits<float>::max() / scale) {
                    return Value{*single};
                }
                return Value{std::nearbyint(*single * scale) / scale};
            }
            if (const auto* currency = std::get_if<Currency>(&arguments[0])) {
                if (digits >= 4) {
                    return Value{*currency};
                }
                static constexpr std::int64_t divisors[4] = {10000, 1000, 100, 10};
                const std::int64_t divisor = divisors[digits];
                const std::int64_t scaled = currency->scaled;
                const std::int64_t quotient = scaled / divisor;
                const std::int64_t remainder = scaled % divisor;
                const std::int64_t abs_remainder = remainder < 0 ? -remainder : remainder;
                const std::int64_t half = divisor / 2;
                std::int64_t rounded_quotient = quotient;
                if (abs_remainder > half ||
                    (abs_remainder == half && (quotient % 2 != 0))) {
                    rounded_quotient += (scaled < 0 ? -1 : 1);
                }
                return Value{Currency{rounded_quotient * divisor}};
            }
            if (const auto* decimal = std::get_if<Decimal>(&arguments[0])) {
                if (digits >= decimal->scale) {
                    return Value{*decimal};
                }
                Decimal rounded = *decimal;
                const Integer reduce_by = static_cast<Integer>(decimal->scale) - digits;
                for (Integer step = 0; step < reduce_by; ++step) {
                    rounded.mantissa = divide_by_ten_rounded_big(rounded.mantissa);
                }
                rounded.scale = static_cast<std::uint8_t>(digits);
                if (is_zero_big(rounded.mantissa)) {
                    rounded.negative = false;
                }
                return Value{rounded};
            }
            const double number = std::get<double>(arguments[0]);
            if (digits >= std::numeric_limits<double>::max_digits10) {
                return Value{number};
            }
            const double scale = std::pow(10.0, static_cast<double>(digits));
            if (std::abs(number) > std::numeric_limits<double>::max() / scale) {
                return Value{number};
            }
            return Value{std::nearbyint(number * scale) / scale};
        }

        if (is_format) {
            if (arguments.size() == 1U) {
                return Value{execute_ ? render(arguments[0]) : std::string{}};
            }
            const auto* style = std::get_if<std::string>(&arguments[1]);
            if (style == nullptr) {
                set_error("WFC0073", "Format requires a String Style argument", identifier_offset);
                return std::nullopt;
            }
            if (const auto* date_argument = std::get_if<DateValue>(&arguments[0])) {
                return Value{execute_ ? format_date_pattern(date_argument->serial, *style)
                                      : std::string{}};
            }
            if (is_number(arguments[0]) && style->find_first_of("@&") != std::string::npos &&
                style->find_first_of("0#") == std::string::npos) {
                arguments[0] = Value{render(arguments[0])};  // `@@@@` formats the digits as text
            }
            if (const auto* text_argument = std::get_if<std::string>(&arguments[0])) {
                // REQ-0264: string formats -- `@`/`&` placeholders, `<`, `>`, `!`.
                if (!execute_) {
                    return Value{std::string{}};
                }
                std::string fmt = *style;
                const auto section = fmt.find(';');
                if (section != std::string::npos) {
                    fmt = text_argument->empty() && fmt.find(';', section + 1) != std::string::npos
                              ? fmt.substr(fmt.find(';', section + 1) + 1)
                              : fmt.substr(0, section);
                }
                bool upper = false, lower = false, left_fill = false;
                std::string mask;
                for (std::size_t i = 0; i < fmt.size(); ++i) {
                    const char c = fmt[i];
                    if (c == '>') upper = true;
                    else if (c == '<') lower = true;
                    else if (c == '!') left_fill = true;
                    else if (c == '\\' && i + 1 < fmt.size()) mask += std::string{'\x01', fmt[++i]};
                    else if (c == '"') {
                        while (++i < fmt.size() && fmt[i] != '"') mask += std::string{'\x01', fmt[i]};
                    } else mask.push_back(c);
                }
                std::string source_text = *text_argument;
                for (char& c : source_text) {
                    if (upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                    if (lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                std::size_t placeholders = 0;
                for (std::size_t i = 0; i < mask.size(); ++i) {
                    if (mask[i] == '\x01') { ++i; continue; }
                    if (mask[i] == '@' || mask[i] == '&') ++placeholders;
                }
                if (placeholders == 0) {
                    return Value{source_text};
                }
                std::string out;
                // `@` pads with a space, `&` with nothing; characters fill right to left
                // unless `!` asks for left to right.
                std::vector<char> chars(source_text.begin(), source_text.end());
                std::size_t next = left_fill ? 0 : (chars.size() > placeholders ? chars.size() - placeholders : 0);
                const std::size_t skip = left_fill ? 0 : (placeholders > chars.size() ? placeholders - chars.size() : 0);
                std::size_t seen = 0;
                std::string tail;
                if (left_fill && chars.size() > placeholders) {
                    tail = source_text.substr(placeholders);
                }
                if (!left_fill && chars.size() > placeholders) {
                    out = source_text.substr(0, chars.size() - placeholders);
                }
                for (std::size_t i = 0; i < mask.size(); ++i) {
                    if (mask[i] == '\x01') { out.push_back(mask[++i]); continue; }
                    if (mask[i] == '@' || mask[i] == '&') {
                        const bool pad = !left_fill && seen < skip;
                        if (pad) {
                            if (mask[i] == '@') out.push_back(' ');
                        } else if (next < chars.size()) {
                            out.push_back(chars[next++]);
                        } else if (mask[i] == '@') {
                            out.push_back(' ');
                        }
                        ++seen;
                    } else {
                        out.push_back(mask[i]);
                    }
                }
                return Value{out + tail};
            }
            if (std::holds_alternative<Null>(arguments[0])) {
                return Value{Null{}};
            }
            if (std::holds_alternative<Empty>(arguments[0])) {
                return Value{std::string{}};
            }
            if (!is_number(arguments[0]) && !std::holds_alternative<bool>(arguments[0])) {
                set_error(
                    "WFC0073",
                    "Format with a Style argument requires a Long, Double, or Boolean expression",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            std::string lowered_style = *style;
            for (char& character : lowered_style) {
                character = ascii_lower(character);
            }
            // An empty Style behaves exactly like the one-argument form
            // (REQ-0218), matching real VB6.
            if (style->empty() || lowered_style == "general number") {
                if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
                    return Value{std::to_string(*integer)};
                }
                if (std::holds_alternative<bool>(arguments[0])) {
                    return Value{std::string{std::get<bool>(arguments[0]) ? "-1" : "0"}};
                }
                return Value{render(arguments[0])};
            }
            const double widened = std::holds_alternative<bool>(arguments[0])
                                        ? (std::get<bool>(arguments[0]) ? -1.0 : 0.0)
                                        : as_double(arguments[0]);
            if (lowered_style == "yes/no") {
                return Value{std::string{widened != 0.0 ? "Yes" : "No"}};
            }
            if (lowered_style == "true/false") {
                return Value{std::string{widened != 0.0 ? "True" : "False"}};
            }
            if (lowered_style == "on/off") {
                return Value{std::string{widened != 0.0 ? "On" : "Off"}};
            }
            if (lowered_style == "fixed") {
                return Value{render_fixed_style(widened, false)};
            }
            if (lowered_style == "standard") {
                return Value{render_fixed_style(widened, true)};
            }
            if (lowered_style == "currency") {
                // A disclosed, unverified-against-the-reference-runtime
                // simplification (REQ-0193): real VB6's Currency style
                // uses the *system locale's* currency symbol and negative-
                // value convention (commonly parenthesized, e.g.
                // "($1,234.50)" under a US locale), neither of which this
                // evaluator has any notion of. This renders a fixed
                // US-dollar-sign prefix ahead of the same grouped,
                // two-decimal "Standard" magnitude, with a leading '-' for
                // a negative value (matching every other numeric style's
                // own negative-sign convention) rather than parentheses.
                std::string rendered = render_fixed_style(widened, true);
                const std::string::size_type dollar_position =
                    (!rendered.empty() && rendered.front() == '-') ? 1U : 0U;
                rendered.insert(dollar_position, "$");
                return Value{std::move(rendered)};
            }
            if (lowered_style == "percent") {
                const double scaled = widened * 100.0;
                if (!std::isfinite(scaled)) {
                    set_error("WFC0009", "numeric overflow", identifier_offset);
                    return std::nullopt;
                }
                return Value{render_fixed_style(scaled, false) + "%"};
            }
            if (lowered_style == "scientific") {
                return Value{render_scientific_style(widened)};
            }
            // Any Style that names none of the reserved styles above is a
            // custom numeric picture (REQ-0218), matching real VB6: it is
            // never rejected outright, only rendered character by
            // character (retiring the old WFC0102 "unsupported Style"
            // diagnostic this branch used to report unconditionally).
            return Value{render_custom_numeric_picture(widened, *style)};
        }

        if (is_rnd) {
            double argument{};
            if (!arguments.empty()) {
                if (!is_number(arguments[0]) && !std::holds_alternative<bool>(arguments[0])) {
                    set_error("WFC0073", "Rnd requires a numeric argument", identifier_offset);
                    return std::nullopt;
                }
                argument = std::holds_alternative<bool>(arguments[0])
                               ? (std::get<bool>(arguments[0]) ? -1.0 : 0.0)
                               : as_double(arguments[0]);
            }
            if (!execute_) {
                return Value{0.0f};
            }
            if (!arguments.empty() && argument == 0.0) {
                return Value{rnd_last_value_};
            }
            rnd_state_ = (!arguments.empty() && argument < 0.0) ? seed_from_number(argument)
                                                                 : rnd_step(rnd_state_);
            // REQ-0195's own Scope explicitly deferred this: real VB6's Rnd
            // returns Single, not Double. Narrowing rnd_value's double
            // result to float here (rather than computing state / 2^24 in
            // float from the start) is exact for this specific case --
            // dividing by a power of two is exact/correctly-rounded in
            // both precisions, and rounding a correctly-rounded double
            // result to the nearest float gives the same answer a genuine
            // single-precision division would.
            rnd_last_value_ = static_cast<float>(rnd_value(rnd_state_));
            return Value{rnd_last_value_};
        }

        if (is_cstr) {
            // Verified: CStr(Null), like CBool(Null), raises "Invalid use of
            // Null" rather than returning a string.
            if (std::holds_alternative<Null>(arguments[0])) {
                set_error("WFC0104", "Invalid use of Null", identifier_offset);
                return std::nullopt;
            }
            if (is_object_reference(arguments[0])) {
                set_error("WFC0106", "Invalid use of Nothing", identifier_offset);
                return std::nullopt;
            }
            if (std::holds_alternative<ArrayValue>(arguments[0])) {
                set_error("WFC0073", "CStr does not accept an array argument", identifier_offset);
                return std::nullopt;
            }
            return Value{execute_ ? render(arguments[0]) : std::string{}};
        }

        if (is_typename) {
            if (!execute_) {
                return Value{std::string{}};
            }
            if (std::holds_alternative<Integer>(arguments[0])) {
                return Value{std::string{"Long"}};
            }
            if (std::holds_alternative<Int16>(arguments[0])) {
                return Value{std::string{"Integer"}};
            }
            if (std::holds_alternative<double>(arguments[0])) {
                return Value{std::string{"Double"}};
            }
            if (std::holds_alternative<float>(arguments[0])) {
                return Value{std::string{"Single"}};
            }
            if (std::holds_alternative<Currency>(arguments[0])) {
                return Value{std::string{"Currency"}};
            }
            if (std::holds_alternative<Byte>(arguments[0])) {
                return Value{std::string{"Byte"}};
            }
            if (std::holds_alternative<ErrorValue>(arguments[0])) {
                return Value{std::string{"Error"}};
            }
            if (std::holds_alternative<DateValue>(arguments[0])) {
                return Value{std::string{"Date"}};
            }
            if (std::holds_alternative<Decimal>(arguments[0])) {
                return Value{std::string{"Decimal"}};
            }
            if (std::holds_alternative<bool>(arguments[0])) {
                return Value{std::string{"Boolean"}};
            }
            // Verified against the local VB6 6.00.8176 reference:
            // TypeName(Null) = "Null", TypeName(Empty) = "Empty".
            if (std::holds_alternative<Null>(arguments[0])) {
                return Value{std::string{"Null"}};
            }
            if (std::holds_alternative<Empty>(arguments[0])) {
                return Value{std::string{"Empty"}};
            }
            // Verified VB6 fact: TypeName(Nothing) = "Nothing".
            if (std::holds_alternative<Nothing>(arguments[0])) {
                return Value{std::string{"Nothing"}};
            }
            // A live instance's TypeName is its own class's name (its
            // as-supplied spelling, not the lowercased lookup key).
            if (const auto* instance = std::get_if<ObjectInstance>(&arguments[0])) {
                const auto& shown = class_definitions_.at(instance->data->class_name).display_name;
                if (shown == "WfcDictionary") return Value{std::string{"Dictionary"}};
                if (shown == "WfcFileSystemObject") return Value{std::string{"FileSystemObject"}};
                if (shown == "WfcTextStream") return Value{std::string{"TextStream"}};
                if (shown == "WfcFile") return Value{std::string{"File"}};
                if (shown == "WfcRegExp") return Value{std::string{"RegExp"}};
                if (shown == "WfcMatchCollection") return Value{std::string{"MatchCollection"}};
                if (shown == "WfcMatch") return Value{std::string{"Match"}};
                if (shown == "WfcSubMatches") return Value{std::string{"SubMatches"}};
                return Value{shown};
            }
            if (const auto* array = std::get_if<ArrayValue>(&arguments[0])) {
                // Real VB6 renders an array's TypeName as its element type
                // name plus "()", e.g. "Long()", "Variant()", "Object()".
                // Uses the array's declared element type (not its current
                // first element, which may not exist for an unallocated
                // dynamic array, and which -- for a Variant/Object-element
                // array, REQ-0212 -- is not fixed at all).
                if (array->is_variant_element) {
                    return Value{std::string{"Variant()"}};
                }
                if (array->is_object_element) {
                    // A class-typed array element (REQ-0214) renders its
                    // own class's display name, the same as a live
                    // instance does; the generic `As Object` form (no
                    // declared class) renders "Object()".
                    if (!array->element_class_name.empty()) {
                        return Value{
                            class_definitions_.at(array->element_class_name).display_name + "()"};
                    }
                    return Value{std::string{"Object()"}};
                }
                return Value{
                    element_type_name(array_element_default(array->element_type_index)) + "()"};
            }
            return Value{std::string{"String"}};
        }

        if (is_vartype) {
            if (!execute_) {
                return Value{Integer{}};
            }
            if (std::holds_alternative<Integer>(arguments[0])) {
                return Value{Integer{3}};
            }
            if (std::holds_alternative<Int16>(arguments[0])) {
                return Value{Integer{2}};  // vbInteger
            }
            if (std::holds_alternative<double>(arguments[0])) {
                return Value{Integer{5}};
            }
            if (std::holds_alternative<float>(arguments[0])) {
                return Value{Integer{4}};
            }
            if (std::holds_alternative<Currency>(arguments[0])) {
                return Value{Integer{6}};
            }
            if (std::holds_alternative<Byte>(arguments[0])) {
                return Value{Integer{17}};
            }
            if (std::holds_alternative<ErrorValue>(arguments[0])) {
                return Value{Integer{10}};
            }
            if (std::holds_alternative<DateValue>(arguments[0])) {
                return Value{Integer{7}};
            }
            if (std::holds_alternative<Decimal>(arguments[0])) {
                return Value{Integer{14}};  // vbDecimal
            }
            if (std::holds_alternative<bool>(arguments[0])) {
                return Value{Integer{11}};
            }
            // Verified against the local VB6 6.00.8176 reference:
            // VarType(Empty) = 0 (vbEmpty), VarType(Null) = 1 (vbNull).
            if (std::holds_alternative<Empty>(arguments[0])) {
                return Value{Integer{0}};
            }
            if (std::holds_alternative<Null>(arguments[0])) {
                return Value{Integer{1}};
            }
            // Verified VB6 fact: VarType(Nothing) = 9 (vbObject). A live
            // instance is also vbObject: VarType never encodes which class,
            // only that the value is an object reference.
            if (is_object_reference(arguments[0])) {
                return Value{Integer{9}};
            }
            if (const auto* array = std::get_if<ArrayValue>(&arguments[0])) {
                // Real VB6 ORs the element VarType with vbArray (8192). Uses
                // the array's declared element type, the same as TypeName
                // above.
                if (array->is_variant_element) {
                    return Value{Integer{12 + 8192}};  // vbVariant Or vbArray
                }
                if (array->is_object_element) {
                    return Value{Integer{9 + 8192}};  // vbObject Or vbArray
                }
                return Value{
                    Integer{element_vartype_code(array_element_default(array->element_type_index)) +
                             8192}};
            }
            return Value{Integer{8}};
        }

        if (is_iif) {
            bool iif_flag = false;
            const bool* condition = nullptr;
            if (const auto converted = coerce_condition_boolean(
                    arguments[0], identifier_offset, "WFC0021", "IIf condition must be Boolean")) {
                iif_flag = *converted;
                condition = &iif_flag;
            }
            if (condition == nullptr) {
                return std::nullopt;
            }
            if (!execute_) {
                return arguments[1];
            }
            return *condition ? arguments[1] : arguments[2];
        }

        if (is_choose) {
            const auto* index = std::get_if<Integer>(&arguments[0]);
            if (index == nullptr) {
                set_error("WFC0073", "Choose requires a Long index", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return arguments[1];
            }
            const auto choice_count = static_cast<Integer>(arguments.size() - 1U);
            if (*index < 1 || *index > choice_count) {
                return Value{Null{}};  // VB: an out-of-range index yields Null
            }
            return arguments[static_cast<std::size_t>(*index)];
        }

        if (is_switch) {
            for (std::size_t pair = 0U; pair < arguments.size(); pair += 2U) {
                const auto* condition = std::get_if<bool>(&arguments[pair]);
                if (condition == nullptr && execute_) {
                    set_error(
                        "WFC0021",
                        "Switch expressions must be Boolean",
                        identifier_offset);
                    return std::nullopt;
                }
                if (execute_ && *condition) {
                    return arguments[pair + 1U];
                }
            }
            if (!execute_) {
                return arguments[1];
            }
            return Value{Null{}};  // VB: no expression matched
        }

        if (is_isnumeric) {
            if (!execute_) {
                return Value{false};
            }
            // Per documented VB6 behavior (not independently probed this
            // session): IsNumeric(Empty) is True (Empty coerces to 0, a
            // number), and IsNumeric(Null) is False.
            if (std::holds_alternative<Null>(arguments[0]) ||
                is_object_reference(arguments[0]) ||
                std::holds_alternative<ArrayValue>(arguments[0])) {
                return Value{false};
            }
            if (std::holds_alternative<Integer>(arguments[0]) ||
                std::holds_alternative<Int16>(arguments[0]) ||
                std::holds_alternative<bool>(arguments[0]) ||
                std::holds_alternative<float>(arguments[0]) ||
                std::holds_alternative<double>(arguments[0]) ||
                std::holds_alternative<Currency>(arguments[0]) ||
                std::holds_alternative<Decimal>(arguments[0]) ||
                std::holds_alternative<Empty>(arguments[0])) {
                return Value{true};
            }

            if (!std::holds_alternative<std::string>(arguments[0])) {
                return Value{false};  // Date, error values, ...
            }
            const auto parsed = parse_numeric_string(std::get<std::string>(arguments[0]));
            return Value{parsed.status == NumericStringStatus::valid};
        }

        if (is_cbyte) {
            if (std::holds_alternative<Null>(arguments[0])) {
                set_error("WFC0104", "Invalid use of Null", identifier_offset);
                return std::nullopt;
            }
            if (!is_number(arguments[0]) &&
                !std::holds_alternative<std::string>(arguments[0]) &&
                !std::holds_alternative<bool>(arguments[0]) &&
                !std::holds_alternative<Empty>(arguments[0])) {
                set_error("WFC0073", "CByte requires a numeric argument", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{Integer{}};
            }
            if (std::holds_alternative<Empty>(arguments[0])) {
                return Value{Integer{0}};
            }

            if (const auto* number = std::get_if<Integer>(&arguments[0])) {
                if (*number < 0 || *number > 255) {
                    set_error("WFC0009", "integer overflow", identifier_offset);
                    return std::nullopt;
                }
                return Value{*number};
            }
            if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
                if (*short_integer < 0 || *short_integer > 255) {
                    set_error("WFC0009", "integer overflow", identifier_offset);
                    return std::nullopt;
                }
                return Value{static_cast<Integer>(*short_integer)};
            }
            if (const auto* number = std::get_if<double>(&arguments[0])) {
                return round_double_to_long(*number, 0, 255, identifier_offset);
            }
            if (const auto* single = std::get_if<float>(&arguments[0])) {
                return round_double_to_long(
                    static_cast<double>(*single), 0, 255, identifier_offset);
            }
            if (std::holds_alternative<Currency>(arguments[0]) ||
                std::holds_alternative<Decimal>(arguments[0])) {
                return round_double_to_long(
                    as_double(arguments[0]), 0, 255, identifier_offset);
            }
            if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
                return Value{*boolean ? Integer{255} : Integer{0}};
            }

            const auto parsed = parse_numeric_string(std::get<std::string>(arguments[0]));
            if (parsed.status == NumericStringStatus::out_of_range) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            if (parsed.status != NumericStringStatus::valid) {
                set_error("WFC0098", "CByte requires a numeric value", identifier_offset);
                return std::nullopt;
            }
            return round_double_to_long(parsed.value, 0, 255, identifier_offset);
        }

        if (is_error_message) {
            if (!execute_) {
                return Value{std::string{}};
            }
            if (arguments.empty()) {
                return Value{std::string{}};
            }
            const auto* number = std::get_if<Integer>(&arguments[0]);
            if (number == nullptr) {
                set_error("WFC0073", "Error requires a Long argument", identifier_offset);
                return std::nullopt;
            }
            if (*number < 0 || *number > 65535) {
                set_error("WFC0101", "Error number is outside the valid range", identifier_offset);
                return std::nullopt;
            }
            return Value{vb_error_description(*number)};
        }

        if (is_cvar) {
            return arguments[0];
        }

        if (is_cdbl || is_csng) {
            if (std::holds_alternative<Null>(arguments[0])) {
                set_error("WFC0104", "Invalid use of Null", identifier_offset);
                return std::nullopt;
            }
            if (is_object_reference(arguments[0]) ||
                std::holds_alternative<ArrayValue>(arguments[0])) {
                set_error(
                    "WFC0073",
                    is_cdbl ? "CDbl requires a numeric value" : "CSng requires a numeric value",
                    identifier_offset);
                return std::nullopt;
            }
            double value{};
            if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
                value = static_cast<double>(*integer);
            } else if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
                value = static_cast<double>(*short_integer);
            } else if (const auto* number = std::get_if<double>(&arguments[0])) {
                value = *number;
            } else if (const auto* single = std::get_if<float>(&arguments[0])) {
                value = static_cast<double>(*single);
            } else if (std::holds_alternative<Currency>(arguments[0]) ||
                       std::holds_alternative<Decimal>(arguments[0])) {
                value = as_double(arguments[0]);
            } else if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
                value = *boolean ? -1.0 : 0.0;
            } else if (std::holds_alternative<Empty>(arguments[0])) {
                value = 0.0;
            } else {
                if (!execute_) {
                    return is_csng ? Value{0.0f} : Value{0.0};
                }
                const auto parsed =
                    parse_numeric_string(std::get<std::string>(arguments[0]));
                if (parsed.status == NumericStringStatus::out_of_range) {
                    set_error("WFC0009", "numeric overflow", identifier_offset);
                    return std::nullopt;
                }
                if (parsed.status != NumericStringStatus::valid) {
                    set_error(
                        "WFC0095",
                        is_cdbl ? "CDbl requires a numeric value"
                                : "CSng requires a numeric value",
                        identifier_offset);
                    return std::nullopt;
                }
                value = parsed.value;
            }
            if (!execute_) {
                return is_csng ? Value{0.0f} : Value{0.0};
            }
            if (is_csng) {
                const auto narrowed = static_cast<float>(value);
                if (!std::isfinite(narrowed)) {
                    set_error("WFC0009", "numeric overflow", identifier_offset);
                    return std::nullopt;
                }
                return Value{narrowed};
            }
            return Value{value};
        }

        if (is_ccur) {
            if (std::holds_alternative<Null>(arguments[0])) {
                set_error("WFC0104", "Invalid use of Null", identifier_offset);
                return std::nullopt;
            }
            if (is_object_reference(arguments[0]) ||
                std::holds_alternative<ArrayValue>(arguments[0])) {
                set_error("WFC0073", "CCur requires a numeric value", identifier_offset);
                return std::nullopt;
            }
            if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
                if (!execute_) {
                    return Value{Currency{}};
                }
                return Value{Currency{static_cast<std::int64_t>(*integer) * 10000}};
            }
            if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
                if (!execute_) {
                    return Value{Currency{}};
                }
                return Value{Currency{static_cast<std::int64_t>(*short_integer) * 10000}};
            }
            if (const auto* currency = std::get_if<Currency>(&arguments[0])) {
                return Value{execute_ ? *currency : Currency{}};
            }
            if (std::holds_alternative<Empty>(arguments[0])) {
                return Value{execute_ ? Currency{} : Currency{}};
            }
            double value{};
            if (const auto* number = std::get_if<double>(&arguments[0])) {
                value = *number;
            } else if (const auto* single = std::get_if<float>(&arguments[0])) {
                value = static_cast<double>(*single);
            } else if (std::holds_alternative<Decimal>(arguments[0])) {
                value = as_double(arguments[0]);
            } else if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
                value = *boolean ? -1.0 : 0.0;
            } else {
                if (!execute_) {
                    return Value{Currency{}};
                }
                const auto parsed =
                    parse_numeric_string(std::get<std::string>(arguments[0]));
                if (parsed.status == NumericStringStatus::out_of_range) {
                    set_error("WFC0009", "numeric overflow", identifier_offset);
                    return std::nullopt;
                }
                if (parsed.status != NumericStringStatus::valid) {
                    set_error("WFC0103", "CCur requires a numeric value", identifier_offset);
                    return std::nullopt;
                }
                value = parsed.value;
            }
            if (!execute_) {
                return Value{Currency{}};
            }
            const auto scaled = currency_from_double(value);
            if (!scaled.has_value()) {
                set_error("WFC0009", "numeric overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{Currency{*scaled}};
        }

        if (is_cint) {
            // CInt returns a genuine Int16 (VB6 Integer), not a Long narrowed
            // to the Integer range but still typed Long.
            if (std::holds_alternative<Null>(arguments[0])) {
                set_error("WFC0104", "Invalid use of Null", identifier_offset);
                return std::nullopt;
            }
            if (is_object_reference(arguments[0]) ||
                std::holds_alternative<ArrayValue>(arguments[0])) {
                set_error("WFC0088", "CInt requires a numeric value", identifier_offset);
                return std::nullopt;
            }
            if (std::holds_alternative<Empty>(arguments[0])) {
                return Value{execute_ ? Int16{0} : Int16{}};
            }
            if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
                return Value{execute_ ? *short_integer : Int16{}};
            }
            if (const auto* number = std::get_if<Integer>(&arguments[0])) {
                if (!execute_) {
                    return Value{Int16{}};
                }
                if (*number < std::numeric_limits<Int16>::min() ||
                    *number > std::numeric_limits<Int16>::max()) {
                    set_error("WFC0009", "integer overflow", identifier_offset);
                    return std::nullopt;
                }
                return Value{static_cast<Int16>(*number)};
            }
            if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
                return Value{execute_ && *boolean ? Int16{-1} : Int16{0}};
            }
            if (const auto* number = std::get_if<double>(&arguments[0])) {
                if (!execute_) {
                    return Value{Int16{}};
                }
                return round_double_to_short_integer(*number, identifier_offset);
            }
            if (const auto* single = std::get_if<float>(&arguments[0])) {
                if (!execute_) {
                    return Value{Int16{}};
                }
                return round_double_to_short_integer(
                    static_cast<double>(*single), identifier_offset);
            }
            if (std::holds_alternative<Currency>(arguments[0]) ||
                std::holds_alternative<Decimal>(arguments[0])) {
                if (!execute_) {
                    return Value{Int16{}};
                }
                return round_double_to_short_integer(
                    as_double(arguments[0]), identifier_offset);
            }
            if (!execute_) {
                return Value{Int16{}};
            }

            const auto parsed = parse_numeric_string(std::get<std::string>(arguments[0]));
            if (parsed.status == NumericStringStatus::out_of_range) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            if (parsed.status != NumericStringStatus::valid) {
                set_error("WFC0088", "CInt requires a numeric value", identifier_offset);
                return std::nullopt;
            }
            return round_double_to_short_integer(parsed.value, identifier_offset);
        }

        if (is_clng) {
            if (std::holds_alternative<Null>(arguments[0])) {
                set_error("WFC0104", "Invalid use of Null", identifier_offset);
                return std::nullopt;
            }
            if (is_object_reference(arguments[0]) ||
                std::holds_alternative<ArrayValue>(arguments[0])) {
                set_error("WFC0086", "CLng requires a numeric value", identifier_offset);
                return std::nullopt;
            }
            if (std::holds_alternative<Empty>(arguments[0])) {
                return Value{execute_ ? Integer{0} : Integer{}};
            }
            if (const auto* number = std::get_if<Integer>(&arguments[0])) {
                return Value{execute_ ? *number : Integer{}};
            }
            if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
                return Value{execute_ ? static_cast<Integer>(*short_integer) : Integer{}};
            }
            if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
                return Value{execute_ && *boolean ? Integer{-1} : Integer{0}};
            }
            if (const auto* number = std::get_if<double>(&arguments[0])) {
                if (!execute_) {
                    return Value{Integer{}};
                }
                return round_double_to_long(
                    *number,
                    std::numeric_limits<Integer>::min(),
                    std::numeric_limits<Integer>::max(),
                    identifier_offset);
            }
            if (const auto* single = std::get_if<float>(&arguments[0])) {
                if (!execute_) {
                    return Value{Integer{}};
                }
                return round_double_to_long(
                    static_cast<double>(*single),
                    std::numeric_limits<Integer>::min(),
                    std::numeric_limits<Integer>::max(),
                    identifier_offset);
            }
            if (std::holds_alternative<Currency>(arguments[0]) ||
                std::holds_alternative<Decimal>(arguments[0])) {
                if (!execute_) {
                    return Value{Integer{}};
                }
                return round_double_to_long(
                    as_double(arguments[0]),
                    std::numeric_limits<Integer>::min(),
                    std::numeric_limits<Integer>::max(),
                    identifier_offset);
            }
            if (!execute_) {
                return Value{Integer{}};
            }

            const auto parsed = parse_numeric_string(std::get<std::string>(arguments[0]));
            if (parsed.status == NumericStringStatus::out_of_range) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            if (parsed.status != NumericStringStatus::valid) {
                set_error("WFC0086", "CLng requires a numeric value", identifier_offset);
                return std::nullopt;
            }
            return round_double_to_long(parsed.value, std::numeric_limits<Integer>::min(),
                std::numeric_limits<Integer>::max(), identifier_offset);
        }

        if (is_cbool) {
            // Verified against the local VB6 6.00.8176 reference:
            // CBool(Null) raises "Invalid use of Null"; CBool(Empty) = False.
            if (std::holds_alternative<Null>(arguments[0])) {
                set_error("WFC0104", "Invalid use of Null", identifier_offset);
                return std::nullopt;
            }
            if (is_object_reference(arguments[0]) ||
                std::holds_alternative<ArrayValue>(arguments[0])) {
                set_error("WFC0087", "CBool requires a Boolean or numeric value", identifier_offset);
                return std::nullopt;
            }
            if (std::holds_alternative<Empty>(arguments[0])) {
                return Value{false};
            }
            if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
                return Value{execute_ && *boolean};
            }
            if (const auto* number = std::get_if<Integer>(&arguments[0])) {
                return Value{execute_ && *number != 0};
            }
            if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
                return Value{execute_ && *short_integer != 0};
            }
            if (const auto* number = std::get_if<double>(&arguments[0])) {
                return Value{execute_ && *number != 0.0};
            }
            if (const auto* single = std::get_if<float>(&arguments[0])) {
                return Value{execute_ && *single != 0.0f};
            }
            if (const auto* currency = std::get_if<Currency>(&arguments[0])) {
                return Value{execute_ && currency->scaled != 0};
            }
            if (const auto* decimal = std::get_if<Decimal>(&arguments[0])) {
                return Value{execute_ && !is_zero_big(decimal->mantissa)};
            }
            if (!execute_) {
                return Value{false};
            }

            const auto& text = std::get<std::string>(arguments[0]);
            std::size_t first{};
            std::size_t last = text.size();
            while (first < last &&
                   (text[first] == ' ' || text[first] == '\t' || text[first] == '\r' ||
                    text[first] == '\n')) {
                ++first;
            }
            while (last > first &&
                   (text[last - 1U] == ' ' || text[last - 1U] == '\t' ||
                    text[last - 1U] == '\r' || text[last - 1U] == '\n')) {
                --last;
            }
            std::string normalized;
            normalized.reserve(last - first);
            for (std::size_t index = first; index < last; ++index) {
                normalized.push_back(ascii_lower(text[index]));
            }
            if (normalized == "true") {
                return Value{true};
            }
            if (normalized == "false") {
                return Value{false};
            }

            const auto parsed = parse_numeric_string(normalized);
            if (parsed.status != NumericStringStatus::valid) {
                set_error("WFC0087", "CBool requires a Boolean or numeric value", identifier_offset);
                return std::nullopt;
            }
            return Value{parsed.value != 0.0};
        }

        if (is_space) {
            const auto* count = std::get_if<Integer>(&arguments[0]);
            if (count == nullptr) {
                set_error("WFC0073", "Space requires a Long argument", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            if (*count < 0) {
                set_error("WFC0075", "function length cannot be negative", identifier_offset);
                return std::nullopt;
            }
            if (*count > 268435456) {
                static_cast<void>(raise_runtime(7, "Out of memory", identifier_offset));
                return std::nullopt;
            }
            return Value{std::string(static_cast<std::size_t>(*count), ' ')};
        }

        if (is_string) {
            const auto* count = std::get_if<Integer>(&arguments[0]);
            const bool fill_is_code = std::holds_alternative<Integer>(arguments[1]);
            const bool fill_is_text = std::holds_alternative<std::string>(arguments[1]);
            if (count == nullptr || (!fill_is_code && !fill_is_text)) {
                set_error(
                    "WFC0073",
                    "String requires a Long count and a Long or String fill",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            if (*count < 0) {
                set_error("WFC0075", "function length cannot be negative", identifier_offset);
                return std::nullopt;
            }
            if (*count > 268435456) {
                static_cast<void>(raise_runtime(7, "Out of memory", identifier_offset));
                return std::nullopt;
            }
            char fill{};
            std::string wide_fill;
            if (fill_is_code) {
                const auto code = std::get<Integer>(arguments[1]);
                if (code < 0 || code > 255) {
                    set_error(
                        "WFC0079",
                        "String fill code must be in the character range",
                        identifier_offset);
                    return std::nullopt;
                }
                if (code > 127) {
                    append_utf8_unit(wide_fill, ansi_to_unicode(static_cast<unsigned>(code)));
                }
                fill = static_cast<char>(code);
            } else {
                const auto& text = std::get<std::string>(arguments[1]);
                if (text.empty()) {
                    set_error(
                        "WFC0080",
                        "String requires a non-empty fill String",
                        identifier_offset);
                    return std::nullopt;
                }
                fill = text.front();
                if (static_cast<unsigned char>(fill) >= 0x80U) {
                    const auto first_unit = to_utf16_units(text).front();
                    append_utf8_unit(wide_fill, first_unit);
                }
            }
            if (!wide_fill.empty()) {
                std::string repeated;
                repeated.reserve(wide_fill.size() * static_cast<std::size_t>(*count));
                for (Integer i = 0; i < *count; ++i) repeated += wide_fill;
                return Value{std::move(repeated)};
            }
            return Value{std::string(static_cast<std::size_t>(*count), fill)};
        }

        if (is_instr) {
            const bool has_start = arguments.size() >= 3U;
            const bool has_compare = arguments.size() == 4U;
            Integer start = 1;
            if (has_start) {
                const auto* start_argument = std::get_if<Integer>(&arguments[0]);
                if (start_argument == nullptr) {
                    set_error("WFC0073", "InStr start requires a Long argument", identifier_offset);
                    return std::nullopt;
                }
                start = *start_argument;
            }
            const std::size_t haystack_index = has_start ? 1U : 0U;
            const auto* haystack = std::get_if<std::string>(&arguments[haystack_index]);
            const auto* needle = std::get_if<std::string>(&arguments[haystack_index + 1U]);
            if (haystack == nullptr || needle == nullptr) {
                set_error("WFC0073", "InStr requires String arguments", identifier_offset);
                return std::nullopt;
            }
            const auto* compare_method =
                has_compare ? std::get_if<Integer>(&arguments[3]) : nullptr;
            if (has_compare && compare_method == nullptr) {
                set_error("WFC0073", "InStr compare requires a Long argument", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{Integer{}};
            }
            bool text_compare = option_compare_text_;
            if (compare_method != nullptr) {
                if (*compare_method < -1 || *compare_method > 1) {
                    set_error("WFC0081", "unsupported comparison method", identifier_offset);
                    return std::nullopt;
                }
                text_compare = *compare_method == -1 ? option_compare_text_
                                                     : *compare_method >= 1;
            }
            if (start < 1) {
                set_error("WFC0076", "InStr start must be positive", identifier_offset);
                return std::nullopt;
            }
            if (!is_ascii_text(*haystack) || !is_ascii_text(*needle)) {
                auto hay_units = to_utf16_units(*haystack);
                auto needle_units = to_utf16_units(*needle);
                if (text_compare) {
                    for (auto& unit : hay_units) unit = unit_to_lower(unit);
                    for (auto& unit : needle_units) unit = unit_to_lower(unit);
                }
                const auto unit_begin = static_cast<std::size_t>(start - 1);
                if (unit_begin > hay_units.size()) {
                    return Value{Integer{0}};
                }
                if (needle_units.empty()) {
                    return Value{unit_begin < hay_units.size() ? static_cast<Integer>(start)
                                                               : Integer{0}};
                }
                const auto unit_found = hay_units.find(needle_units, unit_begin);
                return Value{unit_found == std::u16string::npos
                                 ? Integer{0}
                                 : static_cast<Integer>(unit_found + 1U)};
            }
            const auto begin = static_cast<std::size_t>(start - 1);
            if (begin > haystack->size()) {
                return Value{Integer{0}};
            }
            if (needle->empty()) {
                return Value{begin < haystack->size() ? static_cast<Integer>(start) : Integer{0}};
            }
            std::size_t found{};
            if (text_compare) {
                std::string lowered_haystack = *haystack;
                std::string lowered_needle = *needle;
                for (char& character : lowered_haystack) {
                    character = ascii_lower(character);
                }
                for (char& character : lowered_needle) {
                    character = ascii_lower(character);
                }
                found = lowered_haystack.find(lowered_needle, begin);
            } else {
                found = haystack->find(*needle, begin);
            }
            if (found == std::string::npos) {
                return Value{Integer{0}};
            }
            return Value{static_cast<Integer>(found + 1U)};
        }

        if (is_instr_rev) {
            const auto* haystack = std::get_if<std::string>(&arguments[0]);
            const auto* needle = std::get_if<std::string>(&arguments[1]);
            const bool has_start = arguments.size() >= 3U;
            const bool has_compare = arguments.size() == 4U;
            const auto* start_argument =
                has_start ? std::get_if<Integer>(&arguments[2]) : nullptr;
            const auto* compare_method =
                has_compare ? std::get_if<Integer>(&arguments[3]) : nullptr;
            if (haystack == nullptr || needle == nullptr) {
                set_error("WFC0073", "InStrRev requires String arguments", identifier_offset);
                return std::nullopt;
            }
            if ((has_start && start_argument == nullptr) ||
                (has_compare && compare_method == nullptr)) {
                set_error(
                    "WFC0073",
                    "InStrRev start and compare require Long arguments",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{Integer{}};
            }

            const Integer start = start_argument == nullptr ? -1 : *start_argument;
            if (start < -1 || start == 0) {
                set_error("WFC0083", "InStrRev start must be -1 or positive", identifier_offset);
                return std::nullopt;
            }
            bool text_compare = option_compare_text_;
            if (compare_method != nullptr) {
                if (*compare_method < -1 || *compare_method > 1) {
                    set_error("WFC0081", "unsupported comparison method", identifier_offset);
                    return std::nullopt;
                }
                text_compare = *compare_method == -1 ? option_compare_text_
                                                     : *compare_method >= 1;
            }
            if (!is_ascii_text(*haystack) || !is_ascii_text(*needle)) {
                auto hay_units = to_utf16_units(*haystack);
                auto needle_units = to_utf16_units(*needle);
                if (text_compare) {
                    for (auto& unit : hay_units) unit = unit_to_lower(unit);
                    for (auto& unit : needle_units) unit = unit_to_lower(unit);
                }
                if (hay_units.empty()) {
                    return Value{Integer{0}};
                }
                const auto unit_start =
                    start == -1 ? hay_units.size() : static_cast<std::size_t>(start);
                if (unit_start > hay_units.size()) {
                    return Value{Integer{0}};
                }
                if (needle_units.empty()) {
                    return Value{static_cast<Integer>(unit_start)};
                }
                if (needle_units.size() > unit_start) {
                    return Value{Integer{0}};
                }
                const auto unit_found = hay_units.rfind(needle_units, unit_start - needle_units.size());
                return Value{unit_found == std::u16string::npos
                                 ? Integer{0}
                                 : static_cast<Integer>(unit_found + 1U)};
            }
            if (haystack->empty()) {
                return Value{Integer{0}};
            }

            const auto effective_start =
                start == -1 ? haystack->size() : static_cast<std::size_t>(start);
            if (effective_start > haystack->size()) {
                return Value{Integer{0}};
            }
            if (needle->empty()) {
                return Value{static_cast<Integer>(effective_start)};
            }
            if (needle->size() > effective_start) {
                return Value{Integer{0}};
            }

            std::string searchable = *haystack;
            std::string sought = *needle;
            if (text_compare) {
                for (char& character : searchable) {
                    character = ascii_lower(character);
                }
                for (char& character : sought) {
                    character = ascii_lower(character);
                }
            }
            const auto latest_start = effective_start - sought.size();
            const auto found = searchable.rfind(sought, latest_start);
            return Value{
                found == std::string::npos ? Integer{0} : static_cast<Integer>(found + 1U)};
        }

        if (is_strcomp) {
            const auto* left = std::get_if<std::string>(&arguments[0]);
            const auto* right = std::get_if<std::string>(&arguments[1]);
            if (left == nullptr || right == nullptr) {
                set_error("WFC0073", "StrComp requires String arguments", identifier_offset);
                return std::nullopt;
            }
            const bool has_compare = arguments.size() == 3U;
            const auto* compare_method =
                has_compare ? std::get_if<Integer>(&arguments[2]) : nullptr;
            if (has_compare && compare_method == nullptr) {
                set_error("WFC0073", "StrComp compare requires a Long argument", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{Integer{}};
            }
            bool text_compare = option_compare_text_;
            if (compare_method != nullptr) {
                if (*compare_method < -1 || *compare_method > 1) {
                    set_error("WFC0081", "unsupported comparison method", identifier_offset);
                    return std::nullopt;
                }
                text_compare = *compare_method == -1 ? option_compare_text_
                                                     : *compare_method >= 1;
            }
            int comparison{};
            if (text_compare) {
                const std::string lowered_left = fold_case(*left);
                const std::string lowered_right = fold_case(*right);
                comparison = lowered_left.compare(lowered_right);
            } else {
                comparison = left->compare(*right);
            }
            return Value{static_cast<Integer>((comparison > 0) - (comparison < 0))};
        }

        if (is_replace) {
            const auto* expression = std::get_if<std::string>(&arguments[0]);
            const auto* find = std::get_if<std::string>(&arguments[1]);
            const auto* replacement = std::get_if<std::string>(&arguments[2]);
            if (expression == nullptr || find == nullptr || replacement == nullptr) {
                set_error("WFC0073", "Replace requires String arguments", identifier_offset);
                return std::nullopt;
            }
            const bool has_start = arguments.size() >= 4U;
            const bool has_count = arguments.size() >= 5U;
            const bool has_compare = arguments.size() == 6U;
            const auto* start_argument =
                has_start ? std::get_if<Integer>(&arguments[3]) : nullptr;
            const auto* count_argument =
                has_count ? std::get_if<Integer>(&arguments[4]) : nullptr;
            const auto* compare_method =
                has_compare ? std::get_if<Integer>(&arguments[5]) : nullptr;
            if ((has_start && start_argument == nullptr) ||
                (has_count && count_argument == nullptr) ||
                (has_compare && compare_method == nullptr)) {
                set_error(
                    "WFC0073",
                    "Replace start, count, and compare require Long arguments",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            const Integer start = start_argument == nullptr ? 1 : *start_argument;
            const Integer count = count_argument == nullptr ? -1 : *count_argument;
            if (start < 1) {
                set_error("WFC0076", "Replace start must be positive", identifier_offset);
                return std::nullopt;
            }
            if (count < -1) {
                set_error("WFC0082", "Replace count must be -1 or non-negative", identifier_offset);
                return std::nullopt;
            }
            bool text_compare = option_compare_text_;
            if (compare_method != nullptr) {
                if (*compare_method < -1 || *compare_method > 1) {
                    set_error("WFC0081", "unsupported comparison method", identifier_offset);
                    return std::nullopt;
                }
                text_compare = *compare_method == -1 ? option_compare_text_
                                                     : *compare_method >= 1;
            }

            const auto begin = static_cast<std::size_t>(start - 1);
            if (begin >= expression->size()) {
                return Value{std::string{}};
            }
            const std::string source = expression->substr(begin);
            if (find->empty() || count == 0) {
                return Value{source};
            }
            std::string haystack = text_compare ? fold_case(source) : source;
            std::string needle = text_compare ? fold_case(*find) : *find;
            std::string result;
            std::size_t position{};
            Integer replacements{};
            while (true) {
                const std::size_t found = haystack.find(needle, position);
                if (found == std::string::npos || (count >= 0 && replacements >= count)) {
                    result.append(source, position, std::string::npos);
                    break;
                }
                result.append(source, position, found - position);
                result.append(*replacement);
                position = found + needle.size();
                ++replacements;
            }
            return Value{std::move(result)};
        }

        if (is_str) {
            if (!is_number(arguments[0])) {
                set_error("WFC0073", "Str requires a numeric argument", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            std::string digits = render(arguments[0]);
            if (as_double(arguments[0]) == 0.0) {
                digits = "0";
            }
            if (as_double(arguments[0]) >= 0.0) {
                digits.insert(digits.begin(), ' ');
            }
            return Value{std::move(digits)};
        }

        if (is_hex || is_oct) {
            if (!is_number(arguments[0]) &&
                !std::holds_alternative<std::string>(arguments[0])) {
                set_error(
                    "WFC0073",
                    is_hex ? "Hex requires a numeric argument" : "Oct requires a numeric argument",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            std::optional<Integer> number;
            if (const auto* string = std::get_if<std::string>(&arguments[0])) {
                const auto parsed = parse_numeric_string(*string);
                if (parsed.status == NumericStringStatus::out_of_range) {
                    set_error("WFC0009", "integer overflow", identifier_offset);
                    return std::nullopt;
                }
                if (parsed.status != NumericStringStatus::valid) {
                    set_error(
                        "WFC0099",
                        is_hex ? "Hex requires a numeric value" : "Oct requires a numeric value",
                        identifier_offset);
                    return std::nullopt;
                }
                const auto rounded = round_double_to_long(
                    parsed.value,
                    std::numeric_limits<Integer>::min(),
                    std::numeric_limits<Integer>::max(),
                    identifier_offset);
                if (!rounded.has_value()) {
                    return std::nullopt;
                }
                number = std::get<Integer>(*rounded);
            } else {
                number = coerce_long(arguments[0], identifier_offset);
            }
            if (!number.has_value()) {
                return std::nullopt;
            }
            auto magnitude = static_cast<std::uint32_t>(*number);
            if (std::holds_alternative<Int16>(arguments[0])) {
                magnitude &= 0xFFFFU;  // an Integer prints as 16 bits
            }
            if (magnitude == 0U) {
                return Value{std::string{"0"}};
            }
            const std::uint32_t radix = is_hex ? 16U : 8U;
            std::string digits;
            while (magnitude != 0U) {
                const auto value = static_cast<int>(magnitude % radix);
                digits.push_back(
                    value < 10 ? static_cast<char>('0' + value)
                               : static_cast<char>('A' + (value - 10)));
                magnitude /= radix;
            }
            std::reverse(digits.begin(), digits.end());
            return Value{std::move(digits)};
        }

        if (execute_ && !arguments.empty() && std::holds_alternative<Null>(arguments[0])) {
            // The Variant-returning string functions propagate Null; their `$`
            // forms reject it (error 94).
            static const std::set<std::string, std::less<>> null_propagating = {
                "trim", "ltrim", "rtrim", "ucase", "lcase", "left", "right", "mid", "strreverse",
                "space", "string", "chr", "chrw"};
            static const std::set<std::string, std::less<>> null_rejecting = {
                "trim$", "ltrim$", "rtrim$", "ucase$", "lcase$", "left$", "right$", "mid$",
                "space$", "string$", "chr$", "chrw$"};
            const std::string key(identifier);
            if (null_propagating.contains(key)) {
                return Value{Null{}};
            }
            if (null_rejecting.contains(key)) {
                set_error("WFC0104", "Invalid use of Null", identifier_offset);
                return std::nullopt;
            }
        }
        if (is_len && execute_ && std::holds_alternative<Empty>(arguments[0])) {
            return Value{Integer{0}};
        }
        if (is_len && execute_ && std::holds_alternative<Null>(arguments[0])) {
            return Value{Null{}};
        }
        if (is_len && execute_ && !std::holds_alternative<std::string>(arguments[0])) {
            if (const auto* instance = std::get_if<ObjectInstance>(&arguments[0])) {
                if (is_udt_class(instance->data->class_name)) {
                    return Value{static_cast<Integer>(udt_byte_size(arguments[0]))};
                }
            }
            if (is_number(arguments[0]) || std::holds_alternative<bool>(arguments[0]) ||
                std::holds_alternative<DateValue>(arguments[0])) {
                return Value{static_cast<Integer>(render(arguments[0]).size())};
            }
        }
        if (is_strconv && execute_ && std::holds_alternative<ArrayValue>(arguments[0])) {
            const auto& bytes = std::get<ArrayValue>(arguments[0]);
            const auto* mode = std::get_if<Integer>(&arguments[1]);
            if (mode != nullptr && *mode == 64 &&
                bytes.element_type_index == Value{Byte{}}.index()) {  // vbUnicode
                std::string text;
                for (const auto& element : bytes.elements) {
                    if (const auto* byte = std::get_if<Byte>(&element)) {
                        append_utf8_unit(text, ansi_to_unicode(*byte));
                    }
                }
                return Value{std::move(text)};
            }
        }
        const auto* string = std::get_if<std::string>(&arguments[0]);
        static const std::string dry_run_string;
        if (string == nullptr && !execute_) {
            // A placeholder from a not-taken branch (a member of `Nothing`).
            string = &dry_run_string;
        }
        if (string == nullptr) {
            set_error("WFC0073", "function requires a String argument", identifier_offset);
            return std::nullopt;
        }
        if (is_macid) {
            if (!execute_) {
                return Value{Integer{}};
            }
            if (string->size() != 4U) {
                set_error("WFC0100", "MacID requires exactly four bytes", identifier_offset);
                return std::nullopt;
            }
            std::uint32_t packed{};
            for (const unsigned char byte : *string) {
                packed = (packed << 8U) | byte;
            }
            const std::int64_t signed_value = packed >= 0x80000000U
                                                  ? static_cast<std::int64_t>(packed) - 0x100000000LL
                                                  : static_cast<std::int64_t>(packed);
            return Value{static_cast<Integer>(signed_value)};
        }
        if (is_len) {
            if (!execute_) {
                return Value{Integer{}};
            }
            if (string->size() > static_cast<std::size_t>(std::numeric_limits<Integer>::max())) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            // LenB reports stored bytes (REQ-0177); Len counts UTF-16 units.
            return Value{static_cast<Integer>(
                identifier == "lenb" ? string->size() : utf16_length(*string))};
        }

        if (is_asc) {
            if (!execute_) {
                return Value{Integer{}};
            }
            if (string->empty()) {
                set_error("WFC0077", "Asc requires a non-empty String", identifier_offset);
                return std::nullopt;
            }
            const auto lead = static_cast<unsigned char>(string->front());
            if (lead < 0x80U) {
                return Value{static_cast<Integer>(lead)};
            }
            const auto units = to_utf16_units(*string);
            const unsigned first_unit = units.front();
            if (identifier == "ascw") {
                // AscW is a signed 16-bit value: units above 32767 are negative.
                return Value{static_cast<Integer>(
                    first_unit > 0x7FFFU ? static_cast<int>(first_unit) - 0x10000
                                         : static_cast<int>(first_unit))};
            }
            return Value{static_cast<Integer>(unicode_to_ansi(first_unit))};
        }

        if (is_strconv) {
            const auto* conversion = std::get_if<Integer>(&arguments[1]);
            if (conversion == nullptr) {
                set_error(
                    "WFC0073",
                    "StrConv requires a Long conversion argument",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            if (*conversion == 1 || *conversion == 2 || *conversion == 3) {
                // vbUpperCase, vbLowerCase, vbProperCase (per UTF-16 unit).
                auto units = to_utf16_units(*string);
                const auto is_letter = [](const char16_t c) {
                    return unit_to_lower(c) != c || unit_to_upper(c) != c;
                };
                bool word_start = true;
                for (auto& unit : units) {
                    if (*conversion == 1) {
                        unit = unit_to_upper(unit);
                    } else if (*conversion == 2) {
                        unit = unit_to_lower(unit);
                    } else if (is_letter(unit)) {
                        unit = word_start ? unit_to_upper(unit) : unit_to_lower(unit);
                        word_start = false;
                    } else {
                        word_start = true;
                    }
                }
                return Value{from_utf16_units(units)};
            }
            if (*conversion == 128) {  // vbFromUnicode: the string's ANSI bytes
                ArrayValue bytes{};
                bytes.is_dynamic = true;
                bytes.element_type_index = Value{Byte{}}.index();
                for (const char16_t unit : to_utf16_units(*string)) {
                    bytes.elements.emplace_back(static_cast<Byte>(unicode_to_ansi(unit)));
                }
                return Value{std::move(bytes)};
            }
            if (*conversion == 64) {  // vbUnicode on a String: unchanged
                return Value{*string};
            }
            set_error(
                "WFC0093",
                "StrConv conversion is not supported in the current model",
                identifier_offset);
            return std::nullopt;
        }

        if (is_val) {
            if (!execute_) {
                return Value{Integer{}};
            }
            std::string compact;
            compact.reserve(string->size());
            for (const char character : *string) {
                if (character != ' ' && character != '\t' && character != '\r' &&
                    character != '\n') {
                    compact.push_back(character);
                }
            }
            if (compact.empty()) {
                return Value{Integer{0}};
            }

            std::size_t digit_start{};
            if (compact[0] == '+' || compact[0] == '-') {
                digit_start = 1U;
            }
            if (digit_start + 1U < compact.size() && compact[digit_start] == '&' &&
                (compact[digit_start + 1U] == 'h' || compact[digit_start + 1U] == 'H' ||
                 compact[digit_start + 1U] == 'o' || compact[digit_start + 1U] == 'O')) {
                const int base = compact[digit_start + 1U] == 'h' ||
                                         compact[digit_start + 1U] == 'H'
                                     ? 16
                                     : 8;
                const std::size_t radix_start = digit_start + 2U;
                std::size_t radix_end = radix_start;
                const auto is_radix_digit = [base](const char character) {
                    if (character >= '0' && character <= '7') {
                        return true;
                    }
                    return base == 16 &&
                           ((character >= '8' && character <= '9') ||
                            (character >= 'a' && character <= 'f') ||
                            (character >= 'A' && character <= 'F'));
                };
                while (radix_end < compact.size() && is_radix_digit(compact[radix_end])) {
                    ++radix_end;
                }
                if (radix_end == radix_start) {
                    return Value{Integer{0}};
                }

                std::uint32_t magnitude{};
                const auto conversion = std::from_chars(
                    compact.data() + radix_start,
                    compact.data() + radix_end,
                    magnitude,
                    base);
                if (conversion.ec == std::errc::result_out_of_range) {
                    set_error("WFC0009", "integer overflow", identifier_offset);
                    return std::nullopt;
                }

                std::int64_t value = magnitude;
                if (magnitude >= 0x8000U && magnitude <= 0xFFFFU) {
                    value -= 0x10000LL;
                } else if (magnitude >= 0x80000000U) {
                    value -= 0x100000000LL;
                }
                if (digit_start == 1U && compact[0] == '-') {
                    value = -value;
                }
                if (value < std::numeric_limits<Integer>::min() ||
                    value > std::numeric_limits<Integer>::max()) {
                    set_error("WFC0009", "integer overflow", identifier_offset);
                    return std::nullopt;
                }
                return Value{static_cast<Integer>(value)};
            }
            const auto is_digit = [](const char character) {
                return std::isdigit(static_cast<unsigned char>(character)) != 0;
            };
            std::size_t pos = digit_start;
            bool had_int_digits = false;
            while (pos < compact.size() && is_digit(compact[pos])) {
                ++pos;
                had_int_digits = true;
            }
            const std::size_t int_end = pos;
            bool is_float = false;
            bool had_frac_digits = false;
            if (pos < compact.size() && compact[pos] == '.') {
                is_float = true;
                ++pos;
                while (pos < compact.size() && is_digit(compact[pos])) {
                    ++pos;
                    had_frac_digits = true;
                }
            }
            if ((had_int_digits || had_frac_digits) && pos < compact.size() &&
                (compact[pos] == 'e' || compact[pos] == 'E')) {
                std::size_t exponent = pos + 1U;
                if (exponent < compact.size() &&
                    (compact[exponent] == '+' || compact[exponent] == '-')) {
                    ++exponent;
                }
                if (exponent < compact.size() && is_digit(compact[exponent])) {
                    is_float = true;
                    pos = exponent;
                    while (pos < compact.size() && is_digit(compact[pos])) {
                        ++pos;
                    }
                }
            }
            if (!had_int_digits && !had_frac_digits) {
                return Value{Integer{0}};
            }

            const std::size_t conversion_start = compact[0] == '+' ? 1U : 0U;
            if (is_float) {
                double value{};
                const auto conversion = std::from_chars(
                    compact.data() + conversion_start,
                    compact.data() + pos,
                    value);
                if (conversion.ec == std::errc::result_out_of_range) {
                    set_error("WFC0009", "numeric overflow", identifier_offset);
                    return std::nullopt;
                }
                return Value{value};
            }
            Integer result{};
            const auto conversion = std::from_chars(
                compact.data() + conversion_start,
                compact.data() + int_end,
                result);
            if (conversion.ec == std::errc::result_out_of_range) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{result};
        }

        if (is_left || is_right) {
            const auto* length = std::get_if<Integer>(&arguments[1]);
            if (length == nullptr) {
                set_error("WFC0073", "function length requires a Long argument", identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            if (*length < 0) {
                set_error("WFC0075", "function length cannot be negative", identifier_offset);
                return std::nullopt;
            }
            const auto requested = static_cast<std::size_t>(*length);
            if (!is_ascii_text(*string)) {
                const auto units = to_utf16_units(*string);
                const auto unit_count = std::min(requested, units.size());
                return Value{from_utf16_units(
                    is_left ? std::u16string_view(units).substr(0U, unit_count)
                            : std::u16string_view(units).substr(units.size() - unit_count))};
            }
            const auto count = std::min(requested, string->size());
            return Value{
                is_left ? string->substr(0U, count) : string->substr(string->size() - count)};
        }

        if (is_mid) {
            const auto* start = std::get_if<Integer>(&arguments[1]);
            const auto* length =
                arguments.size() == 3U ? std::get_if<Integer>(&arguments[2]) : nullptr;
            if (start == nullptr || (arguments.size() == 3U && length == nullptr)) {
                set_error(
                    "WFC0073",
                    "Mid start and length require Long arguments",
                    identifier_offset);
                return std::nullopt;
            }
            if (!execute_) {
                return Value{std::string{}};
            }
            if (*start < 1) {
                set_error("WFC0076", "Mid start must be positive", identifier_offset);
                return std::nullopt;
            }
            if (length != nullptr && *length < 0) {
                set_error("WFC0075", "function length cannot be negative", identifier_offset);
                return std::nullopt;
            }

            const auto first = static_cast<std::size_t>(*start - 1);
            if (!is_ascii_text(*string)) {
                const auto units = to_utf16_units(*string);
                if (first >= units.size()) {
                    return Value{std::string{}};
                }
                const auto unit_available = units.size() - first;
                const auto unit_count =
                    length == nullptr ? unit_available
                                      : std::min(static_cast<std::size_t>(*length), unit_available);
                return Value{from_utf16_units(std::u16string_view(units).substr(first, unit_count))};
            }
            if (first >= string->size()) {
                return Value{std::string{}};
            }
            const auto available = string->size() - first;
            const auto count = length == nullptr
                                   ? available
                                   : std::min(static_cast<std::size_t>(*length), available);
            return Value{string->substr(first, count)};
        }

        if (!execute_) {
            return Value{std::string{}};
        }

        if (is_left_trim || is_right_trim || is_trim) {
            std::size_t first{};
            std::size_t last = string->size();
            if (is_left_trim || is_trim) {
                while (first < last && (*string)[first] == ' ') {
                    ++first;
                }
            }
            if (is_right_trim || is_trim) {
                while (last > first && (*string)[last - 1U] == ' ') {
                    --last;
                }
            }
            return Value{string->substr(first, last - first)};
        }

        if (is_reverse) {
            if (!is_ascii_text(*string)) {
                auto units = to_utf16_units(*string);
                std::reverse(units.begin(), units.end());
                return Value{from_utf16_units(units)};
            }
            std::string result = *string;
            std::reverse(result.begin(), result.end());
            return Value{std::move(result)};
        }

        if (!is_ascii_text(*string)) {
            auto units = to_utf16_units(*string);
            for (auto& unit : units) unit = is_lower ? unit_to_lower(unit) : unit_to_upper(unit);
            return Value{from_utf16_units(units)};
        }
        std::string result = *string;
        for (char& character : result) {
            character = is_lower ? ascii_lower(character) : ascii_upper(character);
        }
        return Value{std::move(result)};
    }

    [[nodiscard]] std::optional<Value> parse_string() {
        advance();
        std::string value;
        while (!at_end() && current() != '\r' && current() != '\n') {
            if (current() != '"') {
                value.push_back(current());
                advance();
                continue;
            }

            advance();
            if (!at_end() && current() == '"') {
                value.push_back('"');
                advance();
                continue;
            }
            return Value{std::move(value)};
        }

        set_error("WFC0003", "unterminated string literal", offset_);
        return std::nullopt;
    }

    // Consume `digits[.digits][(e|E)[+|-]digits]` from the current position,
    // returning true when a fractional or exponent part made it a Double form.
    [[nodiscard]] bool lex_number_span() noexcept {
        const auto is_digit = [](const char character) {
            return std::isdigit(static_cast<unsigned char>(character)) != 0;
        };
        while (!at_end() && is_digit(current())) {
            advance();
        }
        bool is_float = false;
        if (!at_end() && current() == '.' && is_digit(peek(1))) {
            is_float = true;
            advance();
            while (!at_end() && is_digit(current())) {
                advance();
            }
        }
        if (!at_end() && (current() == 'e' || current() == 'E' || current() == 'd' || current() == 'D')) {
            const char sign = peek(1);
            const std::size_t digit_ahead = (sign == '+' || sign == '-') ? 2U : 1U;
            if (is_digit(peek(digit_ahead))) {
                is_float = true;
                advance();
                if (!at_end() && (current() == '+' || current() == '-')) {
                    advance();
                }
                while (!at_end() && is_digit(current())) {
                    advance();
                }
            }
        }
        return is_float;
    }

    [[nodiscard]] std::optional<Value> parse_double(
        const std::size_t start,
        const std::size_t end) {
        double value{};
        std::string text(source_.substr(start, end - start));
        std::replace(text.begin(), text.end(), 'd', 'e');
        std::replace(text.begin(), text.end(), 'D', 'e');
        const auto conversion = std::from_chars(text.data(), text.data() + text.size(), value);
        if (conversion.ec != std::errc{} || conversion.ptr != text.data() + text.size()) {
            set_error("WFC0006", "numeric literal is malformed", start);
            return std::nullopt;
        }
        return Value{value};
    }

    [[nodiscard]] std::optional<Value> parse_single(
        const std::size_t start,
        const std::size_t end) {
        float value{};
        std::string text(source_.substr(start, end - start));
        std::replace(text.begin(), text.end(), 'd', 'e');
        std::replace(text.begin(), text.end(), 'D', 'e');
        const auto conversion = std::from_chars(text.data(), text.data() + text.size(), value);
        if (conversion.ec != std::errc{} || conversion.ptr != text.data() + text.size()) {
            set_error("WFC0006", "numeric literal is malformed", start);
            return std::nullopt;
        }
        return Value{value};
    }

    [[nodiscard]] std::optional<Value> parse_short_integer(
        const std::size_t start,
        const std::size_t end) {
        Int16 value{};
        const auto conversion =
            std::from_chars(source_.data() + start, source_.data() + end, value);
        if (conversion.ec != std::errc{} || conversion.ptr != source_.data() + end) {
            set_error("WFC0006", "Integer literal is out of range", start);
            return std::nullopt;
        }
        return Value{value};
    }

    // Parse a non-negative decimal span into a Currency scaled int64,
    // without an intermediate floating-point conversion, so a value with
    // more significant digits than a double can represent exactly (up to
    // Currency's full 19-digit magnitude) still parses exactly. Currency
    // literals do not support exponent notation.
    [[nodiscard]] std::optional<Value> parse_currency(
        const std::size_t start,
        const std::size_t end) {
        const std::string_view text = source_.substr(start, end - start);
        if (text.find_first_of("eE") != std::string_view::npos) {
            set_error(
                "WFC0006", "Currency literal does not support exponent notation", start);
            return std::nullopt;
        }
        const auto dot = text.find('.');
        const std::string_view integer_part = dot == std::string_view::npos
            ? text
            : text.substr(0, dot);
        const std::string_view fraction_part = dot == std::string_view::npos
            ? std::string_view{}
            : text.substr(dot + 1U);
        if (fraction_part.size() > 4U) {
            set_error(
                "WFC0006",
                "Currency literal supports at most four decimal digits",
                start);
            return std::nullopt;
        }
        std::uint64_t integer_magnitude{};
        if (!integer_part.empty()) {
            const auto conversion = std::from_chars(
                integer_part.data(), integer_part.data() + integer_part.size(),
                integer_magnitude);
            if (conversion.ec != std::errc{}) {
                set_error("WFC0006", "Currency literal is out of range", start);
                return std::nullopt;
            }
        }
        std::uint64_t fraction_magnitude{};
        if (!fraction_part.empty()) {
            const auto conversion = std::from_chars(
                fraction_part.data(), fraction_part.data() + fraction_part.size(),
                fraction_magnitude);
            if (conversion.ec != std::errc{}) {
                set_error("WFC0006", "numeric literal is malformed", start);
                return std::nullopt;
            }
            for (std::size_t pad = fraction_part.size(); pad < 4U; ++pad) {
                fraction_magnitude *= 10ULL;
            }
        }
        constexpr auto maximum =
            static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
        if (integer_magnitude > maximum / 10000ULL) {
            set_error("WFC0006", "Currency literal is out of range", start);
            return std::nullopt;
        }
        const std::uint64_t scaled_integer = integer_magnitude * 10000ULL;
        if (scaled_integer > maximum - fraction_magnitude) {
            set_error("WFC0006", "Currency literal is out of range", start);
            return std::nullopt;
        }
        return Value{Currency{static_cast<std::int64_t>(scaled_integer + fraction_magnitude)}};
    }

    [[nodiscard]] std::optional<Value> parse_number() {
        const auto start = offset_;
        const bool floating_form = lex_number_span();
        const auto end = offset_;
        char suffix = '\0';
        if (!at_end() &&
            (current() == '#' || current() == '&' || current() == '!' || current() == '@' ||
             current() == '%')) {
            suffix = current();
            advance();
        }
        if (suffix == '!') {
            return parse_single(start, end);
        }
        if (suffix == '@') {
            return parse_currency(start, end);
        }
        if (floating_form || suffix == '#') {
            if (suffix == '&') {
                set_error("WFC0006", "Long literal suffix requires an integer", start);
                return std::nullopt;
            }
            if (suffix == '%') {
                set_error("WFC0006", "Integer literal suffix requires an integer", start);
                return std::nullopt;
            }
            return parse_double(start, end);
        }
        if (suffix == '%') {
            return parse_short_integer(start, end);
        }
        Integer value{};
        const auto conversion =
            std::from_chars(source_.data() + start, source_.data() + end, value);
        if (conversion.ec == std::errc::result_out_of_range) {
            if (suffix != '&') {
                return parse_double(start, end);  // beyond Long: a Double literal
            }
            set_error("WFC0006", "integer literal out of range", start);
            return std::nullopt;
        }
        if (suffix == '\0' && integer_literals_are_integer_ && value >= -32768 && value <= 32767) {
            return Value{static_cast<Int16>(value)};  // a small literal is an Integer
        }
        return Value{value};
    }

    [[nodiscard]] std::optional<Value> parse_negative_number() {
        const auto start = offset_;
        const bool floating_form = lex_number_span();
        const auto end = offset_;
        char suffix = '\0';
        if (!at_end() &&
            (current() == '#' || current() == '&' || current() == '!' || current() == '@' ||
             current() == '%')) {
            suffix = current();
            advance();
        }
        if (suffix == '!') {
            auto value = parse_single(start, end);
            if (!value.has_value()) {
                return std::nullopt;
            }
            return Value{-std::get<float>(*value)};
        }
        if (suffix == '@') {
            auto value = parse_currency(start, end);
            if (!value.has_value()) {
                return std::nullopt;
            }
            return Value{Currency{-std::get<Currency>(*value).scaled}};
        }
        if (floating_form || suffix == '#') {
            if (suffix == '&') {
                set_error("WFC0006", "Long literal suffix requires an integer", start);
                return std::nullopt;
            }
            if (suffix == '%') {
                set_error("WFC0006", "Integer literal suffix requires an integer", start);
                return std::nullopt;
            }
            auto value = parse_double(start, end);
            if (!value.has_value()) {
                return std::nullopt;
            }
            return Value{-std::get<double>(*value)};
        }

        if (suffix == '%') {
            std::uint64_t magnitude{};
            const auto conversion =
                std::from_chars(source_.data() + start, source_.data() + end, magnitude);
            constexpr auto maximum_magnitude =
                static_cast<std::uint64_t>(std::numeric_limits<Int16>::max()) + 1U;
            if (conversion.ec == std::errc::result_out_of_range ||
                magnitude > maximum_magnitude) {
                set_error("WFC0006", "Integer literal is out of range", start);
                return std::nullopt;
            }
            if (magnitude == maximum_magnitude) {
                return Value{std::numeric_limits<Int16>::min()};
            }
            return Value{static_cast<Int16>(-static_cast<Int16>(magnitude))};
        }

        std::uint64_t magnitude{};
        const auto conversion =
            std::from_chars(source_.data() + start, source_.data() + end, magnitude);
        constexpr auto maximum_magnitude =
            static_cast<std::uint64_t>(std::numeric_limits<Integer>::max()) + 1U;
        if (conversion.ec == std::errc::result_out_of_range ||
            magnitude > maximum_magnitude) {
            if (suffix != '&') {
                auto wide = parse_double(start, end);
                if (!wide.has_value()) return std::nullopt;
                return Value{-std::get<double>(*wide)};
            }
            set_error("WFC0006", "integer literal out of range", start);
            return std::nullopt;
        }
        if (magnitude == maximum_magnitude) {
            return Value{std::numeric_limits<Integer>::min()};
        }
        if (suffix == '\0' && integer_literals_are_integer_ && magnitude <= 32768U) {
            return Value{static_cast<Int16>(-static_cast<std::int32_t>(magnitude))};
        }
        return Value{static_cast<Integer>(-static_cast<Integer>(magnitude))};
    }

    // Round a Double to the nearest Long using banker's rounding (the default
    // IEEE round-to-nearest-even, matching VB6), rejecting out-of-range values.
    [[nodiscard]] std::optional<Value> round_double_to_long(
        const double number,
        const Integer minimum,
        const Integer maximum,
        const std::size_t offset) {
        const double rounded = std::nearbyint(number);
        if (!(rounded >= static_cast<double>(minimum) &&
              rounded <= static_cast<double>(maximum))) {
            set_error("WFC0009", "integer overflow", offset);
            return std::nullopt;
        }
        return Value{static_cast<Integer>(rounded)};
    }

    // Round a Double to the nearest Int16 (VB6 Integer) using banker's
    // rounding, rejecting out-of-range values.
    [[nodiscard]] std::optional<Value> round_double_to_short_integer(
        const double number,
        const std::size_t offset) {
        const double rounded = std::nearbyint(number);
        if (!(rounded >= static_cast<double>(std::numeric_limits<Int16>::min()) &&
              rounded <= static_cast<double>(std::numeric_limits<Int16>::max()))) {
            set_error("WFC0009", "integer overflow", offset);
            return std::nullopt;
        }
        return Value{static_cast<Int16>(rounded)};
    }

    // Render a Currency's exact scaled value as decimal digits: the whole
    // part, then a '.' and up to four fraction digits with trailing zeros
    // trimmed (an exact whole amount has no decimal point at all), matching
    // the shortest-round-tripping-form convention this evaluator already
    // uses for Double/Single rendering. No leading space (see render_str_
    // style callers, which add one for Str's sign-space convention).
    [[nodiscard]] static std::string render_currency(const std::int64_t scaled) {
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

    // Fixed-point text of a non-negative finite number with `places` decimals, rounded
    // half-up from its 15-significant-digit decimal form (how VB's Format rounds: 2.5 -> "3",
    // 0.285 -> "0.29"), instead of from the exact binary value.
    [[nodiscard]] static std::string fixed_half_up(const double magnitude, const int places) {
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

    // Render a finite double using VBA's "Fixed"/"Standard" Format styles:
    // exactly two decimal digits, an optional grouped integer part, and a
    // leading '-' only when the rounded magnitude is nonzero.
    [[nodiscard]] static std::string render_fixed_style(
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

    // Render a finite double using VBA's "Scientific" Format style: one
    // mantissa digit, two fraction digits, an uppercase 'E', an explicit
    // exponent sign, and a minimum two-digit exponent.
    [[nodiscard]] static std::string render_scientific_style(const double value) {
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

    // A custom numeric picture section, after expanding every `\`-escaped
    // pair (REQ-0221) and every `"`-quoted run (REQ-0222): `text` drops
    // the backslashes and the quote delimiters themselves, and
    // `forced_literal[i]` is true exactly when `text[i]` came from either
    // one rather than appearing bare -- so it must be rendered as a plain
    // literal character at its position even when it is one of this
    // format's own special characters, never interpreted as a digit
    // placeholder, decimal point, grouping comma, or section separator. A
    // trailing lone `\` (nothing left to escape) is kept as a literal
    // backslash character instead, and an unterminated `"..."` run (no
    // closing quote before the section ends) makes the rest of the
    // section literal -- both disclosed simplifications, since real VB6
    // pictures do not normally end mid-escape or mid-quote.
    struct EscapedPicture {
        std::string text;
        std::vector<bool> forced_literal;
    };

    [[nodiscard]] static EscapedPicture parse_picture_escapes(const std::string& section) {
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

    // Splits a custom numeric picture `Format` `Style` on `;` into its
    // positive/negative/zero sections (REQ-0220) and renders `value`
    // through whichever section real VB6 selects for it, delegating the
    // actual character-by-character rendering of that one section to
    // `render_custom_numeric_picture_section`. An escaped `\;` (REQ-0221)
    // or a `;` inside a `"..."` quoted run (REQ-0222) is not treated as a
    // section separator -- checked textually here, ahead of
    // `render_custom_numeric_picture_section`'s own escape/quote
    // expansion, since a section boundary has to be decided before any
    // one section's own text is otherwise interpreted. A quoted run left
    // open at the end of a section (no closing `"` before the next `;`,
    // or the end of the whole picture) swallows that `;` too, matching
    // `parse_picture_escapes`'s own "unterminated quote" simplification.
    //
    // One section (no `;` at all) applies to every value unchanged
    // (REQ-0218's original behavior, including its automatic leading `-`
    // for a negative value). Two sections split positive-or-zero (the
    // first) from negative (the second); three add a dedicated zero
    // section (the third), selected whenever `value` is exactly `0.0`,
    // ahead of the positive/negative check. A fourth (text) section, for
    // applying `Format` to a `String` expression, is out of scope --
    // `REQ-0193`'s Scope already excludes a `String` expression from every
    // named/custom style.
    //
    // A negative value rendered through the dedicated negative section
    // uses its own magnitude (`std::fabs`) with no automatic leading `-`:
    // passing a non-negative number into the single-section helper below
    // means its own "negative ⇒ leading '-'" check never fires, so any
    // sign in the output must come from the negative section's own
    // literal characters -- matching real VB6, where the negative section
    // is responsible for its own sign.
    [[nodiscard]] static std::string render_custom_numeric_picture(
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

    // Renders one section of a VBA "custom numeric picture" `Format`
    // `Style` (REQ-0218; multi-section dispatch is `REQ-0220`, above; `\`
    // escapes are `REQ-0221`, quoted text `REQ-0222`, and an unescaped
    // `%`'s x100 scaling `REQ-0223`, below): any `Style` section that
    // does not name one of the reserved named styles above is treated
    // this way, character by character. `0` is a digit placeholder that
    // forces a `0` when no digit remains at that position; `#` is a digit
    // placeholder that shows nothing when no digit remains; `.` marks the
    // single decimal point, splitting the picture into an integer and a
    // fraction section; a `,` among the integer section's digit
    // placeholders (and nowhere else in that section) enables
    // comma-grouped thousands separators; every other character is a
    // literal, copied through unchanged at its position -- unless it was
    // itself `\`-escaped, in which case it is always a literal
    // (`escaped.forced_literal`, from `parse_picture_escapes`), even if it
    // would otherwise be one of `0`/`#`/`.`/`,` above. The fraction
    // section is rounded to its own placeholder count using the same
    // nearest-even rounding `to_chars`'s fixed format already gives
    // `Fixed`/`Standard`, and a trailing run of `#`-placeholder digits
    // that rounded to `0` is trimmed. A negative `value` gets a leading
    // `-` unless the entire rendered magnitude is zero, matching
    // `Fixed`/`Standard`'s own convention (a multi-section picture's
    // negative section is only ever called with a non-negative magnitude
    // by the dispatcher above, so this never fires for it). A second
    // *unescaped* literal `.` (if any) is treated as an ordinary literal
    // character within the fraction section, not a second decimal point.
    [[nodiscard]] static std::string render_custom_numeric_picture_section(
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

    // Advance the verified VB6-reference Rnd generator by one 24-bit linear
    // congruential step: state' = (state * 0x43FD43FD + 0xC39EC3) mod 2^24.
    // Confirmed byte-for-byte against a local VB6 6.00.8176 probe.
    [[nodiscard]] static std::uint32_t rnd_step(const std::uint32_t state) noexcept {
        constexpr std::uint32_t multiplier = 0x43FD43FDU;
        constexpr std::uint32_t increment = 0x00C39EC3U;
        constexpr std::uint32_t modulus_mask = 0x00FFFFFFU;  // 2^24 - 1
        return static_cast<std::uint32_t>(
            (static_cast<std::uint64_t>(state) * multiplier + increment) & modulus_mask);
    }

    [[nodiscard]] static double rnd_value(const std::uint32_t state) noexcept {
        return static_cast<double>(state) / 16777216.0;  // state / 2^24
    }

    // WFC-owned deterministic seed hash used by Randomize(number) and
    // Rnd(negative). The reference VB6 runtime's own per-seed sequence is not
    // reproducible even for an explicit Randomize argument (confirmed by a
    // local probe: repeated Randomize calls with the same argument produced
    // different subsequent Rnd() results), so no formula could truthfully
    // claim to match it. This hash instead guarantees a WFC-specific
    // contract: the same seed always produces the same subsequent sequence.
    [[nodiscard]] static std::uint32_t seed_from_number(const double value) noexcept {
        const auto bits = std::bit_cast<std::uint64_t>(value);
        auto folded = static_cast<std::uint32_t>(bits ^ (bits >> 32U));
        folded ^= folded >> 16U;
        return folded & 0x00FFFFFFU;
    }

    // Non-deterministic seed source for argument-less Randomize, matching
    // VB6's documented system-timer-based reseeding.
    [[nodiscard]] static double entropy_seed() {
        return static_cast<double>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }

    [[nodiscard]] const Integer* require_integer(
        const Value& value,
        const std::size_t operator_offset) {
        const auto* integer = std::get_if<Integer>(&value);
        if (integer == nullptr) {
            set_error("WFC0007", "operator requires integer operands", operator_offset);
        }
        return integer;
    }

    [[nodiscard]] const bool* require_boolean(
        const Value& value,
        const std::size_t operator_offset) {
        const auto* boolean = std::get_if<bool>(&value);
        if (boolean == nullptr) {
            set_error("WFC0019", "logical operator requires Boolean operands", operator_offset);
        }
        return boolean;
    }


    // A ternary logical operand: Null and Empty are valid inputs everywhere
    // a Boolean is otherwise required for And/Or/Not/Xor/Eqv/Imp. Null
    // coerces to the ternary "unknown" state (nullopt); Empty coerces to
    // False, matching the verified CBool(Empty) = False behavior. Returns
    // nullopt with an error already set only for a genuinely wrong type.
    struct TernaryOperand {
        bool is_null{};
        bool value{};
    };

    [[nodiscard]] std::optional<TernaryOperand> coerce_ternary_operand(
        const Value& value,
        const std::size_t operator_offset) {
        if (std::holds_alternative<Null>(value)) {
            return TernaryOperand{true, false};
        }
        if (std::holds_alternative<Empty>(value)) {
            return TernaryOperand{false, false};
        }
        if (!execute_ && !std::holds_alternative<bool>(value)) {
            return TernaryOperand{false, false};  // placeholder operand of a not-taken branch
        }
        const auto* boolean = require_boolean(value, operator_offset);
        if (boolean == nullptr) {
            return std::nullopt;
        }
        return TernaryOperand{false, *boolean};
    }

    // Coerce an If/While/Do condition value to a Boolean, treating Null and
    // Empty as False (verified: `If Null Then` takes the Else branch with
    // no runtime error, unlike CBool(Null), which does error). Returns
    // nullopt with an error already set only for a genuinely wrong type.
    [[nodiscard]] std::optional<bool> coerce_condition_boolean(
        const Value& value,
        const std::size_t offset,
        const std::string_view error_code,
        const std::string_view error_message) {
        if (std::holds_alternative<Null>(value) || std::holds_alternative<Empty>(value)) {
            return false;
        }
        const auto* boolean = std::get_if<bool>(&value);
        if (boolean == nullptr) {
            // Any non-zero number is True; "True"/"False" and numeric
            // strings convert as CBool would.
            if (is_number(value)) {
                return as_double(value) != 0.0;
            }
            if (const auto* text = std::get_if<std::string>(&value)) {
                std::string lowered;
                for (const char c : *text) lowered.push_back(ascii_lower(c));
                if (lowered == "true") return true;
                if (lowered == "false") return false;
                const auto parsed = parse_numeric_string(*text);
                if (parsed.status == NumericStringStatus::valid) return parsed.value != 0.0;
            }
            set_error(error_code, error_message, offset);
            return std::nullopt;
        }
        return *boolean;
    }

    [[nodiscard]] std::optional<Value> logical_binary(
        const Value& left,
        const Value& right,
        const char operation,
        const std::size_t operator_offset) {
        if (execute_ && (std::holds_alternative<ObjectInstance>(left) ||
                         std::holds_alternative<ObjectInstance>(right))) {
            Value resolved_left = left;
            Value resolved_right = right;
            if (!resolve_default_value(resolved_left, operator_offset) ||
                !resolve_default_value(resolved_right, operator_offset)) {
                return std::nullopt;
            }
            if (!std::holds_alternative<ObjectInstance>(resolved_left) &&
                !std::holds_alternative<ObjectInstance>(resolved_right)) {
                return logical_binary(resolved_left, resolved_right, operation, operator_offset);
            }
        }
        // REQ-0244: And/Or/Xor/Eqv/Imp on integer operands are bitwise.
        {
            const auto integer_of = [](const Value& v) -> std::optional<std::int32_t> {
                if (const auto* i = std::get_if<Integer>(&v)) return *i;
                if (const auto* i = std::get_if<Int16>(&v)) return static_cast<std::int32_t>(*i);
                if (const auto* i = std::get_if<Byte>(&v)) return static_cast<std::int32_t>(*i);
                return std::nullopt;
            };
            const auto li = integer_of(left);
            const auto ri = integer_of(right);
            const bool lb = std::holds_alternative<bool>(left);
            const bool rb = std::holds_alternative<bool>(right);
            if ((li || ri) && (li || lb) && (ri || rb)) {
                const std::int32_t a = li ? *li : (std::get<bool>(left) ? -1 : 0);
                const std::int32_t b = ri ? *ri : (std::get<bool>(right) ? -1 : 0);
                std::int32_t r{};
                switch (operation) {
                case 'A': r = a & b; break;
                case 'O': r = a | b; break;
                case 'X': r = a ^ b; break;
                case 'E': r = ~(a ^ b); break;
                default: r = ~a | b; break;
                }
                if (std::holds_alternative<Byte>(left) && std::holds_alternative<Byte>(right)) {
                    return Value{static_cast<Byte>(r)};
                }
                if (!std::holds_alternative<Integer>(left) && !std::holds_alternative<Integer>(right)) {
                    return Value{static_cast<Int16>(r)};  // Byte/Integer/Boolean mixes are Integer
                }
                return Value{static_cast<Integer>(r)};
            }
        }
        const auto left_operand = coerce_ternary_operand(left, operator_offset);
        if (!left_operand.has_value()) {
            return std::nullopt;
        }
        const auto right_operand = coerce_ternary_operand(right, operator_offset);
        if (!right_operand.has_value()) {
            return std::nullopt;
        }
        const std::optional<bool> left_ternary =
            left_operand->is_null ? std::optional<bool>{} : std::optional<bool>{left_operand->value};
        const std::optional<bool> right_ternary =
            right_operand->is_null ? std::optional<bool>{} : std::optional<bool>{right_operand->value};

        std::optional<bool> result;
        switch (operation) {
        case 'A':
            result = ternary_and(left_ternary, right_ternary);
            break;
        case 'O':
            result = ternary_or(left_ternary, right_ternary);
            break;
        case 'X':
            result = ternary_xor(left_ternary, right_ternary);
            break;
        case 'E':
            result = ternary_eqv(left_ternary, right_ternary);
            break;
        case 'I':
            result = ternary_imp(left_ternary, right_ternary);
            break;
        default:
            set_error("WFC0004", "unsupported operator", operator_offset);
            return std::nullopt;
        }
        if (!result.has_value()) {
            return Value{Null{}};
        }
        return Value{*result};
    }

    [[nodiscard]] int compare_strings(
        const std::string_view left_in,
        const std::string_view right_in) const {
        std::string folded_left;
        std::string folded_right;
        std::string_view left = left_in;
        std::string_view right = right_in;
        if (option_compare_text_ && (!is_ascii_text(left) || !is_ascii_text(right))) {
            folded_left = fold_case(std::string(left));
            folded_right = fold_case(std::string(right));
            left = folded_left;
            right = folded_right;
        }
        const auto common_size = std::min(left.size(), right.size());
        for (std::size_t index = 0; index < common_size; ++index) {
            const auto left_character = static_cast<unsigned char>(
                option_compare_text_ ? ascii_lower(left[index]) : left[index]);
            const auto right_character = static_cast<unsigned char>(
                option_compare_text_ ? ascii_lower(right[index]) : right[index]);
            if (left_character < right_character) {
                return -1;
            }
            if (left_character > right_character) {
                return 1;
            }
        }
        if (left.size() < right.size()) {
            return -1;
        }
        if (left.size() > right.size()) {
            return 1;
        }
        return 0;
    }

    [[nodiscard]] bool values_equal(const Value& left, const Value& right) const {
        if (const auto* left_string = std::get_if<std::string>(&left)) {
            return compare_strings(*left_string, std::get<std::string>(right)) == 0;
        }
        return left == right;
    }

    [[nodiscard]] std::optional<Value> compare(
        const Value& left,
        const Value& right,
        const std::string_view operation,
        const std::size_t operator_offset) {
        // A comparison against Null yields Null itself (three-valued
        // logic), not True or False: VB6 famously cannot answer "x = Null"
        // definitively, which is why IsNull exists. Verified against the
        // local VB6 6.00.8176 reference.
        if (std::holds_alternative<Null>(left) || std::holds_alternative<Null>(right)) {
            return Value{Null{}};
        }
        // An object beside a value compares through its default member.
        if (execute_ && (std::holds_alternative<ObjectInstance>(left) !=
                         std::holds_alternative<ObjectInstance>(right))) {
            Value resolved_left = left;
            Value resolved_right = right;
            if (!resolve_default_value(resolved_left, operator_offset) ||
                !resolve_default_value(resolved_right, operator_offset)) {
                return std::nullopt;
            }
            if (!is_object_reference(resolved_left) && !is_object_reference(resolved_right)) {
                return compare(resolved_left, resolved_right, operation, operator_offset);
            }
        }
        // Object references compare only through Is, matching real VB6
        // (plain =/<> on an object reference requires a default member,
        // which nothing in this evaluator's minimal object model has).
        if (is_object_reference(left) || is_object_reference(right)) {
            set_error("WFC0107", "object comparison requires Is", operator_offset);
            return std::nullopt;
        }
        // Empty coerces to whichever side of the comparison the other
        // operand expects: a number if compared against a number, an empty
        // string if compared against a String. Verified (Empty = 0 and
        // Empty = "" are both True).
        if (std::holds_alternative<Empty>(left) || std::holds_alternative<Empty>(right)) {
            Value coerced_left = left;
            Value coerced_right = right;
            if (std::holds_alternative<Empty>(left)) {
                coerced_left = std::holds_alternative<std::string>(right) ? Value{std::string{}}
                                                                           : Value{Integer{0}};
            }
            if (std::holds_alternative<Empty>(right)) {
                coerced_right = std::holds_alternative<std::string>(left) ? Value{std::string{}}
                                                                           : Value{Integer{0}};
            }
            return compare(coerced_left, coerced_right, operation, operator_offset);
        }
        // REQ-0242: a Date compares by serial against a Date or a number.
        if (std::holds_alternative<DateValue>(left) || std::holds_alternative<DateValue>(right)) {
            const auto serial_of = [](const Value& v) -> std::optional<double> {
                if (const auto* d = std::get_if<DateValue>(&v)) return d->serial;
                if (is_number(v)) return as_double(v);
                return std::nullopt;
            };
            const auto l = serial_of(left);
            const auto r = serial_of(right);
            if (!l.has_value() || !r.has_value()) {
                set_error("WFC0018", "comparison requires operands of the same type", operator_offset);
                return std::nullopt;
            }
            return compare(Value{*l}, Value{*r}, operation, operator_offset);
        }
        // Boolean beside a number compares as the number (True = -1).
        if (std::holds_alternative<bool>(left) != std::holds_alternative<bool>(right)) {
            const auto as_small = [](const Value& v) -> Value {
                if (const auto* flag = std::get_if<bool>(&v)) {
                    return Value{static_cast<Int16>(*flag ? -1 : 0)};
                }
                return v;
            };
            const Value other = std::holds_alternative<bool>(left) ? right : left;
            if (is_number(other) && !std::holds_alternative<DateValue>(other)) {
                return compare(as_small(left), as_small(right), operation, operator_offset);
            }
        }
        // A String beside a number: Variant operands follow VB's Variant comparison rules.
        if ((std::holds_alternative<std::string>(left) && is_number(right)) ||
            (std::holds_alternative<std::string>(right) && is_number(left))) {
            if ((variant_string_seen_ || variant_number_seen_) && execute_) {
                const bool string_left = std::holds_alternative<std::string>(left);
                const std::string& text = std::get<std::string>(string_left ? left : right);
                const Value& number = string_left ? right : left;
                int ordering{};
                if (variant_string_seen_ && variant_number_seen_) {
                    ordering = string_left ? 1 : -1;  // a numeric Variant sorts before a string Variant
                } else if (variant_string_seen_) {
                    const auto parsed = parse_numeric_string(text);
                    if (parsed.status != NumericStringStatus::valid) {
                        set_error("WFC0018", "comparison requires operands of the same type",
                                  operator_offset);
                        return std::nullopt;
                    }
                    const double l = string_left ? parsed.value : as_double(number);
                    const double r = string_left ? as_double(number) : parsed.value;
                    ordering = l < r ? -1 : l > r ? 1 : 0;
                } else {
                    const std::string rendered = render(number);
                    ordering = string_left ? compare_strings(text, rendered)
                                           : compare_strings(rendered, text);
                }
                if (operation == "=") return Value{ordering == 0};
                if (operation == "<>") return Value{ordering != 0};
                if (operation == "<") return Value{ordering < 0};
                if (operation == "<=") return Value{ordering <= 0};
                if (operation == ">") return Value{ordering > 0};
                return Value{ordering >= 0};
            }
        }
        // Long and Double operands compare numerically, in either combination;
        // int32 widens to double exactly, so the ordering is precise.
        if (is_number(left) && is_number(right)) {
            const double left_value = as_double(left);
            const double right_value = as_double(right);
            if (operation == "=") {
                return Value{left_value == right_value};
            }
            if (operation == "<>") {
                return Value{left_value != right_value};
            }
            if (operation == "<") {
                return Value{left_value < right_value};
            }
            if (operation == "<=") {
                return Value{left_value <= right_value};
            }
            if (operation == ">") {
                return Value{left_value > right_value};
            }
            if (operation == ">=") {
                return Value{left_value >= right_value};
            }
            set_error("WFC0004", "unsupported operator", operator_offset);
            return std::nullopt;
        }

        if (left.index() != right.index()) {
            if (!execute_) {
                return Value{false};  // placeholder operand of a not-taken branch
            }
            set_error("WFC0018", "comparison requires operands of the same type", operator_offset);
            return std::nullopt;
        }

        if (operation == "=") {
            return Value{values_equal(left, right)};
        }
        if (operation == "<>") {
            return Value{!values_equal(left, right)};
        }
        if (std::holds_alternative<bool>(left)) {
            set_error("WFC0018", "Boolean ordering is not supported", operator_offset);
            return std::nullopt;
        }
        if (!std::holds_alternative<std::string>(left)) {
            set_error("WFC0018", "ordering is not supported for this type", operator_offset);
            return std::nullopt;
        }

        bool less{};
        bool greater{};
        {
            const auto& left_string = std::get<std::string>(left);
            const auto& right_string = std::get<std::string>(right);
            const auto ordering = compare_strings(left_string, right_string);
            less = ordering < 0;
            greater = ordering > 0;
        }

        if (operation == "<") {
            return Value{less};
        }
        if (operation == "<=") {
            return Value{!greater};
        }
        if (operation == ">") {
            return Value{greater};
        }
        if (operation == ">=") {
            return Value{!less};
        }
        set_error("WFC0004", "unsupported operator", operator_offset);
        return std::nullopt;
    }

    // Evaluate `+`, `-`, `*`, and `/`. Two Long operands under `+`/`-`/`*` keep
    // the exact integer path (including overflow); every other combination
    // computes in the wider of the two operand categories, in VB6's
    // Long < Currency < Single < Double promotion order (a Long-only `/`
    // still promotes to Double, matching the pre-existing rule). Currency
    // computes with exact scaled-int64 arithmetic rather than floating
    // point, preserving its whole point: exact decimal money math.
    // REQ-0247: Byte participates in arithmetic as a Long.
    [[nodiscard]] static Value widen_byte(const Value& value) {
        if (const auto* byte = std::get_if<Byte>(&value)) {
            return Value{static_cast<Integer>(*byte)};
        }
        if (const auto* flag = std::get_if<bool>(&value)) {
            return Value{static_cast<Int16>(*flag ? -1 : 0)};  // True is -1
        }
        return value;
    }

    [[nodiscard]] std::optional<Value> numeric_binary(
        const Value& left_in,
        const Value& right_in,
        const char operation,
        const std::size_t operator_offset) {
        if (execute_ && (std::holds_alternative<ObjectInstance>(left_in) ||
                         std::holds_alternative<ObjectInstance>(right_in))) {
            Value resolved_left = left_in;
            Value resolved_right = right_in;
            if (!resolve_default_value(resolved_left, operator_offset) ||
                !resolve_default_value(resolved_right, operator_offset)) {
                return std::nullopt;
            }
            if (!std::holds_alternative<ObjectInstance>(resolved_left) &&
                !std::holds_alternative<ObjectInstance>(resolved_right)) {
                return numeric_binary(resolved_left, resolved_right, operation, operator_offset);
            }
        }
        // Byte/Integer operands keep their own width: Byte op Byte is a Byte, anything else
        // small is an Integer (a Variant operand promotes instead of overflowing).
        if ((operation == '+' || operation == '-' || operation == '*' || operation == '\\' ||
             operation == '%') &&
            (std::holds_alternative<Byte>(left_in) || std::holds_alternative<Int16>(left_in)) &&
            (std::holds_alternative<Byte>(right_in) || std::holds_alternative<Int16>(right_in))) {
            const bool both_bytes =
                std::holds_alternative<Byte>(left_in) && std::holds_alternative<Byte>(right_in);
            if (!execute_) {
                return both_bytes ? Value{Byte{}} : Value{Int16{}};
            }
            const auto small_value = [](const Value& v) -> std::int64_t {
                if (const auto* b = std::get_if<Byte>(&v)) return *b;
                return std::get<Int16>(v);
            };
            const std::int64_t a = small_value(left_in);
            const std::int64_t b = small_value(right_in);
            if ((operation == '\\' || operation == '%') && b == 0) {
                set_error("WFC0008", "division by zero", operator_offset);
                return std::nullopt;
            }
            std::int64_t result{};
            switch (operation) {
            case '+': result = a + b; break;
            case '-': result = a - b; break;
            case '*': result = a * b; break;
            case '\\': result = a / b; break;
            default: result = a % b; break;
            }
            if (both_bytes && result >= 0 && result <= 255) {
                return Value{static_cast<Byte>(result)};
            }
            if (!both_bytes && result >= -32768 && result <= 32767) {
                return Value{static_cast<Int16>(result)};
            }
            if (both_bytes && result >= -32768 && result <= 32767 && variant_operand_seen_) {
                return Value{static_cast<Int16>(result)};
            }
            if (variant_operand_seen_) {
                return Value{static_cast<Integer>(result)};
            }
            set_error("WFC0009", "integer overflow", operator_offset);
            return std::nullopt;
        }
        const Value left = widen_byte(left_in);
        const Value right = widen_byte(right_in);
        if (operation == '+' && std::holds_alternative<std::string>(left) &&
            std::holds_alternative<std::string>(right)) {
            return Value{std::get<std::string>(left) + std::get<std::string>(right)};
        }
        if (std::holds_alternative<Null>(left) || std::holds_alternative<Null>(right)) {
            return Value{Null{}};
        }
        if (std::holds_alternative<Empty>(left) || std::holds_alternative<Empty>(right)) {
            const Value coerced_left =
                std::holds_alternative<Empty>(left) ? Value{Integer{0}} : left;
            const Value coerced_right =
                std::holds_alternative<Empty>(right) ? Value{Integer{0}} : right;
            return numeric_binary(coerced_left, coerced_right, operation, operator_offset);
        }
        {
            // REQ-0269: a numeric String beside a number (or another numeric
            // String under - * /) converts to Double, as VB does for Variants.
            const bool left_text = std::holds_alternative<std::string>(left);
            const bool right_text = std::holds_alternative<std::string>(right);
            if ((left_text || right_text) && !(left_text && right_text && operation == '+')) {
                const auto convert = [&](const Value& v) -> std::optional<Value> {
                    if (const auto* text = std::get_if<std::string>(&v)) {
                        const auto parsed = parse_numeric_string(*text);
                        if (parsed.status != NumericStringStatus::valid) return std::nullopt;
                        return Value{parsed.value};
                    }
                    if (is_number(v) && !std::holds_alternative<DateValue>(v)) return v;
                    return std::nullopt;
                };
                const auto converted_left = convert(left);
                const auto converted_right = convert(right);
                if (converted_left && converted_right) {
                    return numeric_binary(*converted_left, *converted_right, operation, operator_offset);
                }
                if (execute_) {
                    err_number_ = 13;
                    err_description_ = "Type mismatch";
                    set_error("WFC0300", "Type mismatch", operator_offset);
                    return std::nullopt;
                }
                return Value{0.0};
            }
        }
        if (std::holds_alternative<DateValue>(left) || std::holds_alternative<DateValue>(right)) {
            // REQ-0242: Date +/- number stays a Date; Date - Date is a
            // Double day count; everything else computes as Double.
            const auto* left_date = std::get_if<DateValue>(&left);
            const auto* right_date = std::get_if<DateValue>(&right);
            const auto to_double = [](const Value& v) -> std::optional<double> {
                if (const auto* d = std::get_if<DateValue>(&v)) return d->serial;
                if (is_number(v)) return as_double(v);
                return std::nullopt;
            };
            const auto l = to_double(left);
            const auto r = to_double(right);
            if (!l.has_value() || !r.has_value()) {
                static_cast<void>(require_integer(l.has_value() ? right : left, operator_offset));
                return std::nullopt;
            }
            double result{};
            switch (operation) {
            case '+': result = *l + *r; break;
            case '-': result = *l - *r; break;
            case '*': result = *l * *r; break;
            default:
                if (*r == 0.0) {
                    set_error("WFC0008", "division by zero", operator_offset);
                    return std::nullopt;
                }
                result = *l / *r;
                break;
            }
            if ((operation == '+' && (left_date != nullptr) != (right_date != nullptr)) ||
                (operation == '-' && left_date != nullptr && right_date == nullptr)) {
                return Value{DateValue{result}};
            }
            if (operation == '+' && left_date != nullptr && right_date != nullptr) {
                return Value{DateValue{result}};
            }
            return Value{result};
        }
        if (operation != '/' &&
            std::holds_alternative<Integer>(left) &&
            std::holds_alternative<Integer>(right)) {
            return integer_binary(left, right, operation, operator_offset);
        }
        if (operation != '/' &&
            std::holds_alternative<Int16>(left) &&
            std::holds_alternative<Int16>(right)) {
            return short_integer_binary(left, right, operation, operator_offset);
        }
        if (!is_number(left) || !is_number(right)) {
            if (require_integer(left, operator_offset) == nullptr) {
                return std::nullopt;
            }
            static_cast<void>(require_integer(right, operator_offset));
            return std::nullopt;
        }

        auto category = std::max(numeric_category(left), numeric_category(right));
        if (operation != '/' &&
            (category == NumericCategory::integer || category == NumericCategory::int16)) {
            // Both-Long and both-Int16 were already short-circuited above, so
            // reaching here with an integral-only category means one operand
            // is Int16 and the other Long: widen the Int16 side exactly (it
            // always fits) and compute exact checked Long arithmetic, the
            // same as Long op Long.
            const auto widen_to_long = [](const Value& value) -> Value {
                if (const auto* short_integer = std::get_if<Int16>(&value)) {
                    return Value{static_cast<Integer>(*short_integer)};
                }
                return value;
            };
            return integer_binary(
                widen_to_long(left), widen_to_long(right), operation, operator_offset);
        }
        if (category == NumericCategory::integer || category == NumericCategory::int16) {
            // Only reachable for `/` between any combination of Long/Int16,
            // which always promotes to Double under the pre-existing rule.
            category = NumericCategory::double_precision;
        }

        if (!execute_) {
            switch (category) {
            case NumericCategory::currency:
                return Value{Currency{}};
            case NumericCategory::single:
                return Value{0.0f};
            case NumericCategory::decimal_precision:
                return Value{Decimal{}};
            default:
                return Value{0.0};
            }
        }

        if (category == NumericCategory::decimal_precision) {
            // Int16, Long, Currency, Single, or Double can all be the *other*
            // operand here (each sorts below decimal_precision), so each
            // needs an exact or best-effort widening to Decimal. Currency
            // widens exactly (its scaled int64 becomes a scale-4 Decimal
            // mantissa). Int16, Long, Single, and Double have no dedicated
            // branch below and fall through to the generic Double-to-Decimal
            // conversion via `as_double`, which is exact for Int16/Long and
            // best-effort (shortest round-tripping decimal text) for
            // Single/Double.
            const auto to_decimal = [](const Value& value) -> Decimal {
                if (const auto* decimal = std::get_if<Decimal>(&value)) {
                    return *decimal;
                }
                if (const auto* integer = std::get_if<Integer>(&value)) {
                    Decimal result;
                    result.negative = *integer < 0;
                    result.mantissa = big_from_u32(
                        static_cast<std::uint32_t>(*integer < 0 ? -static_cast<std::int64_t>(*integer)
                                                                 : *integer));
                    return result;
                }
                if (const auto* currency = std::get_if<Currency>(&value)) {
                    Decimal result;
                    result.negative = currency->scaled < 0;
                    const auto magnitude = currency->scaled < 0
                        ? (~static_cast<std::uint64_t>(currency->scaled) + 1ULL)
                        : static_cast<std::uint64_t>(currency->scaled);
                    result.mantissa.limb[0] = static_cast<std::uint32_t>(magnitude);
                    result.mantissa.limb[1] = static_cast<std::uint32_t>(magnitude >> 32U);
                    result.scale = 4U;
                    return result;
                }
                return decimal_from_double(as_double(value)).value_or(Decimal{});
            };
            const Decimal left_decimal = to_decimal(left);
            const Decimal right_decimal = to_decimal(right);
            std::optional<Decimal> result;
            switch (operation) {
            case '+':
                result = decimal_add(left_decimal, right_decimal);
                break;
            case '-':
                result = decimal_subtract(left_decimal, right_decimal);
                break;
            case '*':
                result = decimal_multiply(left_decimal, right_decimal);
                break;
            case '/':
                if (is_zero_big(right_decimal.mantissa)) {
                    set_error("WFC0008", "division by zero", operator_offset);
                    return std::nullopt;
                }
                result = decimal_divide(left_decimal, right_decimal);
                break;
            default:
                set_error("WFC0004", "unsupported operator", operator_offset);
                return std::nullopt;
            }
            if (!result.has_value()) {
                set_error("WFC0009", "numeric overflow", operator_offset);
                return std::nullopt;
            }
            return Value{*result};
        }

        if (category == NumericCategory::currency) {
            const std::int64_t left_scaled = as_currency_scaled(left);
            const std::int64_t right_scaled = as_currency_scaled(right);
            std::optional<std::int64_t> result;
            switch (operation) {
            case '+': {
                std::int64_t sum{};
                const bool overflowed =
                    (right_scaled > 0 && left_scaled > std::numeric_limits<std::int64_t>::max() - right_scaled) ||
                    (right_scaled < 0 && left_scaled < std::numeric_limits<std::int64_t>::min() - right_scaled);
                if (!overflowed) {
                    sum = left_scaled + right_scaled;
                    result = sum;
                }
                break;
            }
            case '-': {
                std::int64_t difference{};
                const bool overflowed =
                    (right_scaled < 0 && left_scaled > std::numeric_limits<std::int64_t>::max() + right_scaled) ||
                    (right_scaled > 0 && left_scaled < std::numeric_limits<std::int64_t>::min() + right_scaled);
                if (!overflowed) {
                    difference = left_scaled - right_scaled;
                    result = difference;
                }
                break;
            }
            case '*':
                result = currency_multiply(left_scaled, right_scaled);
                break;
            case '/':
                if (right_scaled == 0) {
                    set_error("WFC0008", "division by zero", operator_offset);
                    return std::nullopt;
                }
                result = currency_divide(left_scaled, right_scaled);
                break;
            default:
                set_error("WFC0004", "unsupported operator", operator_offset);
                return std::nullopt;
            }
            if (!result.has_value()) {
                set_error("WFC0009", "numeric overflow", operator_offset);
                return std::nullopt;
            }
            return Value{Currency{*result}};
        }

        if (category == NumericCategory::single) {
            const float left_value = as_single(left);
            const float right_value = as_single(right);
            float result{};
            switch (operation) {
            case '+':
                result = left_value + right_value;
                break;
            case '-':
                result = left_value - right_value;
                break;
            case '*':
                result = left_value * right_value;
                break;
            case '/':
                if (right_value == 0.0f) {
                    set_error("WFC0008", "division by zero", operator_offset);
                    return std::nullopt;
                }
                result = left_value / right_value;
                break;
            default:
                set_error("WFC0004", "unsupported operator", operator_offset);
                return std::nullopt;
            }
            if (!std::isfinite(result)) {
                set_error("WFC0009", "numeric overflow", operator_offset);
                return std::nullopt;
            }
            return Value{result};
        }

        const double left_value = as_double(left);
        const double right_value = as_double(right);
        double result{};
        switch (operation) {
        case '+':
            result = left_value + right_value;
            break;
        case '-':
            result = left_value - right_value;
            break;
        case '*':
            result = left_value * right_value;
            break;
        case '/':
            if (right_value == 0.0) {
                set_error("WFC0008", "division by zero", operator_offset);
                return std::nullopt;
            }
            result = left_value / right_value;
            break;
        default:
            set_error("WFC0004", "unsupported operator", operator_offset);
            return std::nullopt;
        }
        if (!std::isfinite(result)) {
            set_error("WFC0009", "numeric overflow", operator_offset);
            return std::nullopt;
        }
        return Value{result};
    }

    // Coerce a numeric operand to Long for the integer operators, rounding a
    // Double to the nearest even integer (VB6 banker's rounding). Non-numeric
    // operands and out-of-range magnitudes are rejected.
    [[nodiscard]] std::optional<Integer> coerce_long(
        const Value& value,
        const std::size_t operator_offset) {
        if (const auto* integer = std::get_if<Integer>(&value)) {
            return *integer;
        }
        if (const auto* short_integer = std::get_if<Int16>(&value)) {
            return static_cast<Integer>(*short_integer);
        }
        if (const auto* number = std::get_if<double>(&value)) {
            const double rounded = std::nearbyint(*number);
            if (!(rounded >= static_cast<double>(std::numeric_limits<Integer>::min()) &&
                  rounded <= static_cast<double>(std::numeric_limits<Integer>::max()))) {
                set_error("WFC0009", "integer overflow", operator_offset);
                return std::nullopt;
            }
            return static_cast<Integer>(rounded);
        }
        if (const auto* single = std::get_if<float>(&value)) {
            const double rounded = std::nearbyint(static_cast<double>(*single));
            if (!(rounded >= static_cast<double>(std::numeric_limits<Integer>::min()) &&
                  rounded <= static_cast<double>(std::numeric_limits<Integer>::max()))) {
                set_error("WFC0009", "integer overflow", operator_offset);
                return std::nullopt;
            }
            return static_cast<Integer>(rounded);
        }
        if (std::holds_alternative<Currency>(value) || std::holds_alternative<Decimal>(value)) {
            const double rounded = std::nearbyint(as_double(value));
            if (!(rounded >= static_cast<double>(std::numeric_limits<Integer>::min()) &&
                  rounded <= static_cast<double>(std::numeric_limits<Integer>::max()))) {
                set_error("WFC0009", "integer overflow", operator_offset);
                return std::nullopt;
            }
            return static_cast<Integer>(rounded);
        }
        if (const auto* byte = std::get_if<Byte>(&value)) {
            return static_cast<Integer>(*byte);
        }
        if (const auto* flag = std::get_if<bool>(&value)) {
            return *flag ? Integer{-1} : Integer{0};
        }
        if (std::holds_alternative<Empty>(value)) {
            return Integer{0};
        }
        if (const auto* text = std::get_if<std::string>(&value); text != nullptr && execute_) {
            const auto parsed = parse_numeric_string(*text);
            if (parsed.status == NumericStringStatus::valid &&
                std::fabs(parsed.value) < 2147483647.5) {
                return static_cast<Integer>(std::nearbyint(parsed.value));
            }
        }
        if (const auto* date = std::get_if<DateValue>(&value)) {
            return static_cast<Integer>(std::nearbyint(date->serial));
        }
        if (!execute_) {
            return Integer{0};  // a placeholder operand of a not-taken branch
        }
        set_error("WFC0007", "operator requires integer operands", operator_offset);
        return std::nullopt;
    }

    [[nodiscard]] std::optional<Value> integer_binary(
        const Value& left_in,
        const Value& right_in,
        const char operation,
        const std::size_t operator_offset) {
        if (execute_ && (std::holds_alternative<ObjectInstance>(left_in) ||
                         std::holds_alternative<ObjectInstance>(right_in))) {
            Value resolved_left = left_in;
            Value resolved_right = right_in;
            if (!resolve_default_value(resolved_left, operator_offset) ||
                !resolve_default_value(resolved_right, operator_offset)) {
                return std::nullopt;
            }
            if (!std::holds_alternative<ObjectInstance>(resolved_left) &&
                !std::holds_alternative<ObjectInstance>(resolved_right)) {
                return integer_binary(resolved_left, resolved_right, operation, operator_offset);
            }
        }
        if ((std::holds_alternative<Byte>(left_in) || std::holds_alternative<Int16>(left_in)) &&
            (std::holds_alternative<Byte>(right_in) || std::holds_alternative<Int16>(right_in))) {
            return numeric_binary(left_in, right_in, operation, operator_offset);
        }
        const Value left = widen_byte(left_in);
        const Value right = widen_byte(right_in);
        const auto left_coerced = coerce_long(left, operator_offset);
        if (!left_coerced.has_value()) {
            return std::nullopt;
        }
        const auto right_coerced = coerce_long(right, operator_offset);
        if (!right_coerced.has_value()) {
            return std::nullopt;
        }
        const Integer left_integer = *left_coerced;
        const Integer right_integer = *right_coerced;
        if (!execute_) {
            return Value{Integer{}};
        }

        if ((operation == '\\' || operation == '%') && right_integer == 0) {
            set_error("WFC0008", "division by zero", operator_offset);
            return std::nullopt;
        }
        if ((operation == '\\' || operation == '%') &&
            left_integer == std::numeric_limits<Integer>::min() && right_integer == -1) {
            set_error("WFC0009", "integer overflow", operator_offset);
            return std::nullopt;
        }

        std::int64_t result{};
        switch (operation) {
        case '+':
            result = static_cast<std::int64_t>(left_integer) + right_integer;
            break;
        case '-':
            result = static_cast<std::int64_t>(left_integer) - right_integer;
            break;
        case '*':
            result = static_cast<std::int64_t>(left_integer) * right_integer;
            break;
        case '\\':
            result = left_integer / right_integer;
            break;
        case '%':
            result = left_integer % right_integer;
            break;
        default:
            set_error("WFC0004", "unsupported operator", operator_offset);
            return std::nullopt;
        }

        if (result < std::numeric_limits<Integer>::min() ||
            result > std::numeric_limits<Integer>::max()) {
            if (variant_operand_seen_ && (operation == '+' || operation == '-' || operation == '*')) {
                return Value{static_cast<double>(result)};
            }
            set_error("WFC0009", "integer overflow", operator_offset);
            return std::nullopt;
        }
        return Value{static_cast<Integer>(result)};
    }

    // Exact, checked Int16 (VB6 Integer) `+`/`-`/`*` for two Int16 operands.
    // Callers guarantee both operands are already Int16 and the operation is
    // not `/` (which always promotes to Double, matching Long op Long).
    [[nodiscard]] std::optional<Value> short_integer_binary(
        const Value& left,
        const Value& right,
        const char operation,
        const std::size_t operator_offset) {
        const Int16 left_integer = std::get<Int16>(left);
        const Int16 right_integer = std::get<Int16>(right);
        if (!execute_) {
            return Value{Int16{}};
        }

        std::int32_t result{};
        switch (operation) {
        case '+':
            result = static_cast<std::int32_t>(left_integer) + right_integer;
            break;
        case '-':
            result = static_cast<std::int32_t>(left_integer) - right_integer;
            break;
        case '*':
            result = static_cast<std::int32_t>(left_integer) * right_integer;
            break;
        default:
            set_error("WFC0004", "unsupported operator", operator_offset);
            return std::nullopt;
        }

        if (result < std::numeric_limits<Int16>::min() ||
            result > std::numeric_limits<Int16>::max()) {
            if (variant_operand_seen_) {
                return Value{static_cast<Integer>(result)};
            }
            set_error("WFC0009", "integer overflow", operator_offset);
            return std::nullopt;
        }
        return Value{static_cast<Int16>(result)};
    }

    // REQ-0268: VB6 renders a Double with 15 significant digits and a Single
    // with 7 (C's %G rules: exponent form below 1E-4 or from 1E15/1E7 up).
    [[nodiscard]] static std::string render_floating(const double number, const int digits) {
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

    [[nodiscard]] static std::string render(const Value& value) {
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

    void set_error(
        const std::string_view code,
        const std::string_view message,
        const std::size_t offset) {
        if (code == "WFC0135" && execute_) {
            // An unknown member reached at run time is a catchable "Object doesn't support..."
            err_number_ = 438;
            err_description_ = "Object doesn't support this property or method";
            err_source_.clear();
            error_ = failure("WFC0300", err_description_, offset);
            error_source_data_ = source_.data();
            return;
        }
        error_ = failure(code, message, offset);
        error_source_data_ = source_.data();
    }

    const char* error_source_data_{};
    std::string_view source_;
    bool allow_identifiers_;
    const char* main_source_data_{};
    std::size_t offset_{};
    // scopes_[0] is the single module-level scope; every entry after it is
    // one active procedure call's local scope (its parameters and locally
    // Dim'd variables), pushed on call and popped on return. Variable
    // lookups only ever consult scopes_.back() and scopes_.front() -- never
    // any frame in between -- matching VB6's module/procedure two-level
    // scoping.
    // A std::deque, not std::vector: pushing/popping a call frame must
    // never invalidate a Value* captured earlier (e.g. a ByRef argument's
    // write-back target from an enclosing call) -- std::deque guarantees
    // references and pointers to existing elements survive push_back/
    // pop_back, where std::vector's reallocation would not.
    std::deque<Scope> scopes_{Scope{}};
    std::unordered_map<std::string, ProcedureDef> procedures_;
    // Class module sources supplied alongside the standard module (see
    // wfc::ClassModuleSource), and the class definitions scan_classes()
    // extracts from them (keyed by lowercased class name, like every other
    // identifier lookup in this evaluator).
    std::vector<wfc::ClassModuleSource> class_sources_;
    std::unordered_map<std::string, ClassDef> class_definitions_;
    // The stack of instances currently executing a method/property call,
    // innermost last. Non-empty exactly while executing inside a class
    // member's body; find_variable consults its back()'s fields instead of
    // the real module scope while it is non-empty (see find_variable), and
    // an unqualified call (parse_primary_base/parse_call_statement) checks
    // its back()'s class for a sibling method/property before falling back
    // to an "undeclared" error, so a method can call another method of its
    // own class -- including itself, for recursion -- without an explicit
    // `Me.` qualifier. A std::deque for the same pointer-stability reason
    // scopes_ is one: an inner call's pushed InstanceData must never be
    // invalidated by an outer call's later, unrelated container growth
    // (moot for a deque, unlike a vector).
    std::deque<InstanceData*> instance_scopes_;
    std::vector<std::string> enum_names_;
    std::deque<std::string> udt_sources_;
    std::vector<std::string> udt_names_;
    bool bare_call_arguments_{};
    bool udt_array_{};
    std::size_t fixed_string_length_{};
    std::optional<Value> app_instance_;
    bool retry_statement_{};
    std::unordered_set<const char*> identifier_statements_;
    std::unordered_map<const char*, bool> append_eligibility_;
    // Set when the current statement read a Variant variable: Variant arithmetic that overflows
    // is promoted (Integer -> Long -> Double) instead of raising Overflow.
    bool variant_operand_seen_{};
    bool vb_number_spacing_{};
    bool pending_static_procedure_{};
    bool variant_string_seen_{};
    bool variant_number_seen_{};
    bool integer_literals_are_integer_{true};
    bool pending_lazy_new_{};
    std::string current_class_scan_name_;
    // Public Enum/Const members declared in class modules.
    std::unordered_map<std::string, Value> global_class_constants_;
    // REQ-0238 error-handling state.
    Integer err_number_{};
    Integer erl_{};  // the last numbered line executed (VB's Erl)
    std::string err_description_;
    std::string err_source_;
    bool jump_pending_{};
    std::size_t jump_target_{};
    // REQ-0236 With-block state.
    std::unordered_map<std::string, Value> with_slots_;
    Scope with_scope_;
    std::vector<std::string> with_names_;
    std::size_t with_counter_{};
    // REQ-0206: the innermost currently-executing call's own ProcedureDef,
    // so a `Static` statement inside its body can find (and later copy a
    // value back into) that exact definition's persistent `statics` Scope.
    // A plain pointer with save/restore around each call in
    // invoke_definition, mirroring execute_/offset_/source_ -- procedure
    // calls nest but are never concurrent, so there is never more than one
    // "current" definition to restore per returning call.
    const ProcedureDef* current_procedure_def_{};
    std::string output_;
    bool has_output_line_{};
    bool output_line_open_{};
    bool discard_print_{};
    std::string debug_output_;
    bool pending_next_comma_{};
    bool strict_declarations_{};
    std::map<std::string, std::string> settings_;
    std::unordered_set<std::string> module_names_;
    bool end_requested_{};
    std::map<Integer, OpenFile> files_;
    std::vector<std::string> dir_matches_;
    std::size_t dir_index_{};
    bool execute_{true};
    bool allow_declarations_{true};
    bool constant_expression_{};
    bool module_body_started_{};
    bool option_explicit_{};
    bool option_compare_set_{};
    bool option_compare_text_{};
    // REQ-0226: `Option Base 1` (default `0`, matching VB6's own
    // undeclared default). Only ever changes the lower bound a *bound-
    // less* dimension gets (`Dim arr(n)` meaning `<base> To n`); a
    // `<lower> To <upper>` dimension always uses its own explicit
    // `<lower>` regardless of this setting, and a `ParamArray`'s array is
    // always `0`-based no matter what `Option Base` says (a real,
    // documented VB6 exception, not an oversight).
    bool option_base_set_{};
    bool option_base_one_{};
    std::size_t do_depth_{};
    bool exit_do_requested_{};
    std::size_t for_depth_{};
    bool exit_for_requested_{};
    std::size_t procedure_depth_{};
    const char* stack_base_{};
    std::size_t stack_budget_{};
    std::size_t max_procedure_depth_{64U};  // raised when running on a large stack
    bool exit_sub_requested_{};
    bool exit_function_requested_{};
    // Rnd/Randomize generator state. 327680 is the verified default seed of
    // the reference VB6 6.00.8176 runtime's Rnd generator (confirmed against
    // a local probe: the first Rnd() call from this seed is 0.7055475,
    // matching the well-known VB6 fingerprint value).
    std::uint32_t rnd_state_{327680U};
    float rnd_last_value_{};
    wfc::Evaluation error_;
};



// REQ-0261: joins ` _` line continuations by blanking the underscore and the
// line break (byte offsets are unchanged).
// `[bracketed name]` identifiers: plain names lose the brackets; names with spaces or a
// leading underscore become `wfcb_` identifiers; `[_NewEnum]` maps to `NewEnum`.
void rewrite_bracketed_identifiers(std::string& text) {
    if (text.find('[') == std::string::npos) return;
    std::string out;
    out.reserve(text.size());
    bool in_string = false;
    bool in_comment = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        if (ch == '\n') {
            in_string = false;
            in_comment = false;
        } else if (!in_comment) {
            if (ch == '"') {
                in_string = !in_string;
            } else if (!in_string && ch == '\'') {
                in_comment = true;
            } else if (!in_string && ch == '[') {
                const auto close = text.find_first_of("]\n", i + 1);
                if (close != std::string::npos && text[close] == ']' && close > i + 1) {
                    std::string name = text.substr(i + 1, close - i - 1);
                    bool plain = !std::isdigit(static_cast<unsigned char>(name[0])) && name[0] != '_';
                    for (const char c : name) {
                        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) plain = false;
                    }
                    if (name == "_NewEnum") {
                        name = "NewEnum";
                    } else if (!plain) {
                        for (char& c : name) {
                            if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) c = '_';
                        }
                        name = "wfcb_" + name;
                    }
                    out += name;
                    i = close;
                    continue;
                }
            }
        }
        out.push_back(ch);
    }
    text = std::move(out);
}

// `As IUnknown` / `As IDispatch` are plain object references to this interpreter.
void rewrite_object_aliases(std::string& text) {
    const auto lower_at = [&](const std::size_t at, const std::string_view word) {
        if (at + word.size() > text.size()) return false;
        for (std::size_t i = 0; i < word.size(); ++i) {
            const auto ch = static_cast<char>(std::tolower(static_cast<unsigned char>(text[at + i])));
            if (ch != word[i]) return false;
        }
        const auto after = at + word.size();
        return after >= text.size() ||
               !(std::isalnum(static_cast<unsigned char>(text[after])) || text[after] == '_');
    };
    for (std::size_t at = 0; at + 7 < text.size(); ++at) {
        if (text[at] != 'I' && text[at] != 'i') continue;
        const bool unknown = lower_at(at, "iunknown");
        const bool dispatch = !unknown && lower_at(at, "idispatch");
        if (!unknown && !dispatch) continue;
        std::size_t before = at;
        while (before > 0 && (text[before - 1] == ' ' || text[before - 1] == '\t')) --before;
        if (before < 3 || before == at) continue;
        if (!lower_at(before - 2, "as") || (before > 2 && (std::isalnum(static_cast<unsigned char>(text[before - 3])) || text[before - 3] == '_'))) continue;
        text.replace(at, unknown ? 8U : 9U, "Object");
    }
}

void join_line_continuations(std::string& text, std::vector<std::size_t>* joined = nullptr) {
    std::size_t line_start = 0;
    while (line_start < text.size()) {
        std::size_t line_end = text.find('\n', line_start);
        const bool has_newline = line_end != std::string::npos;
        if (!has_newline) {
            line_end = text.size();
        }
        std::size_t content_end = line_end;
        if (content_end > line_start && text[content_end - 1] == '\r') {
            --content_end;
        }
        bool in_string = false;
        bool in_comment = false;
        for (std::size_t i = line_start; i < content_end && !in_comment; ++i) {
            if (text[i] == '"') {
                in_string = !in_string;
            } else if (text[i] == '\'' && !in_string) {
                in_comment = true;
            }
        }
        std::size_t last = content_end;
        while (last > line_start && (text[last - 1] == ' ' || text[last - 1] == '\t')) {
            --last;
        }
        if (!in_string && !in_comment && has_newline && last > line_start &&
            text[last - 1] == '_' &&
            (last - 1 == line_start || text[last - 2] == ' ' || text[last - 2] == '\t')) {
            text[last - 1] = ' ';
            if (joined != nullptr) joined->push_back(line_end);
            text[line_end] = ' ';
            if (line_end > line_start && text[line_end - 1] == '\r') {
                text[line_end - 1] = ' ';
            }
        }
        line_start = has_newline ? line_end + 1 : text.size();
    }
}

// REQ-0240: conditional compilation. Returns `source` with every
// `#If/#ElseIf/#Else/#End If/#Const` directive line, and every line inside
// an inactive branch, blanked to spaces (line breaks kept, so byte offsets
// are unchanged). `error`/`error_offset` are set on malformed directives.
class ConditionalPreprocessor final {
public:
    [[nodiscard]] std::optional<std::string> run(
        const std::string_view source, std::string& error, std::size_t& error_offset) {
        std::string out(source);
        struct Frame {
            bool parent_active{};
            bool taken{};
            bool active{};
        };
        std::vector<Frame> stack;
        const auto active_now = [&] { return stack.empty() || stack.back().active; };
        std::size_t position = 0;
        while (position < out.size()) {
            std::size_t line_end = out.find('\n', position);
            if (line_end == std::string::npos) {
                line_end = out.size();
            }
            std::size_t i = position;
            while (i < line_end && (out[i] == ' ' || out[i] == '\t')) ++i;
            const bool directive = i < line_end && out[i] == '#' &&
                i + 1 < line_end && std::isalpha(static_cast<unsigned char>(out[i + 1])) != 0;
            if (directive) {
                std::string text = out.substr(i + 1, line_end - i - 1);
                if (!text.empty() && text.back() == '\r') text.pop_back();
                if (const auto c = text.find('\''); c != std::string::npos) text.resize(c);
                std::string lower;
                for (const char ch : text) lower.push_back(ascii_lower(ch));
                const auto starts = [&](const std::string_view w) {
                    return lower.compare(0, w.size(), w) == 0 &&
                        (lower.size() == w.size() || !is_identifier_part(lower[w.size()]));
                };
                const auto condition_of = [&](std::size_t skip) -> std::optional<bool> {
                    std::string expr = text.substr(skip);
                    std::string lexpr = lower.substr(skip);
                    const auto then = lexpr.rfind("then");
                    if (then == std::string::npos) {
                        error = "expected Then";
                        error_offset = position;
                        return std::nullopt;
                    }
                    expr.resize(then);
                    const auto value = evaluate_expression(expr, error);
                    if (!value.has_value()) {
                        error_offset = position;
                        return std::nullopt;
                    }
                    return *value != 0;
                };
                if (starts("if")) {
                    const bool parent = active_now();
                    const auto condition = parent ? condition_of(2) : std::optional<bool>{false};
                    if (!condition.has_value()) return std::nullopt;
                    stack.push_back({parent, *condition, parent && *condition});
                } else if (starts("elseif")) {
                    if (stack.empty()) { error = "#ElseIf without #If"; error_offset = position; return std::nullopt; }
                    auto& frame = stack.back();
                    if (frame.parent_active && !frame.taken) {
                        const auto condition = condition_of(6);
                        if (!condition.has_value()) return std::nullopt;
                        frame.active = *condition;
                        frame.taken = *condition;
                    } else {
                        frame.active = false;
                    }
                } else if (starts("else")) {
                    if (stack.empty()) { error = "#Else without #If"; error_offset = position; return std::nullopt; }
                    auto& frame = stack.back();
                    frame.active = frame.parent_active && !frame.taken;
                    frame.taken = true;
                } else if (starts("end")) {
                    if (stack.empty()) { error = "#End If without #If"; error_offset = position; return std::nullopt; }
                    stack.pop_back();
                } else if (starts("const")) {
                    if (active_now()) {
                        const auto equals = text.find('=', 5);
                        if (equals == std::string::npos) {
                            error = "expected = in #Const"; error_offset = position; return std::nullopt;
                        }
                        std::string name;
                        for (std::size_t k = 5; k < equals; ++k) {
                            if (text[k] != ' ' && text[k] != '\t') name.push_back(ascii_lower(text[k]));
                        }
                        const auto value = evaluate_expression(text.substr(equals + 1), error);
                        if (!value.has_value() || name.empty()) {
                            if (name.empty()) error = "expected #Const name";
                            error_offset = position;
                            return std::nullopt;
                        }
                        constants_[name] = *value;
                    }
                } else {
                    error = "unknown conditional compilation directive";
                    error_offset = position;
                    return std::nullopt;
                }
                for (std::size_t k = position; k < line_end; ++k) {
                    if (out[k] != '\r') out[k] = ' ';
                }
            } else if (!active_now()) {
                for (std::size_t k = position; k < line_end; ++k) {
                    if (out[k] != '\r') out[k] = ' ';
                }
            }
            position = line_end + 1;
        }
        if (!stack.empty()) {
            error = "missing #End If";
            error_offset = out.size();
            return std::nullopt;
        }
        return out;
    }

private:
    std::unordered_map<std::string, long long> constants_{
        {"win32", -1}, {"win16", 0}, {"mac", 0}, {"vba6", -1}, {"vba7", -1}, {"vbaver", 6}};

    // Recursive-descent evaluation over: Or/Xor < And < Not < comparison <
    // + - < * / Mod < unary minus < primary.
    struct Parser {
        ConditionalPreprocessor& owner;
        std::string text;
        std::size_t i{};
        std::string& error;
        void ws() { while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) ++i; }
        bool word(const std::string_view w) {
            ws();
            if (text.size() - i < w.size()) return false;
            for (std::size_t k = 0; k < w.size(); ++k)
                if (ascii_lower(text[i + k]) != w[k]) return false;
            if (i + w.size() < text.size() && is_identifier_part(text[i + w.size()])) return false;
            i += w.size();
            return true;
        }
        std::optional<long long> or_expr() {
            auto left = and_expr();
            while (left) {
                if (word("or")) { auto r = and_expr(); if (!r) return r; left = *left | *r; }
                else if (word("xor")) { auto r = and_expr(); if (!r) return r; left = *left ^ *r; }
                else break;
            }
            return left;
        }
        std::optional<long long> and_expr() {
            auto left = not_expr();
            while (left && word("and")) {
                auto r = not_expr(); if (!r) return r; left = *left & *r;
            }
            return left;
        }
        std::optional<long long> not_expr() {
            if (word("not")) { auto r = not_expr(); if (!r) return r; return ~*r; }
            return cmp_expr();
        }
        std::optional<long long> cmp_expr() {
            auto left = add_expr();
            if (!left) return left;
            ws();
            const char a = i < text.size() ? text[i] : '\0';
            const char b = i + 1 < text.size() ? text[i + 1] : '\0';
            int op = 0;  // 1 = 2 <> 3 < 4 > 5 <= 6 >=
            if (a == '<' && b == '>') { op = 2; i += 2; }
            else if (a == '<' && b == '=') { op = 5; i += 2; }
            else if (a == '>' && b == '=') { op = 6; i += 2; }
            else if (a == '=') { op = 1; ++i; }
            else if (a == '<') { op = 3; ++i; }
            else if (a == '>') { op = 4; ++i; }
            if (op == 0) return left;
            auto right = add_expr();
            if (!right) return right;
            const long long l = *left, r = *right;
            const bool result = op == 1 ? l == r : op == 2 ? l != r : op == 3 ? l < r
                              : op == 4 ? l > r : op == 5 ? l <= r : l >= r;
            return result ? -1 : 0;
        }
        std::optional<long long> add_expr() {
            auto left = mul_expr();
            while (left) {
                ws();
                if (i < text.size() && (text[i] == '+' || text[i] == '-')) {
                    const char op = text[i++];
                    auto r = mul_expr(); if (!r) return r;
                    left = op == '+' ? *left + *r : *left - *r;
                } else break;
            }
            return left;
        }
        std::optional<long long> mul_expr() {
            auto left = unary();
            while (left) {
                ws();
                if (i < text.size() && text[i] == '*') {
                    ++i; auto r = unary(); if (!r) return r; left = *left * *r;
                } else if (word("mod")) {
                    auto r = unary(); if (!r) return r;
                    if (*r == 0) { error = "division by zero"; return std::nullopt; }
                    left = *left % *r;
                } else break;
            }
            return left;
        }
        std::optional<long long> unary() {
            ws();
            if (i < text.size() && text[i] == '-') { ++i; auto r = unary(); if (!r) return r; return -*r; }
            return primary();
        }
        std::optional<long long> primary() {
            ws();
            if (i >= text.size()) { error = "expected expression"; return std::nullopt; }
            if (text[i] == '(') {
                ++i; auto r = or_expr(); if (!r) return r; ws();
                if (i >= text.size() || text[i] != ')') { error = "expected closing parenthesis"; return std::nullopt; }
                ++i; return r;
            }
            if (std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
                long long v = 0;
                while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])) != 0)
                    v = v * 10 + (text[i++] - '0');
                return v;
            }
            if (is_identifier_start(text[i])) {
                std::string name;
                while (i < text.size() && is_identifier_part(text[i])) name.push_back(ascii_lower(text[i++]));
                if (name == "true") return -1;
                if (name == "false") return 0;
                const auto it = owner.constants_.find(name);
                return it == owner.constants_.end() ? 0 : it->second;
            }
            error = "expected expression";
            return std::nullopt;
        }
    };

    std::optional<long long> evaluate_expression(const std::string& text, std::string& error) {
        Parser parser{*this, text, 0, error};
        auto value = parser.or_expr();
        if (!value) return value;
        parser.ws();
        if (parser.i != text.size()) {
            error = "unexpected text in conditional expression";
            return std::nullopt;
        }
        return value;
    }
};

[[nodiscard]] wfc::Evaluation failure_for_directive(
    const std::string& message, const std::size_t offset) {
    return failure("WFC0310", message, offset);
}

}  // namespace

namespace wfc {

namespace {

// Deep VB recursion needs far more native stack than a default main thread
// has (1 MB on Windows), so the interpreter runs on a thread with a large
// reserved (not committed) stack. `depth_limit` is the procedure nesting the
// stack is sized for; 0 means the thread could not be created and the work
// ran inline.
constexpr std::size_t kLargeStackBytes =
    sizeof(void*) >= 8U ? (std::size_t{512} << 20U) : (std::size_t{160} << 20U);
constexpr std::size_t kLargeStackDepth = sizeof(void*) >= 8U ? 5000U : 1500U;

struct LargeStackTask {
    std::function<void()> work;
    std::exception_ptr failure;
};

void run_large_stack_task(void* raw) {
    auto& task = *static_cast<LargeStackTask*>(raw);
    try {
        task.work();
    } catch (...) {
        task.failure = std::current_exception();
    }
}

// Runs `work` on a large-stack thread; returns false if no such thread could
// be started (the caller then runs `work` itself with the small limit).
[[nodiscard]] bool run_on_large_stack(std::function<void()> work) {
    LargeStackTask task{std::move(work), nullptr};
    if (!detail::run_on_thread_with_stack(&run_large_stack_task, &task, kLargeStackBytes)) {
        return false;
    }
    if (task.failure) {
        std::rethrow_exception(task.failure);
    }
    return true;
}

// Evaluates with a deep-recursion limit matching the stack it runs on.
[[nodiscard]] Evaluation evaluate_interpreter(
    const std::function<Evaluation(std::size_t, const char*, std::size_t)>& run) {
    Evaluation result;
    const bool ran = run_on_large_stack([&] {
        char stack_top = 0;
        result = run(kLargeStackDepth, &stack_top, kLargeStackBytes - (std::size_t{24} << 20U));
    });
    if (!ran) {
        result = run(64U, nullptr, 0U);
    }
    return result;
}

}  // namespace

// Fills in `error_line`/`error_column` from the failing module's processed text (line breaks
// removed by continuation joining are counted back in).
void attach_position(
    Evaluation& result, const std::string_view text, const std::vector<std::size_t>& joined,
    std::string module) {
    if (result.success) {
        return;
    }
    const auto offset = std::min(result.error_offset, text.size());
    std::size_t line = 1;
    std::size_t line_start = 0;
    for (std::size_t i = 0; i < offset; ++i) {
        if (text[i] == '\n') {
            ++line;
            line_start = i + 1;
        }
    }
    for (const auto position : joined) {
        if (position < offset) {
            ++line;
            line_start = std::max(line_start, position + 1);
        }
    }
    result.error_line = line;
    result.error_column = offset - line_start + 1;
    result.error_module = std::move(module);
}

Evaluation evaluate_program(const std::string_view source) {
    std::string error;
    std::size_t error_offset{};
    auto processed = ConditionalPreprocessor{}.run(source, error, error_offset);
    if (!processed.has_value()) {
        return failure_for_directive(error, error_offset);
    }
    rewrite_object_aliases(*processed);
    rewrite_bracketed_identifiers(*processed);
    std::vector<std::size_t> joined;
    join_line_continuations(*processed, &joined);
    return evaluate_interpreter([&](const std::size_t depth, const char* base,
                                    const std::size_t budget) {
        Interpreter interpreter(*processed);
        interpreter.set_max_procedure_depth(depth);
        interpreter.set_stack_budget(base, budget);
        auto result = interpreter.evaluate();
        if (interpreter.failing_module_name().empty()) {
            attach_position(result, *processed, joined, {});
        }
        return result;
    });
}

Evaluation evaluate_program(
    const std::string_view source, const std::vector<ClassModuleSource>& classes) {
    return evaluate_program(source, classes, EvaluationOptions{});
}

Evaluation evaluate_program(
    const std::string_view source, const std::vector<ClassModuleSource>& classes,
    const EvaluationOptions& options) {
    std::string error;
    std::size_t error_offset{};
    auto processed = ConditionalPreprocessor{}.run(source, error, error_offset);
    if (!processed.has_value()) {
        return failure_for_directive(error, error_offset);
    }
    rewrite_object_aliases(*processed);
    rewrite_bracketed_identifiers(*processed);
    std::vector<std::size_t> joined;
    join_line_continuations(*processed, &joined);
    std::vector<std::string> processed_classes;
    std::vector<std::vector<std::size_t>> class_joined(classes.size());
    processed_classes.reserve(classes.size());
    for (std::size_t index = 0; index < classes.size(); ++index) {
        auto text = ConditionalPreprocessor{}.run(classes[index].source, error, error_offset);
        if (!text.has_value()) {
            return failure_for_directive(error, error_offset);
        }
        rewrite_object_aliases(*text);
        rewrite_bracketed_identifiers(*text);
        join_line_continuations(*text, &class_joined[index]);
        processed_classes.push_back(std::move(*text));
    }
    std::vector<ClassModuleSource> rewritten;
    rewritten.reserve(classes.size());
    for (std::size_t index = 0; index < classes.size(); ++index) {
        rewritten.push_back({classes[index].name, processed_classes[index]});
    }
    return evaluate_interpreter([&](const std::size_t depth, const char* base,
                                    const std::size_t budget) {
        Interpreter interpreter(*processed, rewritten);
        interpreter.set_vb_number_spacing(options.vb6_print_spacing);
        interpreter.set_max_procedure_depth(depth);
        interpreter.set_stack_budget(base, budget);
        auto result = interpreter.evaluate();
        const auto module = interpreter.failing_module_name();
        if (module.empty()) {
            attach_position(result, *processed, joined, {});
        } else {
            for (std::size_t index = 0; index < rewritten.size(); ++index) {
                std::string lowered_a = module;
                std::string lowered_b = rewritten[index].name;
                for (auto& ch : lowered_a) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                for (auto& ch : lowered_b) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                if (lowered_a == lowered_b) {
                    attach_position(result, processed_classes[index], class_joined[index], rewritten[index].name);
                    break;
                }
            }
        }
        return result;
    });
}

Evaluation evaluate_print_statement(const std::string_view source) {
    std::size_t offset{};
    while (offset < source.size() &&
           std::isspace(static_cast<unsigned char>(source[offset])) != 0) {
        ++offset;
    }
    const auto statement_offset = offset;
    constexpr std::string_view keyword = "print";
    for (const char expected : keyword) {
        if (offset == source.size() || ascii_lower(source[offset]) != expected) {
            return failure("WFC0001", "expected Print statement", statement_offset);
        }
        ++offset;
    }
    if (offset < source.size() && is_identifier_part(source[offset])) {
        return failure("WFC0001", "expected Print statement", statement_offset);
    }
    return Interpreter(source, false).evaluate();
}

}  // namespace wfc
