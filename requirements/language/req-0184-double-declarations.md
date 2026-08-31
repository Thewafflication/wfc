# REQ-0184 — Double declarations and introspection

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0142, REQ-0153, REQ-0176, and REQ-0181

## Requirement

The evaluator shall accept module-level `Dim` variables and `Const` constants
declared `As Double`. A `Double` variable shall initialize to zero. Assignment
or constant initialization from a `Long` shall widen exactly to `Double`;
other mismatched value types shall report `WFC0016`.

`TypeName` shall return `Double` for a `Double` value, and `VarType` shall
return the `vbDouble` value `5`.

## Scope

This requirement extends the existing typed scalar declaration model. It does
not add implicit narrowing from `Double` to `Long`, untyped `Variant`
declarations, procedure-local declarations, or the distinct `Single`,
`Currency`, and `Decimal` value types.

## Verification

- `tests/evaluator_tests.cpp` covers default initialization, assignment,
  constant initialization, exact `Long` widening, arithmetic, introspection,
  and mismatched initializer/assignment rejection.
- `TC-MP0002-double-declarations-cli` verifies the declaration and
  introspection path through `wfc --eval`.

## Traceability

This requirement extends typed declarations from `REQ-0142` and constants
from `REQ-0153` to the numeric value introduced by `REQ-0181`, and completes
the `Double` classifications exposed by `REQ-0176`.
