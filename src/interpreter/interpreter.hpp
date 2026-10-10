// The Interpreter class: VB program state, parsing, and execution.
// Internal to the WFC evaluator; not part of the public API.
// Split out of src/evaluator.cpp; see src/interpreter/README.md.

#ifndef WFC_INTERPRETER_INTERPRETER_HPP
#define WFC_INTERPRETER_INTERPRETER_HPP

#include "../large_stack.hpp"
#include "builtin_class_sources.hpp"
#include "vb_text.hpp"
#include "wfc/evaluator.hpp"

namespace wfc::detail {

[[nodiscard]] inline wfc::Evaluation failure(const std::string_view code,
                                             const std::string_view message,
                                             const std::size_t offset) {
    wfc::Evaluation result;
    result.diagnostic = std::string(code) + " at byte " +
                        std::to_string(offset + 1) + ": " +
                        std::string(message);
    result.error_offset = offset;
    return result;
}

// One variable-declaration scope: either the single module-level scope, or
// one procedure call's local scope (its parameters and locally Dim'd
// variables). A procedure call's variable lookups see only its own scope
// and the module scope -- never an enclosing caller's locals -- matching
// VB6's own two-level (module/procedure) scoping, which has no nested
// block scope.
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
    // `Static Sub|Function|Property`: every local variable keeps its value
    // between calls.
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
    // The lower-cased export a `Declare ... Alias "name"` binds to; empty when
    // the routine is declared under its own name.
    std::string external_name;
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
// Native key/value storage behind the built-in Collection and Dictionary
// classes (reached through the hidden `WfcStore` function): ordered entries
// plus a hash index of the keys.
struct NativeStore {
    std::vector<Value> keys;
    std::vector<Value> values;
    std::unordered_map<std::string, std::size_t>
        index;  // canonical key -> 0-based position
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
    std::vector<std::pair<std::weak_ptr<InstanceData>, std::string>>
        event_sinks;
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

    explicit Interpreter(const std::string_view source,
                         const bool allow_identifiers = true)
        : source_(source),
          main_source_(source),
          allow_identifiers_(allow_identifiers),
          main_source_data_(source.data()) {}

    Interpreter(const std::string_view source,
                std::vector<wfc::ClassModuleSource> classes,
                const bool allow_identifiers = true)
        : source_(source),
          main_source_(source),
          allow_identifiers_(allow_identifiers),
          main_source_data_(source.data()),
          class_sources_(std::move(classes)) {}

    [[nodiscard]] Scope& current_scope() noexcept;
    [[nodiscard]] Scope& module_scope() noexcept;
    [[nodiscard]] bool in_procedure() const noexcept;

    // Looks `name` up in the current procedure's local scope (if any),
    // falling back to the module scope -- or, while executing inside a
    // class member (`instance_scopes_` non-empty), that instance's own
    // field scope instead. A class member never falls through to the real
    // module scope: each class is its own isolated module, matching VB6's
    // module-to-module isolation (a class does not implicitly see a
    // standard module's variables, and vice versa). Neither level ever sees
    // an enclosing caller's locals, matching VB6's module/procedure
    // two-level scoping.
    [[nodiscard]] VariableLookup find_variable(const std::string& name);

    [[nodiscard]] VariableLookup find_variable_raw(const std::string& name);

    // REQ-0237: collects `[Public|Private] Enum Name` type names up front so
    // `As Name` resolves even in procedures scanned before the Enum runs.
    [[nodiscard]] static bool in_with_identifier(const std::string& name);

    // REQ-0265: `Option Explicit` anywhere in the program (main source or a
    // class module) makes undeclared names an error everywhere.
    void scan_option_explicit();

    // `DefInt A-C, X` ... `DefVar`: the default type for undeclared names by
    // first letter. Scanned up front from every module (one program-wide
    // table; a per-module table is not modeled).
    static constexpr std::size_t no_default_type = static_cast<std::size_t>(-1);
    std::array<std::size_t, 26> default_types_ = [] {
        std::array<std::size_t, 26> table{};
        table.fill(static_cast<std::size_t>(-1));
        return table;
    }();

    [[nodiscard]] std::optional<std::size_t> default_type_for(
        const std::string& name) const;

    // A Function/Property Get declared without `As Type`: Variant, or the
    // DefXxx type for its first letter.
    void apply_implicit_return_type(ProcedureDef& definition,
                                    const std::string& name,
                                    const char suffix = '\0') const;

    // Parses one `DefXxx letters` line starting at `text[position]`; returns
    // false if the line is not a Def statement.
    bool apply_deftype_line(const std::string_view line);

    void scan_deftypes();

    void scan_module_names();
    // `Attribute VB_PredeclaredId = True` classes get a global default
    // instance named after the class.
    void register_predeclared_instances();

    void scan_enum_names();

    void scan_enum_names_in(const std::string_view text);

    void set_max_procedure_depth(const std::size_t depth) noexcept;

    // Native stack accounting for deep recursion: `base` is an address near
    // the top of the (downward-growing) stack, `budget` the bytes that may be
    // used before a call reports "Out of stack space".
    void set_stack_budget(const char* const base,
                          const std::size_t budget) noexcept;

    [[nodiscard]] bool stack_nearly_exhausted() const noexcept;

    [[nodiscard]] wfc::Evaluation evaluate();

    // The class module (display name) whose source was executing when the last
    // error was raised; empty for the standard module(s).
    void set_vb_number_spacing(const bool enabled) noexcept;
    void set_app_properties(std::map<std::string, std::string> properties);

    [[nodiscard]] std::string failing_module_name() const;

    [[nodiscard]] wfc::Evaluation evaluate_program_text();

private:
    [[nodiscard]] bool at_end() const noexcept;
    [[nodiscard]] char current() const noexcept;
    [[nodiscard]] char peek(const std::size_t ahead) const noexcept;
    void advance() noexcept;

