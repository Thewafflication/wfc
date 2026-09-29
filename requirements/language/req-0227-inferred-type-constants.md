# REQ-0227 — Inferred-type constants

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0157

## Requirement

The evaluator shall recognize `Const <name> = <constant expression>`, with
neither a type-declaration character nor an `As Type` clause: the
constant's type is inferred from whatever type its initializer expression
itself evaluates to, rather than being checked against a separately
declared type.

This is the real-VB6 asymmetry with a bare `Dim <name>` (no type at
all), which instead always defaults to `Variant` (`parse_declaration`'s
own existing bare-`Dim` rule) — an untyped `Const` never becomes a
`Variant`; it takes on the initializer's own concrete type, the same as
if that expression appeared anywhere else in the program.

- `Const x = 5` is `Long` (this evaluator's own existing convention for
  an unsuffixed integral literal — see `REQ-0199`'s Scope for why this
  already differs from real VB6's own "smallest fitting type" inference,
  a pre-existing, evaluator-wide simplification this requirement does
  not change or newly introduce).
- `Const y = "hello"` is `String`; `Const z = True` is `Boolean`;
  `Const w = 3.14` is `Double` — each simply the type the initializer
  expression already produces everywhere else in the language.
- An inferred-type constant's initializer may reference a previously
  declared constant (inferred or explicitly typed alike) and use
  operators, exactly as an explicitly typed constant's initializer
  already could (`REQ-0157`).

## Diagnostics

None specific to this requirement — an inferred-type constant is never
rejected for its type (there is no declared type to mismatch against).
Every diagnostic `REQ-0157` already established for a *typed* constant
(`WFC0062` assignment to a constant, `WFC0063` inside a control-flow
block, `WFC0064` a mutable-variable reference) applies unchanged.

## Scope

This requirement adds only the type-inference behavior above. It does
not add:

- matching real VB6's own literal-based type inference exactly (for
  example inferring `Integer` instead of `Long` for a small whole-number
  constant) — this evaluator's unsuffixed-integer-literal-is-`Long`
  convention is unchanged and pre-existing (`REQ-0199`'s Scope), and an
  inferred-type constant simply inherits whatever that convention
  already produces;
- multiple comma-separated declarations in one `Const` statement, or a
  procedure-local `Const` — both remain excluded per `REQ-0157`'s own
  Scope, unrelated to type inference.

## Verification

- `tests/evaluator_tests.cpp` covers an inferred `Long`, `String`,
  `Boolean`, and `Double` constant, an inferred constant's initializer
  referencing a previously declared inferred constant, and the existing
  `WFC0064` mutable-variable-reference diagnostic still applying to an
  inferred-type constant.
- `TC-MP0002-const-inferred-type-cli` verifies all four inferred types
  and a derived inferred constant through `wfc --eval`.

## Reference

- [Microsoft VBA `Const` statement reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/const-statement)

## Traceability

This requirement closes the "inferred types" exclusion `REQ-0157`'s own
Scope section listed when `Const` was first implemented.
