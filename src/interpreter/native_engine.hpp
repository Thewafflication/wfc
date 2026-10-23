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
#include "wfc/evaluator.hpp"

namespace wfc::detail {

class Interpreter;

// A run-time error the engine reports: the Visual Basic number and text.
struct NativeError final {
    int number{};
    std::string description;
};

// What a project gives the engine: the forms it loads and the options of the
// run (the form code is compiled, so the classes carry design data only).
struct EngineSetup final {
    std::vector<wfc::ClassModuleSource> classes;
    wfc::EvaluationOptions options;
    std::string scan_text;  // the compiled program's source
    std::string program;    // what the engine runs itself: the Declares
};

class NativeEngine final {
public:
    NativeEngine();
    ~NativeEngine();
    NativeEngine(const NativeEngine&) = delete;
    NativeEngine& operator=(const NativeEngine&) = delete;

    // `left op right` for + - * / \ mod & ^ = <> < > <= >= and or xor eqv imp.
    // nullopt means an error (filled in).
    // `left_variant` and `right_variant` say an operand is a Variant (a number
    // or a string held in a Variant compares by other rules than a typed one).
    [[nodiscard]] std::optional<Value> binary(
        std::string_view op, const Value& left, const Value& right,
        bool left_variant, bool right_variant, NativeError& error);
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
    // Runs one statement of the interpreter with values for wfcarg0, wfcarg1,
    // ...; the variables come back in `values`. False means an error.
    [[nodiscard]] bool run_statement(const std::string& text,
                                     std::vector<Value>& values,
                                     std::uint32_t variant_mask,
                                     NativeError& error);
    // Loads the forms of the project into the engine.
    [[nodiscard]] bool start(EngineSetup setup, NativeError& error);
    // A global object of the engine (a form by its predeclared name, Forms).
    [[nodiscard]] std::optional<Value> global(const std::string& name,
                                              NativeError& error);
    // `object.name(arguments)` read, called, and assigned (`Let` or `Set`).
    // By-reference arguments come back changed in `arguments`.
    [[nodiscard]] std::optional<Value> member_get(const Value& object,
                                                  const std::string& name,
                                                  std::vector<Value>& arguments,
                                                  NativeError& error);
    [[nodiscard]] bool member_put(const Value& object, const std::string& name,
                                  const std::vector<Value>& arguments,
                                  const Value& value, bool is_set,
                                  NativeError& error);
    // A new instance of a form of the project.
    [[nodiscard]] std::optional<Value> new_form(const std::string& name,
                                                NativeError& error);
    // The items `For Each` visits in an object.
    [[nodiscard]] std::optional<std::vector<Value>> items(const Value& object,
                                                          NativeError& error);
    // Shows the startup form and runs the events until the forms are gone.
    [[nodiscard]] bool run_forms(NativeError& error);
    // Whether two Values are the same object (or both Nothing).
    [[nodiscard]] static bool same_object(const Value& a, const Value& b);
    // What the interpreter has printed since the last call (the output of
    // forms and the like); `line_open` says a line is still open.
    [[nodiscard]] std::string take_output(bool& line_open);
    // The line that was open has been ended by the caller.
    void close_output_line();
    // `text Like pattern`.
    [[nodiscard]] static bool like(const std::string& text,
                                   const std::string& pattern);
    // `Randomize [seed]`.
    void randomize(bool has_seed, double seed, bool is_timer);
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