    void skip_horizontal_whitespace() noexcept;

    [[nodiscard]] bool consume_line_break() noexcept;

    // Skips to the end of the current statement (a ':' or line end outside a
    // string).
    void skip_comment_free_statement_text() noexcept;

    void skip_comment() noexcept;

    void skip_program_leading_trivia() noexcept;

    // Advances past the rest of the current logical line (its content is
    // not this call's concern) and the line break that ends it.
    void skip_rest_of_line() noexcept;

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
    [[nodiscard]] bool skip_to_matching_end(const std::string_view keyword,
                                            std::size_t& body_end);

    // Parses `(` [`ByVal`|`ByRef`] name [`As` Type] {`,` ...} `)` into
    // `definition.parameters`, used by `scan_procedures` for both `Sub` and
    // `Function` declarations.
    [[nodiscard]] bool scan_procedure_parameters(ProcedureDef& definition);

    // Returns whether `name` is already used anywhere in `class_def` (as a
    // field, a method, or any Property accessor), regardless of kind --
    // used to reject a field or method name that collides with any existing
    // member. A Property accessor's own duplicate check is narrower (see
    // scan_class_body): Get/Let/Set of the *same* property name are meant
    // to coexist.
    [[nodiscard]] static bool class_member_name_used(const ClassDef& class_def,
                                                     const std::string& name);

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
    [[nodiscard]] bool scan_class_body(ClassDef& class_def);

    // `[Dim|Public|Private] name [As Type]` -- the keyword itself has
    // already been consumed by scan_class_body, which also determined
    // `is_private`.
    // `Private a As Long, b As String` declares several fields on one line.
    [[nodiscard]] bool scan_class_field_declaration(ClassDef& class_def,
                                                    const bool is_private);

    [[nodiscard]] bool scan_class_field_declarator(ClassDef& class_def,
                                                   const bool is_private,
                                                   bool& more);

    // `Property Get|Let|Set name(...) [As Type] ... End Property` -- the
    // `Property` keyword has already been consumed by scan_class_body,
    // which also determined `is_private`.
    [[nodiscard]] bool scan_class_property_declaration(
        ClassDef& class_def, const bool is_private,
        const std::size_t line_offset);

    // `Sub|Function name(...) [As Type] ... End Sub|Function` -- the
    // `Sub`/`Function` keyword has already been consumed by scan_class_body,
    // which also determined `is_private`.
    [[nodiscard]] bool scan_class_procedure_declaration(
        ClassDef& class_def, const bool is_function, const bool is_private,
        const std::size_t line_offset);

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
    [[nodiscard]] Value copy_if_udt(Value value);

    [[nodiscard]] bool is_udt_class(const std::string& class_name) const;

    // Deep-copies a UDT instance's fields (nested UDT fields get their own
    // fresh copies, matching value semantics).
    [[nodiscard]] std::shared_ptr<InstanceData> clone_udt(
        const InstanceData& source);

    // Gives every UDT instance inside `value` (itself, or the elements of an
    // array) its own copy, so value semantics hold for arrays of UDTs.
    void deep_copy_udt_values(Value& value);

    // `target = source` where both are UDT instances of the same type.
    [[nodiscard]] bool assign_udt(Value& target, const Value& source,
                                  const std::size_t offset);

    // Finds `[Public|Private] Type Name ... End Type` blocks in the main
    // source and registers each as a value-semantics class (REQ-0241).
    void scan_udt_types();

    // REQ-0243: registers the built-in Collection when the program mentions
    // it and does not define its own class of that name.
    void scan_builtin_classes();

    // REQ-0257: finds `Attribute Name.VB_UserMemId = 0` in a class source.
    static void scan_default_member(ClassDef& class_def);

    [[nodiscard]] bool scan_classes();

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
    [[nodiscard]] bool scan_procedures();

    [[nodiscard]] bool consume_statement_end();

    // `Case x: statement` -- a colon may end the Case line (REQ-0252).
    [[nodiscard]] bool consume_case_line_end();

    // A loop header may be followed by `:` and the first body statement on
    // the same line (`For i = 1 To 3: Print i: Next`).
    [[nodiscard]] bool consume_loop_header_end();

    [[nodiscard]] bool consume_block_line_end();

    [[nodiscard]] bool consume(const char character) noexcept;

    [[nodiscard]] bool consume_keyword(const std::string_view keyword);

    [[nodiscard]] bool consume_keyword_slow(const std::string_view keyword);

    [[nodiscard]] bool at_with_member() const noexcept;

    [[nodiscard]] std::optional<std::string> parse_identifier(
        char* const type_character = nullptr);

    [[nodiscard]] bool validate_type_character(
        const char type_character, const std::size_t identifier_offset);

    [[nodiscard]] std::size_t type_character_index(
        const char type_character) const;

    // Recognizes one `As Type` keyword (Integer/Long/Double/Single/
    // Currency/String/Boolean/Variant) for a procedure parameter or
    // function return type, mirroring the type keywords `Dim` already
    // accepts. `Object` and array element types are deliberately not
    // included here (see REQ-0202's Scope).
    struct TypeKeywordResult {
        Value default_value;
        bool is_variant{};
    };
    [[nodiscard]] std::optional<TypeKeywordResult> parse_type_keyword();

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
    [[nodiscard]] std::optional<ResolvedType>
    parse_scalar_object_or_class_type();

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
        const ResolvedType& type_result, const std::size_t type_offset);

    [[nodiscard]] bool type_character_matches(
        const Value& value, const char type_character,
        const std::size_t identifier_offset);

    // An object reference used where a value is expected stands for its class's
    // default member (`Attribute Name.VB_UserMemId = 0`): `s = obj`, `"x" &
    // obj`, `Print obj`. Replaces `value` with that member's result; leaves it
    // unchanged (returning true) when it is not an instance of a class with a
    // default member. Returns false only after the default member itself
    // failed.
    [[nodiscard]] bool resolve_default_value(Value& value,
                                             const std::size_t offset);

