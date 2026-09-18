# REQ-0180 — Round function

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0160, and REQ-0077

## Requirement

`Round(<number>)` and `Round(<number>, <digits>)` shall accept one or two
arguments. `<number>` shall be a `Long`, `Single`, `Currency`, or `Double`;
the optional `<digits>` shall be a `Long` naming the number of decimal places
to retain.

`Single`/`Currency`/`Double` values shall use banker's rounding (round half
to even). A `Long` value is already whole and remains unchanged. `Round`
preserves a `Single`, `Currency`, or `Double` argument's type in the result.
`Currency` rounds using exact scaled-integer arithmetic rather than floating
point, since `Currency` already has no more than four decimal digits to
round from. A digit count at or beyond the round-tripping decimal precision
of the argument's type (or, for `Currency`, at or beyond its four decimal
digits) also leaves the value unchanged, avoiding an overflowing decimal
scale that cannot affect the represented value. A `<digits>` value less than
zero shall fail with `WFC0094`. Wrong arity shall fail with `WFC0072` and a
non-numeric argument shall fail with `WFC0073`.

## Scope

This requirement covers the current `Long` and `Double` numeric model;
`Single` preservation followed in `REQ-0195` and exact `Currency`
preservation in `REQ-0196`. `Decimal` inputs remain deferred with that
distinct value type. `Single`/`Double` decimal-place rounding follows the
host IEEE-754 representation; `Currency` rounding is exact base-10 fixed-
point arithmetic, not a floating-point approximation.

## Verification

- `tests/evaluator_tests.cpp` covers one- and two-argument calls, positive and
  negative values, half-even ties, decimal places, a precision-saturating digit
  count, preserved result type across `Long`/`Single`/`Currency`/`Double`,
  wrong arity, a non-numeric argument, and a negative digit count.
- `TC-MP0002-round-cli` verifies `Round` through `wfc --eval`.

## Traceability

This requirement partially implements `REQ-0077`.
