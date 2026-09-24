# REQ-0195 — Single numeric value type

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0142, REQ-0153, REQ-0160, REQ-0171, REQ-0176,
REQ-0180, REQ-0181, REQ-0182, REQ-0184, REQ-0185, REQ-0187, and REQ-0189

## Requirement

The evaluator shall represent a `Single` value as an IEEE-754 single-precision
(32-bit) float, distinct from `Long`, `Double`, `String`, and `Boolean`.

- The `!` numeric literal suffix forces a literal to `Single`, including an
  otherwise integral literal (`123!`) and a fractional/exponent literal
  (`1.5!`, `2.5E-1!`). The `!` identifier type-declaration character declares
  or asserts `Single` the same way `#` does for `Double`, and `Dim`/`Const`
  accept `As Single`.
- A `Single`-typed variable initializes to zero. Assignment or constant
  initialization from a `Long` widens exactly to `Single`. Assignment or
  constant initialization from a `Double` narrows to `Single`, reporting
  `WFC0009` when the result is outside the finite `Single` range (matching
  `CSng`'s existing narrowing rule from `REQ-0182`). Assignment or constant
  initialization from a `Single` to a `Double`-typed target widens exactly.
- The operators `+`, `-`, `*`, and `/` produce a `Single` result when at least
  one operand is `Single` and neither operand is `Double`; a `Double` operand
  always dominates and produces a `Double` result, matching VB6's numeric
  widening. `/` reports `WFC0008` on a zero divisor; a result outside the
  finite `Single` range reports `WFC0009`. The `\` and `Mod` operators round a
  `Single` operand to the nearest even `Long` (banker's rounding) before
  dividing, reusing the existing `Double` rounding path.
- Comparisons between any combination of `Long`, `Single`, and `Double`
  evaluate numerically by widening to `Double`.
- `TypeName` returns `Single` for a `Single` value; `VarType` returns the
  `vbSingle` value `4`.
- `CSng` now returns a genuine `Single` value (previously it narrowed to
  float precision but stored the result in the `Double` slot, per the
  original `REQ-0182` text, because no distinct `Single` type existed).
  `CDbl`, `CLng`, `CInt`, `CByte`, `CBool`, `CStr`, `IsNumeric`, `Abs`, `Sgn`,
  `Int`, `Fix`, `Round`, `Str`, and `Hex`/`Oct` now accept a `Single` argument
  the same way they already accept a `Double` argument. `Abs`, `Int`, `Fix`,
  and `Round` preserve a `Single` argument's type in the result, matching
  their existing `Double`-preserving behavior; `Sgn`, `CLng`, `CInt`, `CByte`,
  and `CBool` still return their existing result types; `CStr`, `Str`, and
  `Hex`/`Oct` render/convert a `Single` argument the same way they already do
  for a `Double` argument (widening through the shared `Double` path).

## Scope

This requirement completes the `Single` foundation and extends every
intrinsic that already had a generic `Long`/`Double` numeric path. It does
not add:

- the `%` (Integer) or `@` (Currency) literal suffixes or identifier
  type-declaration characters, which still report `WFC0097`/`WFC0006` — those
  need the distinct `Integer` and `Currency` value types from `REQ-0072`'s
  Tailoring list;
- `Format`'s named styles gaining a `Single`-specific named style (they
  already accept `Single` by the existing `Long`/`Double`/`Boolean` widening
  in `REQ-0193`, since `Format` widens through `Double`);
- `Rnd` returning a genuine `Single` — deliberately not part of *this*
  increment (`REQ-0194` documented it as a known gap pending `Single`
  existing at all); implemented later as its own narrower follow-up, see
  `REQ-0194`'s own updated Scope note;
- calling any intrinsic function without parentheses was deferred here;
  `REQ-0213` later added the zero-argument form.

## Diagnostics

`WFC0006` reports a `%`/`@` literal suffix or a malformed/out-of-range
`Single` literal. `WFC0009` reports `Single` arithmetic overflow, a `Single`
narrowing (assignment, constant initialization, or `CSng`) outside the finite
`Single` range, or an out-of-range `Single`-to-`Long` conversion. `WFC0016`
reports an assignment or constant-initializer type mismatch that is not
covered by the `Long`\/`Single`\/`Double` widening and narrowing rules above.
`WFC0097` reports the still-unsupported `%`/`@` type-declaration characters.

## Verification

- `tests/evaluator_tests.cpp` covers the `!` literal suffix (integral,
  fractional, exponent, and negative forms), the `!` identifier
  type-declaration character, `Dim`/`Const As Single`, exact `Long` widening,
  `Double` narrowing with overflow rejection, `Single`-to-`Double` widening on
  assignment, `TypeName`/`VarType`, arithmetic promotion in every combination
  of `Long`/`Single`/`Double` (including division and overflow), cross-type
  comparison, and the extended `CSng`/`CDbl`/`CLng`/`CInt`/`CByte`/`CBool`/
  `CStr`/`IsNumeric`/`Abs`/`Sgn`/`Int`/`Fix`/`Round`/`Str`/`Hex` behavior over
  `Single` arguments, including type preservation where applicable.
- `TC-MP0002-single-cli` verifies declaration, arithmetic promotion, and
  conversion through `wfc --eval`.

## Traceability

This requirement extends the numeric model of `REQ-0181` with the `Single`
type it explicitly deferred, completes the `!` suffix left deferred by
`REQ-0185`/`REQ-0186`, supersedes the "no distinct Single type" language in
`REQ-0182`, and extends the `Double` coverage in `REQ-0171`, `REQ-0176`,
`REQ-0180`, `REQ-0187`, and `REQ-0189` to `Single`.