    // Attempt Integer/Long/Single/Double/Currency widening or checked
    // narrowing so `value` matches `target_index`. Leaves `value` unchanged,
    // and returns true, when no numeric conversion applies (including when it
    // already matches) -- the caller still compares `value->index()` against
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
        Value& value, const std::size_t target_index, const std::size_t offset);

    [[nodiscard]] bool coerce_numeric_value(Value& value,
                                            const std::size_t target_index,
                                            const std::size_t offset);

    [[nodiscard]] static std::string vb_error_description(const Integer number);

    // The VB error number a failed statement's diagnostic stands for, or 0
    // when it is not a catchable runtime error (a syntax/semantic error).
    [[nodiscard]] Integer runtime_error_number() const;

    // Moves offset_ to the end of the current statement (a ':' or line
    // break outside a string literal), without consuming it.
    void skip_to_statement_end() noexcept;

    // REQ-0238: applies the frame's `On Error` mode to a statement that just
    // failed with a runtime error. Returns true when the error was absorbed
    // (Resume Next) or converted into a pending jump to the handler label.
    [[nodiscard]] bool recover_runtime_error(const std::size_t statement_start);

    [[nodiscard]] bool parse_statement();

    [[nodiscard]] bool parse_statement_once();

    // After a statement sequence failed with a pending jump, transfers
    // control to the jump target. Returns false when no jump is pending.
    [[nodiscard]] bool take_pending_jump();

    // Offset of the `label:` line inside the current procedure (or the
    // whole program at module level), or npos.
    [[nodiscard]] std::size_t find_label(const std::string& label) const;

    // A label operand: an identifier or a line number.
    [[nodiscard]] std::optional<std::string> parse_label_name();

    [[nodiscard]] bool in_procedure_body() const noexcept;

    // `On Error ...`, `Resume ...`, `GoTo label`, `label:` and `Err.Raise`/
    // `Err.Clear` (REQ-0238). Returns nullopt when the statement is none of
    // these (offset_ unchanged).
    [[nodiscard]] std::optional<bool> parse_error_handling_statement(
        const std::size_t statement_offset, const bool allow_label = true);

    [[nodiscard]] bool parse_statement_core();

    // `[Let] name ...`: an assignment, array/member write, or a bare call.
    [[nodiscard]] bool parse_identifier_statement(
        const std::size_t statement_offset);

    // ---- File I/O (REQ-0245) ---------------------------------------------

    struct OpenFile {
        std::FILE* handle{};
        int mode{};  // 1 = Input, 2 = Output, 3 = Append, 4 = Binary, 5 =
                     // Random
        long record_length{128};
        std::string path;  // normalized, for the already-open check
        bool shared{};     // opened with the Shared clause
    };

    [[nodiscard]] static std::FILE* open_file(const std::string& path,
                                              const char* mode);

    [[nodiscard]] static std::string environment_variable(
        const std::string& name);

    [[nodiscard]] bool raise_runtime(const Integer number,
                                     const std::string& description,
                                     const std::size_t offset);

    [[nodiscard]] OpenFile* find_open_file(const Integer number,
                                           const std::size_t offset);

    [[nodiscard]] bool write_to_file(const Integer number,
                                     const std::string& text,
                                     const std::size_t offset);

    // Reads one line (without the terminator) into `line`; false at EOF.
    [[nodiscard]] static bool read_file_line(std::FILE* handle,
                                             std::string& line);

    [[nodiscard]] static bool file_at_eof(std::FILE* handle);

    // Reads one `Input #` field: a quoted string or a run up to ',' / newline.
    [[nodiscard]] static bool read_input_field(std::FILE* handle,
                                               std::string& token,
                                               bool& quoted);

    [[nodiscard]] bool store_input_token(Value& target, const bool is_variant,
                                         const std::string& token,
                                         const bool quoted,
                                         const std::size_t offset);

    [[nodiscard]] std::string write_item_text(const Value& value);

    [[nodiscard]] bool parse_hash_file_number(Integer& number);

    // `Mid[$](var, start[, length]) = expression` (REQ-0246): overwrites part
    // of a String variable in place, never changing its length.
    [[nodiscard]] std::optional<bool> parse_mid_statement(
        const std::size_t statement_offset);

    // Positions a Binary/Random file for `Get`/`Put`/`Seek` (1-based
    // `position`).
    static void seek_record(OpenFile& file, const long position);

    // `Get|Put [#]n, [position], variable` and `Seek [#]n, position`.
    struct LValue {
        Value* ptr{};
        std::size_t fixed{};
        bool variant{};  // a Variant variable: Get/Put carry a type tag
    };

    // `name`, `name(i, ...)`, `.field` chains: a storage location for Get.
    [[nodiscard]] bool parse_lvalue_path(LValue& result);

    [[nodiscard]] std::optional<bool> parse_binary_statement(
        const std::size_t statement_offset);

    [[nodiscard]] std::optional<bool> parse_file_statement(
        const std::size_t statement_offset);

    [[nodiscard]] static bool is_file_function_name(
        const std::string_view name);

    [[nodiscard]] std::optional<Value> evaluate_file_function(
        const std::string_view name, std::vector<Value>& arguments,
        const std::size_t offset);

    // Consumes `#n,` (file number) when present; `file_number` < 0 means
    // "standard output". A `#` followed by digits and a comma is a file
    // number, anything else (`Print #1/1/2000#`) is an expression.
    [[nodiscard]] bool parse_file_number_prefix(Integer& file_number,
                                                bool& found);

    [[nodiscard]] bool at_statement_end() const noexcept;

    // `Print [#n,] item [; | , item ...] [;]` -- `;` joins, `,` advances to
    // the next 14-column zone, a trailing separator suppresses the newline,
    // `Spc(n)` and `Tab(n)` pad (REQ-0245).
    [[nodiscard]] bool parse_print_statement();

    [[nodiscard]] bool parse_randomize_statement(
        const std::size_t statement_offset);

    [[nodiscard]] bool parse_option_statement(
        const std::size_t statement_offset);

    [[nodiscard]] bool parse_exit_statement(const std::size_t statement_offset);

    [[nodiscard]] bool control_exit_requested() const noexcept;

    [[nodiscard]] bool parse_if_statement();

    // REQ-0271: `If c Then a : b Else c : d` -- colon-separated statements
    // after Then / Else all belong to that branch.
    [[nodiscard]] bool parse_inline_statement_list();

    [[nodiscard]] bool parse_block_if_statement(const bool enclosing_execution,
                                                const bool condition);

    [[nodiscard]] bool parse_while_statement();

    // `Type Name ... End Type` was registered as a class by scan_udt_types
    // (REQ-0241); at run time the block is just skipped.
    [[nodiscard]] bool parse_type_statement_skip(
        const std::size_t statement_offset);

    // `Enum Name` ... `End Enum` (REQ-0237): each member becomes a Long
    // module constant; `As Name` is accepted wherever `As Long` is.
    [[nodiscard]] bool parse_enum_statement(const std::size_t statement_offset);

    // `With expr ... End With` (REQ-0236): the object is held in a hidden
    // variable named "with.N" (un-spellable in source), and parse_identifier
    // yields that name for a leading `.member`.
    [[nodiscard]] bool parse_with_statement(const std::size_t statement_offset);

    // REQ-0252: offset of the line that closes the innermost loop
    // (`Loop`/`Next`/`Wend`) enclosing `from`, scanning forward with simple
    // nesting counts; npos when none is found.
    [[nodiscard]] std::size_t find_loop_end(const std::size_t from) const;

    // A pending `GoTo`/`GoSub`/`Resume` jump whose label lies inside the loop
    // body currently being parsed is taken in place, keeping the block
    // context (`GoTo skip` ... `skip:` ... `Next`).
    [[nodiscard]] bool take_local_jump(const std::size_t body_start,
                                       const std::size_t statement_start);

    [[nodiscard]] bool parse_while_body(std::size_t& continuation_offset);

    [[nodiscard]] bool parse_do_statement();

    [[nodiscard]] bool parse_posttest_do_statement(
        const bool enclosing_execution);

    [[nodiscard]] bool parse_do_body(std::size_t& continuation_offset);

    [[nodiscard]] bool parse_for_statement();

    [[nodiscard]] bool parse_for_body(const std::string_view identifier,
                                      std::size_t& continuation_offset);

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
    [[nodiscard]] bool parse_for_each_statement();

    [[nodiscard]] bool parse_select_statement();

    // Bare `Name [args]` call statement (see the REQ-0217 notes where this is
    // used); nullopt when the statement is not such a call.
    struct ParenGroup {
        bool ends_statement{};  // `( ... )` is balanced and ends the statement
        bool is_list{};         // empty, or has a top-level comma
    };

    [[nodiscard]] ParenGroup scan_statement_paren_group(
        const std::size_t open_offset) const;

    [[nodiscard]] std::optional<bool> parse_bare_call(
        const std::string& identifier, const std::size_t identifier_offset,
        const char type_character, const bool has_let);

    // One statement of a single-line `If` branch; a runtime error in it is
    // handled by the frame's `On Error` mode like any other statement.
    [[nodiscard]] bool parse_inline_statement();

    [[nodiscard]] bool parse_inline_statement_core();

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
    [[nodiscard]] bool parse_static_declaration(
        const std::size_t statement_offset);

    [[nodiscard]] bool parse_single_static_declaration(
        const std::size_t statement_offset);

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
    parse_fixed_array_bounds(const std::size_t identifier_offset);

    // REQ-0248: `Dim a As Long, b As String` -- one declarator at a time.
    [[nodiscard]] bool parse_declaration();

    [[nodiscard]] bool parse_single_declaration();

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
    [[nodiscard]] bool parse_redim_statement();

    [[nodiscard]] bool parse_redim_declarator(const bool preserve);

    // `Erase identifier[, identifier...]` (REQ-0208). For a fixed-size
    // array, resets every element to the declared type's default value
    // (the array stays allocated at its original bounds). For a dynamic
    // array, deallocates it entirely -- as if it had never been `ReDim`'d
    // -- matching real VB6's differing `Erase` behavior for the two array
    // kinds.
    [[nodiscard]] bool parse_erase_statement();

    [[nodiscard]] bool parse_constant_declaration();

    [[nodiscard]] bool parse_single_constant_declaration();

    enum class AppendOutcome { not_applicable, done, failed };

    // `s = s & expr [& expr...]` extends the variable's string in place instead
    // of building and copying a new string each time (quadratic in loops that
    // accumulate text). Used only when the rest of the statement is a plain
    // concatenation of side-effect-free operands.
    [[nodiscard]] bool append_statement_eligible(const std::string& identifier);

    [[nodiscard]] AppendOutcome try_append_assignment(
        const std::string& identifier, std::string& target);

    // Whether the parenthesised group opening at `open_offset` is followed by a
    // '.'.
    [[nodiscard]] bool paren_followed_by_dot(
        const std::size_t open_offset) const noexcept;

    std::size_t chain_counter_{};

    [[nodiscard]] bool parse_assignment(std::string identifier,
                                        const char type_character = '\0');

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
        const std::string& actual_class_name,
        const std::string& declared_class_name) const;

    [[nodiscard]] bool assign_object_reference(
        Value& target, const std::string& declared_class_name, Value source,
        const std::size_t offset);

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
        InstanceData& instance, const ClassDef& class_def,
        const ProcedureDef& definition, const std::string& property_name,
        const std::size_t property_offset);

    // `Set obj.Prop = expression` assigns through a Property Set accessor
    // when the class declares one for `Prop`; otherwise, for a class-typed
    // or `As Object` field with no accessor, directly to that field (see
    // REQ-0205 -- a plain, non-object field has no `Set` target at all,
    // since it is always read/written through ordinary `=`). `base` is the
    // already-evaluated object reference the member is accessed on; its
    // own '.' has already been consumed.
    [[nodiscard]] bool parse_member_set_assignment(
        const Value base, const std::size_t base_offset);

    // `Set identifier = expression` is the only legal way to assign an
    // object reference (see REQ-0200): the target must be a fixed
    // `Object`-typed (or `As ClassName`-typed) variable or a `Variant`-
    // declared one, and the source expression must itself be an object
    // reference -- `Nothing`, or (since REQ-0203) a live `New`-produced
    // instance. A target declared `As ClassName` (as opposed to the generic
    // `As Object`) additionally requires the source instance's own class to
    // match exactly (this evaluator has no class hierarchy/interfaces, so
    // "match" is always exact identity, not a compatible-supertype check).
    [[nodiscard]] bool parse_set_statement();

    // `Set field = x` on an instance's field: like assign_object_reference,
    // but a WithEvents field also moves the instance's event subscription
    // from the old referent to the new one.
    [[nodiscard]] bool assign_field_reference(
        InstanceData& owner, const std::string& field_name, Value& slot,
        const std::string& declared_class_name, Value source,
        const std::size_t offset);

    // `RaiseEvent Name[(args)]`: runs every subscribed `field_Name` handler.
    [[nodiscard]] bool parse_raise_event_statement(
        const std::size_t statement_offset);

    // `identifier.member = expression` writes through a Property Let
    // accessor when the class declares one for `member`, otherwise directly
    // to that instance field. `base` is the already-evaluated object
    // reference the member is accessed on; its own '.' has already been
    // consumed.
    [[nodiscard]] bool parse_member_assignment(
        const Value base, const std::size_t base_offset,
        const std::string& via_interface_class = {});

    // Dispatches a bare `identifier = ...`/`identifier(...) = ...`/
    // `identifier.member = ...` statement to array-element assignment,
    // member assignment, or ordinary scalar assignment, in that order,
    // based on what immediately follows `identifier`.
    [[nodiscard]] bool parse_assignment_or_array_element(
        std::string identifier, const char type_character = '\0');

    // The number of dimensions `array` currently has, or (REQ-0219) will
    // have once its first `ReDim` allocates it. Once allocated,
    // `dimensions` (or its absence, for an ordinary 1-D array) is
    // authoritative; before that, a dynamic array's pre-declared
    // `dynamic_dimension_count` (from a comma-only `Dim`, e.g. `Dim
    // arr(,)`) takes precedence when set, so an index-count check against
    // an as-yet-unallocated multi-dimensional array reports the array's
    // real expected count instead of always assuming 1-D.
    [[nodiscard]] static std::size_t array_expected_dimension_count(
        const ArrayValue& array) noexcept;

    // Parses a comma-separated index-expression list "i1, i2, ..." (the
    // caller has already consumed the opening '('), coercing each to
    // `Long`, and leaves `offset_` just past the matching ')'. Always runs
    // regardless of `execute_` so the parser advances correctly even
    // during a dry-run pass; range-checking and flat-offset computation
    // are the caller's job (via `array_flat_offset`), skipped during a dry
    // run the same way every other runtime check in this evaluator is.
    // Shared by `parse_array_index` (read) and
    // `parse_array_element_assignment` (write). REQ-0210.
    static constexpr std::size_t kAnyDimensionCount =
        static_cast<std::size_t>(-1);

    [[nodiscard]] std::optional<std::vector<std::pair<Integer, std::size_t>>>
    parse_index_list(const std::size_t dimension_count);

    // Computes the flat storage offset for `indices` into `array` (already
    // count-matched by `parse_index_list`), range-checking each dimension
    // against its declared bounds. Only meaningful under real execution --
    // callers skip this during a dry run. REQ-0210.
    [[nodiscard]] std::optional<std::size_t> array_flat_offset(
        const ArrayValue& array,
        const std::vector<std::pair<Integer, std::size_t>>& indices);

    [[nodiscard]] bool parse_array_element_assignment(
        const std::string& identifier, const std::size_t identifier_offset);

    // Element assignment against an array held in `slot` (a variable or a
    // class-instance array field, REQ-0251); positioned at the '('.
    [[nodiscard]] bool parse_array_element_assignment_on(
        Value& slot, const std::size_t identifier_offset);

    [[nodiscard]] std::optional<Value> parse_expression();

    [[nodiscard]] std::optional<Value> parse_implication();

    [[nodiscard]] std::optional<Value> parse_equivalence();

    [[nodiscard]] std::optional<Value> parse_exclusive_or();

    [[nodiscard]] std::optional<Value> parse_or();

    [[nodiscard]] std::optional<Value> parse_and();

    [[nodiscard]] std::optional<Value> parse_not();

    // VB `Like`: ? any char, * any run, # digit, [list]/[!list] with ranges.
    template <class Text>
    [[nodiscard]] static bool like_match_units(const Text& text,
                                               const Text& pattern,
                                               const bool fold) {
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
        std::function<bool(std::size_t, std::size_t)> match =
            [&](std::size_t t, std::size_t p) {
                while (p < pattern.size()) {
                    const Unit c = pattern[p];
                    if (c == Unit{'*'}) {
                        while (p < pattern.size() && pattern[p] == Unit{'*'}) {
                            ++p;
                        }
                        if (p == pattern.size()) {
                            return true;
                        }
                        for (std::size_t k = t; k <= text.size(); ++k) {
                            if (match(k, p)) {
                                return true;
                            }
                        }
                        return false;
                    }
                    if (t >= text.size()) {
                        return false;
                    }
                    if (c == Unit{'?'}) {
                        ++t;
                        ++p;
                    } else if (c == Unit{'#'}) {
                        if (!(text[t] >= Unit{'0'} && text[t] <= Unit{'9'})) {
                            return false;
                        }
                        ++t;
                        ++p;
                    } else if (c == Unit{'['}) {
                        const auto close = pattern.find(Unit{']'}, p + 2);
                        if (close == Text::npos) {
                            return false;
                        }
                        std::size_t q = p + 1;
                        bool negate = false;
                        if (q < close && pattern[q] == Unit{'!'}) {
                            negate = true;
                            ++q;
                        }
                        bool hit = false;
                        while (q < close) {
                            if (q + 2 < close && pattern[q + 1] == Unit{'-'}) {
                                const Unit lo =
                                    fold ? lower(pattern[q]) : pattern[q];
                                const Unit hi = fold ? lower(pattern[q + 2])
                                                     : pattern[q + 2];
                                const Unit ch = fold ? lower(text[t]) : text[t];
                                if (ch >= lo && ch <= hi) {
                                    hit = true;
                                }
                                q += 3;
                            } else {
                                if (same(pattern[q], text[t])) {
                                    hit = true;
                                }
                                ++q;
                            }
                        }
                        if (hit == negate) {
                            return false;
                        }
                        ++t;
                        p = close + 1;
                    } else {
                        if (!same(c, text[t])) {
                            return false;
                        }
                        ++t;
                        ++p;
                    }
                }
                return t == text.size();
            };
        return match(0, 0);
    }

    [[nodiscard]] static bool like_match(const std::string& text,
                                         const std::string& pattern,
                                         const bool fold);

    [[nodiscard]] std::optional<Value> parse_comparison();

    [[nodiscard]] std::optional<Value> parse_concatenation();

    [[nodiscard]] std::optional<Value> parse_additive();

    [[nodiscard]] std::optional<Value> parse_star_slash();

    // REQ-0244: VB precedence is `* /` > `\` > `Mod` > `+ -`.
    [[nodiscard]] std::optional<Value> parse_integer_division();

    [[nodiscard]] std::optional<Value> parse_multiplicative();

    [[nodiscard]] std::optional<Value> parse_unary();

    // REQ-0244: `a ^ b` (left-associative, binds tighter than unary minus).
    [[nodiscard]] std::optional<Value> parse_power();

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
    [[nodiscard]] std::size_t udt_byte_size(const Value& value) const;

    [[nodiscard]] std::optional<Value> instantiate_class(
        const std::string& class_name, const std::size_t offset);

    // Wraps parse_primary_base with postfix `.member`/`.member(args)`
    // handling, applied in a loop so a member that itself evaluates to an
    // object reference (a Variant-typed field or a Variant-returning
    // Function/Property Get holding one -- this evaluator's only routes to
    // an object-valued result besides a plain variable, since a dedicated
    // "As ClassName" method/property return type is deferred; see REQ-0203's
    // Scope) chains further (`a.b.c`), without every parse_primary_base exit
    // path needing to know about member access itself.
    [[nodiscard]] std::optional<Value> parse_primary();

    [[nodiscard]] std::optional<Value> parse_primary_base();

    // Reads `array_variable(index)`. `array_variable` is the array's
    // current Value (a reference into `variables_`, stable across this call
    // since expression parsing never inserts into that map).
    // The zero/default value for one of the fixed scalar type indices a
    // procedure parameter or Function return type can name (mirrors
    // `parse_type_keyword`'s non-Variant results).
    [[nodiscard]] static Value zero_value_for_index(
        const std::size_t type_index);

    // Parses one call argument. A bare identifier naming a declared
    // variable, with nothing else in its own argument slot, is captured as
    // a possible ByRef target; anything else (a literal, an operator
    // expression, `Not x`, `arr(i)`, an intrinsic/procedure call, ...)
    // parses as an ordinary expression with no write-back target.
    [[nodiscard]] std::optional<CallArgument> parse_call_argument();

    // Runs statements from the current offset_ until reaching `body_end`
    // (a procedure's own "End Sub"/"End Function" position, as recorded by
    // `scan_procedures`) or an Exit Sub/Exit Function. Mirrors `evaluate`'s
    // own top-level statement loop.
    [[nodiscard]] bool run_procedure_body(const std::size_t body_end);

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
    [[nodiscard]] std::optional<std::vector<CallArgument>>
    parse_call_argument_list();

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
        const ProcedureDef& definition, const std::string& binding_name,
        std::vector<CallArgument> arguments,
        const std::size_t identifier_offset, const std::string_view body_source,
        InstanceData* const instance);

    // Parses `(args)` for a call to the already-looked-up module-level
    // procedure `name`, then runs it via invoke_definition against the
    // module's own source and no instance scope.
    [[nodiscard]] std::optional<Value> call_procedure(
        const std::string& name, const std::size_t identifier_offset,
        const bool require_function);

    [[nodiscard]] std::optional<Value> parse_procedure_call(
        const std::string& name, const std::size_t identifier_offset);

    // Whether a method/property call is currently executing, and if so, its
    // instance and class (see instance_scopes_). Used to resolve an
    // unqualified call/read to a sibling member of the class currently
    // executing -- the implicit-Me equivalent VB6 itself provides for a
    // class's own members, without spelling out `Me.`.
    [[nodiscard]] InstanceData* current_instance() noexcept;
    [[nodiscard]] const ClassDef* current_class_def();

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
        const bool bypass_for_interface_dispatch = false);

    // The `Me` keyword: a fresh ObjectInstance sharing the current class
    // member's own instance (see current_instance), via
    // enable_shared_from_this so it shares that instance's existing control
    // block rather than creating a second, independent one. `offset` is the
    // `Me` token's own position, for the "only valid inside a class member"
    // diagnostic.
    [[nodiscard]] std::optional<Value> me_value(const std::size_t offset);

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
    [[nodiscard]] bool terminate_if_last_reference(Value& value);

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
    [[nodiscard]] bool drain_scope_instances(Scope& scope);

    // Parses `(args)` for a call to `class_def`'s already-looked-up
    // `member_name` method on `instance`, requiring it to be a Function
    // when `require_function` (an expression-context call; false for a
    // `Call`-statement Sub invocation). Shared by parse_member_access_after_dot
    // (an explicit `obj.Method(args)`) and parse_primary_base/
    // parse_call_statement (an unqualified sibling call resolved via
    // current_instance/current_class_def).
    [[nodiscard]] std::optional<Value> call_class_method(
        InstanceData& instance, const ClassDef& class_def,
        const std::string& member_name, const std::size_t member_offset,
        const bool require_function,
        const bool bypass_for_interface_dispatch = false);

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
        const Value base, const std::size_t base_offset,
        const bool require_function = true,
        const std::string& via_interface_class = {});

    // `Call name(args)` -- the only supported way to invoke a Sub as a
    // statement, or a Function while discarding its result, matching real
    // VB6's `Call` statement (this evaluator does not support VB6's other,
    // parenthesis-free `name arg1, arg2` statement-call form; see
    // REQ-0202's Scope).
    [[nodiscard]] bool parse_call_statement();

    // Skips over a module-level `Sub`/`Function` declaration during the
    // main top-to-bottom pass: its body only runs when called, not where
    // it's textually written. `scan_procedures` already registered it
    // (including its `declaration_end`) before this pass began.
    // `Property Get|Let|Set Name(...) ... End Property` in a standard module:
    // `Property` has been consumed; skips the whole declaration.
    [[nodiscard]] bool parse_property_declaration_skip(
        const std::size_t statement_offset);

    [[nodiscard]] bool parse_procedure_declaration_skip(
        const std::size_t statement_offset);

    [[nodiscard]] std::optional<Value> parse_array_index(
        const Value& array_variable);

    [[nodiscard]] static bool is_misc_function_name(
        const std::string_view name);

    // REQ-0246: financial functions, FormatNumber/Currency/Percent, Partition.
    [[nodiscard]] std::optional<Value> evaluate_misc_function(
        const std::string_view name, std::vector<Value>& arguments,
        const std::size_t offset);

    [[nodiscard]] static bool is_date_function_name(
        const std::string_view name);

    [[nodiscard]] static double current_date_serial();

    // REQ-0242: Date/Time intrinsics.
    [[nodiscard]] std::optional<Value> evaluate_date_function(
        const std::string_view name, std::vector<Value>& arguments,
        const std::size_t offset);

    [[nodiscard]] std::optional<Value> parse_function_call(
        const std::string_view identifier, const std::size_t identifier_offset);

    [[nodiscard]] std::optional<Value> parse_function_call_impl(
        const std::string_view identifier, const std::size_t identifier_offset);

    [[nodiscard]] std::optional<Value> parse_string();

    // Consume `digits[.digits][(e|E)[+|-]digits]` from the current position,
    // returning true when a fractional or exponent part made it a Double form.
    [[nodiscard]] bool lex_number_span() noexcept;

    [[nodiscard]] std::optional<Value> parse_double(const std::size_t start,
                                                    const std::size_t end);

    [[nodiscard]] std::optional<Value> parse_single(const std::size_t start,
                                                    const std::size_t end);

    [[nodiscard]] std::optional<Value> parse_short_integer(
        const std::size_t start, const std::size_t end);

    // Parse a non-negative decimal span into a Currency scaled int64,
    // without an intermediate floating-point conversion, so a value with
    // more significant digits than a double can represent exactly (up to
    // Currency's full 19-digit magnitude) still parses exactly. Currency
    // literals do not support exponent notation.
    [[nodiscard]] std::optional<Value> parse_currency(const std::size_t start,
                                                      const std::size_t end);

    [[nodiscard]] std::optional<Value> parse_number();

    [[nodiscard]] std::optional<Value> parse_negative_number();

    // Round a Double to the nearest Long using banker's rounding (the default
    // IEEE round-to-nearest-even, matching VB6), rejecting out-of-range values.
    [[nodiscard]] std::optional<Value> round_double_to_long(
        const double number, const Integer minimum, const Integer maximum,
        const std::size_t offset);

    // Round a Double to the nearest Int16 (VB6 Integer) using banker's
    // rounding, rejecting out-of-range values.
    [[nodiscard]] std::optional<Value> round_double_to_short_integer(
        const double number, const std::size_t offset);

    // Render a Currency's exact scaled value as decimal digits: the whole
    // part, then a '.' and up to four fraction digits with trailing zeros
    // trimmed (an exact whole amount has no decimal point at all), matching
    // the shortest-round-tripping-form convention this evaluator already
    // uses for Double/Single rendering. No leading space (see render_str_
    // style callers, which add one for Str's sign-space convention).
    [[nodiscard]] static std::string render_currency(const std::int64_t scaled);

    // Fixed-point text of a non-negative finite number with `places` decimals,
    // rounded half-up from its 15-significant-digit decimal form (how VB's
    // Format rounds: 2.5 -> "3", 0.285 -> "0.29"), instead of from the exact
    // binary value.
    [[nodiscard]] static std::string fixed_half_up(const double magnitude,
                                                   const int places);

    // Render a finite double using VBA's "Fixed"/"Standard" Format styles:
    // exactly two decimal digits, an optional grouped integer part, and a
    // leading '-' only when the rounded magnitude is nonzero.
    [[nodiscard]] static std::string render_fixed_style(const double value,
                                                        const bool grouping);

    // Render a finite double using VBA's "Scientific" Format style: one
    // mantissa digit, two fraction digits, an uppercase 'E', an explicit
    // exponent sign, and a minimum two-digit exponent.
    [[nodiscard]] static std::string render_scientific_style(
        const double value);

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

    [[nodiscard]] static EscapedPicture parse_picture_escapes(
        const std::string& section);

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
        const double value, const std::string& picture);

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
        const double value, const std::string& picture);

    // Advance the verified VB6-reference Rnd generator by one 24-bit linear
    // congruential step: state' = (state * 0x43FD43FD + 0xC39EC3) mod 2^24.
    // Confirmed byte-for-byte against a local VB6 6.00.8176 probe.
    [[nodiscard]] static std::uint32_t rnd_step(
        const std::uint32_t state) noexcept;

    [[nodiscard]] static double rnd_value(const std::uint32_t state) noexcept;

    // WFC-owned deterministic seed hash used by Randomize(number) and
    // Rnd(negative). The reference VB6 runtime's own per-seed sequence is not
    // reproducible even for an explicit Randomize argument (confirmed by a
    // local probe: repeated Randomize calls with the same argument produced
    // different subsequent Rnd() results), so no formula could truthfully
    // claim to match it. This hash instead guarantees a WFC-specific
    // contract: the same seed always produces the same subsequent sequence.
    [[nodiscard]] static std::uint32_t seed_from_number(
        const double value) noexcept;

    // Non-deterministic seed source for argument-less Randomize, matching
    // VB6's documented system-timer-based reseeding.
    [[nodiscard]] static double entropy_seed();

    [[nodiscard]] const Integer* require_integer(
        const Value& value, const std::size_t operator_offset);

    [[nodiscard]] const bool* require_boolean(
        const Value& value, const std::size_t operator_offset);

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
        const Value& value, const std::size_t operator_offset);

    // Coerce an If/While/Do condition value to a Boolean, treating Null and
    // Empty as False (verified: `If Null Then` takes the Else branch with
    // no runtime error, unlike CBool(Null), which does error). Returns
    // nullopt with an error already set only for a genuinely wrong type.
    [[nodiscard]] std::optional<bool> coerce_condition_boolean(
        const Value& value, const std::size_t offset,
        const std::string_view error_code,
        const std::string_view error_message);

    // Converts a non-integer numeric or numeric-String operand of a bitwise
    // operator to Long (rounded); returns false on overflow.
    [[nodiscard]] bool widen_bitwise_operand(Value& value, std::size_t offset);
    [[nodiscard]] std::optional<Value> logical_binary(
        const Value& left, const Value& right, const char operation,
        const std::size_t operator_offset);

    [[nodiscard]] int compare_strings(const std::string_view left_in,
                                      const std::string_view right_in) const;

    [[nodiscard]] bool values_equal(const Value& left,
                                    const Value& right) const;

    [[nodiscard]] std::optional<Value> compare(
        const Value& left, const Value& right, const std::string_view operation,
        const std::size_t operator_offset);

    // Evaluate `+`, `-`, `*`, and `/`. Two Long operands under `+`/`-`/`*` keep
    // the exact integer path (including overflow); every other combination
    // computes in the wider of the two operand categories, in VB6's
    // Long < Currency < Single < Double promotion order (a Long-only `/`
    // still promotes to Double, matching the pre-existing rule). Currency
    // computes with exact scaled-int64 arithmetic rather than floating
    // point, preserving its whole point: exact decimal money math.
    // REQ-0247: Byte participates in arithmetic as a Long.
    [[nodiscard]] static Value widen_byte(const Value& value);

    [[nodiscard]] std::optional<Value> numeric_binary(
        const Value& left_in, const Value& right_in, const char operation,
        const std::size_t operator_offset);

    // Coerce a numeric operand to Long for the integer operators, rounding a
    // Double to the nearest even integer (VB6 banker's rounding). Non-numeric
    // operands and out-of-range magnitudes are rejected.
    [[nodiscard]] std::optional<Integer> coerce_long(
        const Value& value, const std::size_t operator_offset);

    [[nodiscard]] std::optional<Value> integer_binary(
        const Value& left_in, const Value& right_in, const char operation,
        const std::size_t operator_offset);

    // Exact, checked Int16 (VB6 Integer) `+`/`-`/`*` for two Int16 operands.
    // Callers guarantee both operands are already Int16 and the operation is
    // not `/` (which always promotes to Double, matching Long op Long).
    [[nodiscard]] std::optional<Value> short_integer_binary(
        const Value& left, const Value& right, const char operation,
        const std::size_t operator_offset);

    // REQ-0268: VB6 renders a Double with 15 significant digits and a Single
    // with 7 (C's %G rules: exponent form below 1E-4 or from 1E15/1E7 up).
    [[nodiscard]] static std::string render_floating(const double number,
                                                     const int digits);

    [[nodiscard]] static std::string render(const Value& value);

    void set_error(const std::string_view code, const std::string_view message,
                   const std::size_t offset);

    const char* error_source_data_{};
    std::string_view source_;
    // The standard-module program text: where module procedures live, even
    // while a class member (whose own text is `source_`) is executing.
    std::string_view main_source_;
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
    // Set when the current statement read a Variant variable: Variant
    // arithmetic that overflows is promoted (Integer -> Long -> Double) instead
    // of raising Overflow.
    // Records that `value`, read from a Variant, took part in the statement's
    // expression, so mixed comparisons and arithmetic follow Variant rules.
    void note_variant_value(const Value& value) noexcept;
    bool variant_operand_seen_{};
    bool vb_number_spacing_{};
    std::map<std::string, std::string> app_properties_;
    std::string app_source_storage_;
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
    std::size_t max_procedure_depth_{
        64U};  // raised when running on a large stack
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

}  // namespace wfc::detail

#endif  // WFC_INTERPRETER_INTERPRETER_HPP
