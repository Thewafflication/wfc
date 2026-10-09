// The VB Value variant and its Empty/Null/Nothing/array/object alternatives.
// Internal to the WFC evaluator; not part of the public API.
// Split out of src/evaluator.cpp; see src/interpreter/README.md.

#ifndef WFC_INTERPRETER_VB_VALUE_HPP
#define WFC_INTERPRETER_VB_VALUE_HPP

#include "vb_numeric.hpp"

namespace wfc::detail {

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
[[nodiscard]] inline NumericStringResult parse_numeric_string(const std::string_view text) {
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

}  // namespace wfc::detail

#endif  // WFC_INTERPRETER_VB_VALUE_HPP
