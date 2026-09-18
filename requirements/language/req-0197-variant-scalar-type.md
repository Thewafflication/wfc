# REQ-0197 — Scalar Variant value type

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0160, REQ-0181, REQ-0190, REQ-0195, and
REQ-0196

## Requirement

The evaluator shall represent a scalar VB6/VBA `Variant`: a `Dim`/bare
`Dim`-declared variable that freely holds any of `Empty`, `Null`, `Boolean`,
`Long`, `Single`, `Currency`, `Double`, `Decimal`, or `String`, retyping
itself to whichever type it was last assigned rather than enforcing one fixed
type. `Empty` and `Null` become genuine, inspectable run-time states — no
other `Value` alternative can represent either.

- `Dim x As Variant` declares a `Variant`-typed variable, initialized to
  `Empty`. A bare `Dim x`, with no `As` clause and no identifier
  type-declaration character, is equivalent to `Dim x As Variant`, matching
  VB6's implicit-Variant default.
- Assignment to a `Variant`-declared variable accepts any value type and
  retypes the variable to match, unlike assignment to a fixed-type variable
  (`REQ-0171`), which still requires (with numeric widening/narrowing) the
  assigned value's type to match the variable's declared type.
- The `Null` and `Empty` keywords are literals usable anywhere an expression
  is expected.
- `IsNull(<value>)` and `IsEmpty(<value>)` return whether `<value>` currently
  holds `Null` or `Empty` respectively; every other value returns `False`.
  `IsArray`, `IsObject`, `IsError`, and `IsMissing` remain constant `False`,
  since arrays, object references, and error-value `Variant`s remain outside
  this requirement's scope (see Scope).
- `TypeName` returns `"Null"` for `Null` and `"Empty"` for `Empty`. `VarType`
  returns the `vbNull` value `1` for `Null` and the `vbEmpty` value `0` for
  `Empty`.
- Three-valued (Kleene) logic governs `Null`'s propagation through every
  operator:
  - `+`, `-`, `*`, `/`, and every comparison operator (`=`, `<>`, `<`, `<=`,
    `>`, `>=`) return `Null` when either operand is `Null`.
  - `And`, `Or`, `Not`, `Xor`, `Eqv`, and `Imp` treat `Null` as the ternary
    "unknown" state: `Null And False` is `False`, `Null And True` is `Null`,
    `Null Or True` is `True`, `Null Or False` is `Null`, and `Not Null` is
    `Null`. `Xor`, `Eqv`, and `Imp` follow from the standard identities
    `Eqv = Not(Xor(a, b))` and `Imp = Not(a) Or b`.
  - String concatenation (`&`) with exactly one `Null` operand treats that
    operand as an empty string (no error); concatenating two `Null` operands
    together reports `WFC0104`.
  - An `If`, `ElseIf`, `While`, or `Do` condition that evaluates to `Null`
    (or `Empty`) is treated as `False` — the associated branch or loop does
    not execute, and no error is raised.
  - `CBool`, `CStr`, `CByte`, `CInt`, `CLng`, `CDbl`, `CSng`, `CCur`, and
    `CDec` each report `WFC0104` for a `Null` argument.
- `Empty` coerces to a type-appropriate zero wherever a definite value is
  required: `0` in arithmetic and numeric conversions, `False` in `CBool` and
  as a logical operand, `""` when concatenated or rendered, and `0`/`""` when
  compared (matching whichever side of the comparison it faces).
- `CVar` (`REQ-0190`) already returns its argument unchanged for every
  representable scalar type, including `Null` and `Empty`, so no change was
  needed there.

## Diagnostics

`WFC0104` reports "Invalid use of Null": concatenating two `Null` operands
together, or passing `Null` to `CBool`, `CStr`, `CByte`, `CInt`, `CLng`,
`CDbl`, `CSng`, `CCur`, or `CDec`.

## Scope

This requirement implements only the agreed "Scalar Variant only" subset. It
does not add:

- arrays, `ReDim`, or any array-typed `Variant` — `IsArray` remains constant
  `False`;
- object references or late binding — `IsObject` remains constant `False`;
- `CVErr`/error-value `Variant`s — `IsError` remains constant `False`;
- optional-argument detection — `IsMissing` remains constant `False`
  (unchanged from `REQ-0072`'s original deferral; this evaluator has no
  user-defined `Sub`/`Function` parameter model at all yet);
- a `Variant`-typed `Const`. Real VB6 does not allow `Const x As Variant`
  either (constants must be an intrinsic data type), so this is not a gap;
- widening the fixed-type `Long`/`Single`/`Currency`/`Double`/`String`/
  `Boolean` declaration and assignment model from `REQ-0171` — a
  `Variant`-declared variable is the only place free retyping applies.

## Verification

- `tests/evaluator_tests.cpp` covers the `Null`/`Empty` literals,
  `IsNull`/`IsEmpty`, `TypeName`/`VarType` for both, `Dim x As Variant` and
  bare `Dim x` retyping across `Long`/`String`/`Boolean` assignments,
  assigning a `Variant`'s current value into a fixed-type variable (both a
  matching and a mismatched case), `Null` propagation through `+`,
  comparison, `And`/`Or`/`Not`, `If Null Then` taking the `Else` branch
  without error, `Empty` coercion in arithmetic and comparison, single- and
  double-`Null`-operand concatenation, and `CBool(Null)`/`CStr(Null)`
  reporting `WFC0104`.
- `TC-MP0002-variant-cli` verifies retyping, `IsNull`/`IsEmpty`, and `Null`
  propagation through `wfc --eval`.

## Reference

- Local reference probe: VB6 6.00.8176 (`VB6.EXE`), per
  `planning/reference-environment.md`. Probe source and captured output are
  recorded in `planning/work-log-mp-0002-strings.md` (increment #82). The
  probe verified: `TypeName`/`VarType` of `Null` and `Empty`;
  `IsNull`/`IsEmpty` of each; `Null & "x"` and `"x" & Null` both equal `"x"`
  with no error; `Null & Null` raises run-time error 94 ("Invalid use of
  Null"); `Empty & "x"` equals `"x"`; `Empty + 5` equals `5`;
  `Empty = 0` and `Empty = ""` are both `True`; `Null + 5` and `Null = 5`
  both propagate `Null` without error; `CBool(Null)` raises error 94;
  `CBool(Empty)` is `False`; `Null And False`/`Null And True`/
  `Null Or True`/`Null Or False`/`Not Null` match the three-valued-logic
  table above; and `If Null Then ... Else ...` takes the `Else` branch with
  `Err.Number = 0`.

## Traceability

This requirement implements the `Variant`, `Empty`, and `Null` states
deferred by `REQ-0072`'s Tailoring list and by `REQ-0190`'s Scope section.
