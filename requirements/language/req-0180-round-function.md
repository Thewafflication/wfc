# REQ-0180 — Round function

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0160, and REQ-0077

## Requirement

`Round(<number>)` and `Round(<number>, <digits>)` shall accept one or two
arguments. `<number>` shall be a `Long` or `Double`; the optional `<digits>`
shall be a `Long` naming the number of decimal places to retain.

`Double` values shall use banker's rounding (round half to even). A `Long`
value is already whole and remains unchanged. A digit count at or beyond the
round-tripping decimal precision of `Double` also leaves the value unchanged,
avoiding an overflowing decimal scale that cannot affect the represented
value. A `<digits>` value less than zero shall fail with `WFC0094`. Wrong arity
shall fail with `WFC0072` and a non-numeric argument shall fail with `WFC0073`.

## Scope

This requirement covers the current `Long` and `Double` numeric model.
`Single`, `Currency`, and `Decimal` inputs remain deferred with those distinct
value types. Decimal-place rounding follows the host IEEE-754 representation;
it does not claim exact base-10 `Currency` or `Decimal` arithmetic.

## Verification

- `tests/evaluator_tests.cpp` covers one- and two-argument calls, positive and
  negative values, half-even ties, decimal places, a precision-saturating digit
  count, preserved result type, wrong arity, a non-numeric argument, and a
  negative digit count.
- `TC-MP0002-round-cli` verifies `Round` through `wfc --eval`.

## Traceability

This requirement partially implements `REQ-0077`.
