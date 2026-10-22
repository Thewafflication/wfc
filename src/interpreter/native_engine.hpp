// The value-level face of the interpreter for native programs (ADR-0009):
// compiled code keeps typed values unboxed, and a Variant is a handle to a box
// holding a `Value`. The operators, comparisons, conversions and formatting of
// those Values are the interpreter's own, reached through this class so that
// there is one implementation of the semantics of Visual Basic.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "vb_value.hpp"

namespace wfc::detail {

class Interpreter;

// A run-time error the engine reports: the Visual Basic number and text.
struct NativeError final {
    int number{};
    std::string description;
};

class NativeEngine final {
public:
    NativeEngine();
    ~NativeEngine();
    NativeEngine(const NativeEngine&) = delete;
    NativeEngine& operator=(const NativeEngine&) = delete;

    // `left op right` for + - * / \ mod & ^ = <> < > <= >= and or xor eqv imp.
    // nullopt means an error (filled in).
    [[nodiscard]] std::optional<Value> binary(std::string_view op,
                                              const Value& left,
                                              const Value& right,
                                              NativeError& error);
    [[nodiscard]] std::optional<Value> negate(const Value& value,
                                              NativeError& error);
    [[nodiscard]] std::optional<Value> logical_not(const Value& value,
                                                   NativeError& error);

    // Conversions of a Value to a typed value, as an assignment would.
    [[nodiscard]] std::optional<std::int32_t> to_long(const Value& value,
                                                      NativeError& error);
    [[nodiscard]] std::optional<double> to_double(const Value& value,
                                                  NativeError& error);
    [[nodiscard]] std::optional<std::string> to_text(const Value& value,
                                                     NativeError& error);
    [[nodiscard]] std::optional<bool> to_boolean(const Value& value,
                                                 NativeError& error);

    // Calls a function of the VBA library (Format, IIf, Now, ...) with
    // evaluated arguments; the name is lower case.
    [[nodiscard]] std::optional<Value> call_function(
        std::string_view name, const std::vector<Value>& arguments,
        NativeError& error);
    // Whether the name is a function of the library.
    [[nodiscard]] static bool is_library_function(std::string_view name);

    // The text `Print` writes for a Value (numbers carry their sign position
    // and trailing space).
    [[nodiscard]] static std::string print_text(const Value& value);
    // `CStr`, `Str`-free rendering of a Value.
    [[nodiscard]] static std::string render(const Value& value);
    [[nodiscard]] std::string type_name(const Value& value);
    [[nodiscard]] static int var_type(const Value& value);

private:
    std::unique_ptr<Interpreter> interpreter_;
};

}  // namespace wfc::detail
