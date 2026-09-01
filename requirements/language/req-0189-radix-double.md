# REQ-0189 — Hex and Oct over Long and Double

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0072 and REQ-0181

## Requirement

`Hex`/`Hex$` and `Oct`/`Oct$` shall accept a `Long`, `Double`, or `String`
containing a complete finite decimal/exponent number after surrounding ASCII
whitespace is removed. A `Double` or numeric `String` shall first be rounded to
the nearest even `Long` (banker's rounding). The resulting 32-bit value shall
then be rendered using the existing uppercase two's-complement hexadecimal or
octal representation.

A malformed numeric `String` shall report `WFC0099`; another non-numeric
argument shall report `WFC0073`. A rounded value outside the `Long` range shall
report `WFC0009`; wrong arity shall report `WFC0072`.

## Scope

`Single`, `Currency`, and `Decimal` inputs remain deferred with those distinct
value types. This requirement retains the current 32-bit `Long` radix width.

## Verification

- `tests/evaluator_tests.cpp` covers half-even rounding in both directions,
  numeric Strings with whitespace and exponent forms, negative two's-complement
  output, malformed/non-numeric rejection, and overflow.
- `TC-MP0002-radix-cli` verifies numeric-String hexadecimal and octal conversion
  through `wfc --eval`.

## Traceability

This requirement extends the `Hex` and `Oct` portion of `REQ-0072` to the
`Double` model introduced by `REQ-0181` and the current String model.
