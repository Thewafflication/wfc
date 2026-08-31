# REQ-0180 — Round function over the Long domain

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0160, and REQ-0077

## Requirement

`Round(<number>)` and `Round(<number>, <digits>)` shall accept one or two
arguments. `<number>` shall be a `Long`; the optional `<digits>` shall be a
`Long` naming the number of decimal places to retain.

Because every value in the current numeric model is a whole `Long`, rounding to
zero or more decimal places returns the argument unchanged. A `<digits>` value
less than zero shall fail with `WFC0094`. Wrong arity shall fail with `WFC0072`
and a non-`Long` argument shall fail with `WFC0073`.

## Scope

This requirement exposes `Round` for the integer-compatible numeric model
alongside `Abs`, `Sgn`, `Int`, and `Fix`. Banker's rounding of fractional
magnitudes, `Currency`/`Decimal`, and floating-point arguments remain deferred
until a deliberate numeric-value architecture increment introduces a
fractional type; only then can a non-identity `Round` be represented without
truncation.

## Verification

- `tests/evaluator_tests.cpp` covers one- and two-argument calls, a positive
  digit count, wrong arity, a non-Long argument, and a negative digit count.
- `TC-MP0002-round-cli` verifies `Round` through `wfc --eval`.

## Traceability

This requirement partially implements `REQ-0077`.
