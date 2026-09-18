# REQ-0171 — Abs and Sgn numeric functions

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0160, and REQ-0077

## Requirement

`Abs(<number>)` and `Sgn(<number>)` shall each require one `Long` or `Double`
argument. `Abs` shall preserve the input numeric type; `Sgn` shall return a
`Long`.

`Abs` returns the non-negative magnitude. Applying it to -2147483648 shall fail
with the existing integer-overflow diagnostic because its positive magnitude is
not representable as a signed 32-bit value.

`Sgn` returns -1 for a negative argument, 0 for zero, and 1 for a positive
argument. Calls may be nested in other expressions and intrinsic calls.

`Abs` returns a `Double` magnitude for a `Double` input, a `Single` magnitude
for a `Single` input (`REQ-0195`), an exact `Currency` magnitude for a
`Currency` input (`REQ-0196`), an exact `Decimal` magnitude for a `Decimal`
input (`REQ-0198`), and an `Integer` magnitude (failing with the same
overflow diagnostic for `Integer`'s own unrepresentable minimum value,
-32768) for an `Integer` input (`REQ-0199`).

## Verification

- `tests/evaluator_tests.cpp` covers Long/Integer/Single/Currency/Decimal/
  Double negative, zero, and positive values, result types, nesting, type
  mismatch, Long/Integer overflow behavior, and both wrong-arity forms of
  each function.
- `TC-MP0002-integer-math-cli` verifies both functions through `wfc --eval`.
