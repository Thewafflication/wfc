# REQ-0196 — Currency numeric value type

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0142, REQ-0153, REQ-0160, REQ-0171, REQ-0176,
REQ-0180, REQ-0181, REQ-0187, REQ-0189, and REQ-0195

## Requirement

The evaluator shall represent a `Currency` value as VB6/COM's fixed-point
representation: a signed 64-bit integer scaled by 10000 (four decimal
digits), distinct from `Long`, `Single`, `Double`, `String`, and `Boolean`.
This scale maps the documented `Currency` range,
-922337203685477.5808 through 922337203685477.5807, exactly onto
`std::int64_t`'s minimum and maximum, so `Currency` arithmetic within range
is exact rather than a binary-floating-point approximation — the type's
defining purpose.

- The `@` numeric literal suffix forces a literal to `Currency` (`123@`,
  `1.5@`). A `Currency` literal supports at most four fractional digits and
  does not support exponent notation; either reports `WFC0006`. The `@`
  identifier type-declaration character declares or asserts `Currency` the
  same way `!` does for `Single`, and `Dim`/`Const` accept `As Currency`.
- A `Currency`-typed variable initializes to zero. Assignment or constant
  initialization from a `Long` widens exactly (multiply by 10000). Assignment
  or constant initialization from a `Single` or `Double` rounds to the
  nearest `Currency` tick (banker's rounding), reporting `WFC0009` when the
  result is outside `Currency`'s range. Assignment or constant initialization
  from a `Currency` to a `Single`- or `Double`-typed target widens through
  its exact decimal value.
- VB6's numeric promotion order is `Long` < `Currency` < `Single` < `Double`:
  the operators `+`, `-`, `*`, and `/` compute in the wider of the two
  operand categories. A `Currency` result (both operands `Long`/`Currency`,
  at least one `Currency`) is computed with exact scaled-integer arithmetic,
  not floating point: `+`/`-` use checked 64-bit addition/subtraction;
  `*`/`/` use an exact 128-bit intermediate (a 64-bit product or a
  `value * 10000` numerator can each exceed 64 bits) with banker's rounding
  on the exact remainder. Any of the four operators reports `WFC0009` when
  the mathematical result is outside `Currency`'s range; `/` reports
  `WFC0008` on a zero divisor. The `\` and `Mod` operators round a `Currency`
  operand to the nearest even `Long` (banker's rounding) before dividing,
  reusing the existing rounding path.
- Comparisons between any combination of `Long`, `Currency`, `Single`, and
  `Double` evaluate numerically by widening to `Double`.
- `TypeName` returns `Currency` for a `Currency` value; `VarType` returns the
  `vbCurrency` value `6`.
- `CCur(<value>)` requires one argument and returns a `Currency`: a `Long`
  widens exactly; a `Currency` argument returns unchanged; a `Single`,
  `Double`, or `Boolean` argument rounds to the nearest tick (banker's
  rounding); a `String` argument is parsed the same way `CDbl` parses one,
  then rounded. A value outside `Currency`'s range reports `WFC0009`; a
  malformed `String` reports `WFC0103`.
- `CDbl`, `CSng`, `CLng`, `CInt`, `CByte`, `CBool`, `CStr`, `IsNumeric`,
  `Abs`, `Sgn`, `Int`, `Fix`, `Round`, `Str`, and `Hex`/`Oct` now accept a
  `Currency` argument the same way they already accept `Single`/`Double`.
  `Abs`, `Int`, `Fix`, and `Round` preserve `Currency` in the result, using
  exact scaled-integer arithmetic (not floating point) the same way the `+`/
  `-`/`*`/`/` operators do.

## Diagnostics

`WFC0006` reports a `Currency` literal with an exponent, more than four
fractional digits, or a magnitude outside `Currency`'s range. `WFC0008`
reports `Currency` division by zero. `WFC0009` reports `Currency` arithmetic
overflow or a narrowing conversion (assignment, `Const`, or `CCur`) outside
`Currency`'s range. `WFC0072`/`WFC0073` report `CCur`'s wrong arity or a
non-numeric argument the same way every other conversion function does.
`WFC0103` reports a `CCur` `String` argument that is not a complete finite
numeric value.

## Scope

This requirement does not add:

- `Decimal`, which remains blocked on a `Variant` type that does not yet
  exist (real VBA only exposes `Decimal` through `CDec` into a `Variant`);
- the `%` (Integer) literal suffix or identifier type-declaration character,
  which still reports `WFC0097`/`WFC0006` — that needs the distinct
  `Integer` value type from `REQ-0072`'s Tailoring list;
- `Format`'s `Currency` named style (a locale currency symbol and formatting
  convention are a `Format`-specific design question, separate from the
  `Currency` type itself, and remain deferred per `REQ-0193`'s Scope).
  `Format` already accepts a `Currency` argument for its existing named
  styles by widening through `Double`, since `Format` widens every numeric
  argument that way;
- calling any intrinsic function without parentheses, which remains deferred
  with every other function under the evaluator's existing
  parenthesized-call-only architecture.

## Verification

- `tests/evaluator_tests.cpp` covers the `@` literal suffix (integral,
  fractional, negative, and boundary-range forms), rejected exponent and
  excess-fractional-digit literals, the `@` identifier type-declaration
  character, `Dim`/`Const As Currency`, exact `Long` widening, checked
  `Single`/`Double` narrowing and `Currency`-to-`Single`/`Double` widening,
  `TypeName`/`VarType`, exact `+`/`-`/`*`/`/` arithmetic (including a
  multiplication whose exact intermediate product exceeds 64 bits, a
  division with a repeating decimal requiring rounding, and both overflow
  and division-by-zero diagnostics), cross-type comparison, and the extended
  `CCur`/`CDbl`/`CSng`/`CLng`/`CInt`/`CByte`/`CBool`/`CStr`/`IsNumeric`/`Abs`/
  `Int`/`Fix`/`Round`/`Str`/`Hex` behavior over `Currency` arguments,
  including exact type preservation where applicable.
- `TC-MP0002-currency-cli` verifies declaration, exact arithmetic, and
  conversion through `wfc --eval`.

## Traceability

This requirement adds the `Currency` type deferred by `REQ-0181` and
`REQ-0195`'s Scope sections, and extends the `Long`/`Single`/`Double`
coverage in `REQ-0171`, `REQ-0172`, `REQ-0176`, `REQ-0180`, `REQ-0187`, and
`REQ-0189` to `Currency`.
