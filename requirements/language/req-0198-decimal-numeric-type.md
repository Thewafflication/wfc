# REQ-0198 — Decimal numeric value type

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0160, REQ-0181, REQ-0195, REQ-0196, and
REQ-0197

## Requirement

The evaluator shall represent a `Decimal` value the way VB6/COM's `DECIMAL`
structure does: a sign, a variable scale from 0 through 28 (the count of
digits to the right of the decimal point), and a 96-bit unsigned integer
mantissa, giving exact base-10 arithmetic across `Decimal`'s full range —
unlike `Single`/`Double` (binary floating point) and unlike `Currency`
(exact, but fixed at four decimal digits). Real VB6 only exposes `Decimal`
through a `Variant`; there is no `Dim x As Decimal` declaration syntax, so
`Decimal` is reachable only via `CDec` and through arithmetic that produces
one, matching real VB6 exactly (this corrects an initial scoping assumption
that `Dim`/`Const As Decimal` would need support).

- `CDec(<value>)` requires one argument and returns a `Decimal`: a `Decimal`
  argument returns unchanged; a `Long` or `Currency` argument widens exactly
  (a `Currency`'s scaled 64-bit integer becomes a scale-4 `Decimal`
  mantissa); a `Boolean` argument becomes `0` or `-1`; a `Single`, `Double`,
  or numeric `String` argument converts through the value's shortest
  round-tripping decimal text, matching `CDec`'s documented behavior of
  producing the "obvious" decimal value for a literal like `"10.25"` rather
  than a binary-floating-point artifact. `CDec(Null)` reports `WFC0104`; a
  malformed `String` reports `WFC0105`; a magnitude or scale outside
  `Decimal`'s range reports `WFC0009`.
- `+`, `-`, `*`, and `/` produce an exact `Decimal` result whenever at least
  one operand is `Decimal`. `Decimal` dominates every other numeric type,
  including `Double` — verified against a local VB6 6.00.8176 reference
  probe (`CDec(1) + 1234567.89`, a large-magnitude `Double` literal, returns
  a `Decimal` `1234568.89`, and the same holds for `Decimal` vs `Currency`
  and `Decimal` vs `Long`). This corrects the previous, unverified assumption
  that `Double`'s larger representable magnitude made it dominant instead:
  - `+`/`-` align both operands to the larger scale (exact, since increasing
    scale is a power-of-ten multiply) and add/subtract the 96-bit-or-narrower
    mantissas as signed magnitudes.
  - `*` computes the exact product of the two mantissas (needing up to 192
    bits before scale reduction) and, if the result does not fit in 96 bits
    or the combined scale exceeds 28, reduces the scale with banker's
    rounding until it does.
  - `/` scales the numerator up (by as much as preserves exactness and fits
    the 96-bit mantissa, up to scale 28) before performing exact integer long
    division, maximizing the quotient's precision; the remainder determines
    banker's rounding of the final digit. `WFC0008` reports division by a
    zero `Decimal`.
  - Any operation whose exact result does not fit `Decimal`'s 96-bit
    mantissa/scale-28 range reports `WFC0009`.
- `Long`, `Currency`, `Single`, and `Double` operands widen to `Decimal`
  losslessly (`Long`, `Currency`) or through their shortest round-tripping
  decimal text (`Single`, `Double`; the same conversion `CDec` itself uses)
  before the operation above.
- Comparisons involving a `Decimal` operand evaluate numerically the same way
  every other numeric comparison does (widening to `Double`); this is exact
  enough for comparison purposes even though it is not `Decimal`'s
  exact-arithmetic path.
- `TypeName` returns `"Decimal"`; `VarType` returns the `vbDecimal` value
  `14`.
- `Abs`, `Int`, `Fix`, and `Round` preserve `Decimal` in the result, computed
  with exact mantissa arithmetic (not a `Double` intermediate): `Abs` clears
  the sign; `Int`/`Fix` divide the mantissa by 10^scale with `Int` flooring
  and `Fix` truncating; `Round` reduces the scale toward the requested digit
  count using the same banker's-rounding mantissa division as `*`'s scale
  reduction.
- `CLng`, `CInt`, `CByte`, `CBool`, `CStr`, `CDbl`, `CSng`, `CCur`,
  `IsNumeric`, `Sgn`, `Str`, and `Hex`/`Oct` accept a `Decimal` argument the
  same way they already accept `Currency`. `CStr` renders a `Decimal`
  argument exactly (trimming trailing fractional zeros, matching the
  `Currency` rendering convention) rather than through a lossy `Double`
  intermediate; the other conversions widen through the `Decimal`'s `Double`
  value, which is exact enough for their integer-range or single/double-
  precision results.

## Diagnostics

`WFC0008` reports `Decimal` division by zero. `WFC0009` reports `Decimal`
arithmetic overflow (result too large for the 96-bit mantissa or scale > 28)
or a `CDec` argument outside `Decimal`'s range. `WFC0104` reports `CDec`
called with `Null` (see `REQ-0197`). `WFC0105` reports a `CDec` `String`
argument that is not a complete finite numeric value. `WFC0072`/`WFC0073`
report `CDec`'s wrong arity the same way every other conversion function
does.

## Scope

This requirement does not add:

- `Dim`/`Const As Decimal` — not valid VB6 syntax; `Decimal` is Variant-only,
  matching the reference language exactly (see Requirement);
- `Format`'s named styles gaining `Decimal`-specific behavior (`Format`
  already accepts a `Decimal` argument by widening through `Double`, the
  same as every other numeric type it accepts);
- calling any intrinsic function without parentheses was deferred here;
  `REQ-0213` later added the zero-argument form.

## Verification

- `tests/evaluator_tests.cpp` covers `CDec` from `Long`/`Currency`/`Boolean`/
  `Double`/`String`, `TypeName`/`VarType`, exact `+`/`-`/`*`/`/` arithmetic
  (including a multiplication whose exact intermediate product needs more
  than 64 bits, and a division producing a full-precision repeating
  decimal), `Decimal` dominating `Single`, `Currency`, `Long`, and `Double`
  in mixed-type arithmetic (both operand orders), `Abs`/`Int`/`Fix`/`Round`
  type and value preservation (including banker's-rounding-to-even cases),
  `CLng`/`CStr`/`CBool`/`IsNumeric`/`Hex` over a `Decimal` argument, and the
  `WFC0104`/`WFC0105`/`WFC0008`/`WFC0009` diagnostics.
- `TC-MP0002-decimal-cli` verifies declaration-via-`Variant`, `TypeName`,
  exact multiplication, and a full-precision division through `wfc --eval`.

## Reference

- Local reference probe: VB6 6.00.8176 (`VB6.EXE`), per
  `planning/reference-environment.md`. Probe source and captured output are
  recorded in `planning/work-log-mp-0002.md` (increment #91).

## Traceability

This requirement implements the `Decimal` type deferred by `REQ-0196`'s
Scope section (blocked at the time on the `Variant` type `REQ-0197` now
provides) and extends the `Long`/`Single`/`Currency`/`Double` coverage in
`REQ-0171`, `REQ-0176`, `REQ-0180`, `REQ-0187`, `REQ-0189`, `REQ-0195`, and
`REQ-0196` to `Decimal`.
