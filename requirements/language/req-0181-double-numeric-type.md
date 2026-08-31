# REQ-0181 — Double numeric value type

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140 and REQ-0160

## Requirement

The evaluator shall represent a fractional numeric value as an IEEE-754 double
(`Double`) alongside the existing `Long`, `String`, and `Boolean` values.

- A numeric literal containing a decimal point or an exponent shall lex to a
  `Double`; a bare integer literal shall remain a `Long`. Leading-dot forms
  (`.5`) and exponent forms (`1e3`, `2.5E-1`) shall be accepted.
- The operators `+`, `-`, and `*` shall keep the exact `Long` result when both
  operands are `Long`, and shall widen to `Double` when either operand is a
  `Double`. The `/` operator shall always produce a `Double`, reporting
  `WFC0008` on a zero divisor.
- The integer-division operators `\` and `Mod` shall round each `Double`
  operand to the nearest even `Long` (banker's rounding) before dividing, and
  shall reject a non-numeric operand with `WFC0007`.
- Comparisons between any combination of `Long` and `Double` shall evaluate
  numerically; `Long` widens to `Double` exactly.
- `CLng`, `CInt`, and `CByte` of a `Double` shall round to the nearest integer
  using banker's rounding (round half to even, matching VB6) and report
  `WFC0009` when the rounded value is out of range. `CBool` of a `Double` shall
  be `False` only for zero. `CStr` shall render a `Double` in its shortest
  round-tripping decimal form. `IsNumeric` of a `Double` shall be `True`.

## Scope

This requirement introduces the fractional value type and its arithmetic,
comparison, rendering, and conversion behavior. It does not yet add the
`Single`, `Currency`, or `Decimal` types, the `CDbl`/`CSng`/`CCur`/`CDec`
conversions, fractional `Val`, the floating-point math functions
(`Sqr`, `Sin`, ...), numeric type suffixes (`#`, `!`), `Format`, or exact VB6
`Str`/`Print` sign-space and locale formatting. `Double` rendering is the
shortest round-tripping form rather than the VB6 locale-specific rendering.

## Verification

- `tests/evaluator_tests.cpp` covers literal forms, mixed arithmetic, true
  division, comparison across `Long`/`Double`, banker's-rounding conversions,
  rendering, division by zero, and banker's-rounding of `Double` operands for
  `\`/`Mod`.
- `TC-MP0002-double-cli` verifies division, widening, and conversion through
  `wfc --eval`.

## Traceability

This requirement extends the numeric model of `REQ-0160` and underpins the
deferred floating-point functions and conversions in `REQ-0072` and `REQ-0077`.
