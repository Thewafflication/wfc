# REQ-0190 — CVar scalar identity conversion

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0072, REQ-0140, REQ-0160, and REQ-0181

## Requirement

The MP-0002 evaluator shall recognize the case-insensitive `CVar` intrinsic
with exactly one argument. For each currently representable scalar value type
(`Long`, `Double`, `Boolean`, and `String`), `CVar` shall preserve both the
value and its run-time subtype.

## Scope

The evaluator's `Value` union is its current Variant representation, so scalar
conversion is an identity operation. This increment does not add `Empty`,
`Null`, `Error`, `Date`, `Currency`, arrays, objects, or user-defined types;
those require distinct value alternatives before `CVar` can preserve them.

## Verification

Unit tests cover value and subtype preservation for all four current scalar
types plus wrong-arity rejection. `TC-MP0002-cvar-cli` verifies identity and
run-time type visibility through `wfc --eval`.

## Traceability

This requirement partially implements `REQ-0072`.
