# REQ-0199 — Integer numeric value type

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0142, REQ-0153, REQ-0160, REQ-0171, REQ-0176,
REQ-0180, REQ-0187, REQ-0189, REQ-0195, REQ-0196, and REQ-0198

## Requirement

The evaluator shall represent VB6/VBA's `Integer` value as a signed 16-bit
integer (`std::int16_t`), distinct from `Long` (this codebase's own C++
`Integer` alias, an unfortunate naming collision that predates this type's
introduction — VB6's `Integer` is 16-bit, this codebase's `Integer` alias is
32-bit and represents VB6's `Long`), `Single`, `Currency`, `Decimal`,
`Double`, `String`, and `Boolean`.

- The `%` numeric literal suffix forces a literal to `Integer` (`123%`), and
  requires the literal to already be an integral value in `Integer`'s
  -32768 through 32767 range; a fractional or exponent literal, or an
  out-of-range magnitude, reports `WFC0006`. The `%` identifier
  type-declaration character declares or asserts `Integer` the same way `@`
  does for `Currency`, and `Dim`/`Const` accept `As Integer`.
- An `Integer`-typed variable initializes to zero. Assignment or constant
  initialization from a `Long` narrows with a checked range (`WFC0009` when
  outside -32768 through 32767); from a `Double` or `Single`, banker's
  rounding is applied before the same range check; from a `Currency`, its
  exact decimal value is rounded (banker's rounding) before the same range
  check. Assignment from an `Integer` to a `Long`-, `Single`-, `Currency`-,
  or `Double`-typed target widens exactly. (Assignment *to* a `Long`-typed
  target never coerces from any other numeric type, including `Integer` —
  this is a pre-existing evaluator-wide scope boundary that predates this
  type, not something specific to `Integer`; see `REQ-0171`'s Scope for the
  same boundary already documented for `Currency`/`Single`.)
- VB6's numeric promotion order is `Integer` < `Long` < `Currency` <
  `Single` < `Decimal` < `Double`: the operators `+`, `-`, `*`, and `/`
  compute in the wider of the two operand categories. An `Integer` result
  (both operands `Integer`, at least one not `Long` or wider) is computed
  with exact checked 16-bit arithmetic, matching the existing `Long`+`Long`
  exact-arithmetic precedent; a mixed `Integer`/`Long` result widens the
  `Integer` operand exactly and computes exact checked `Long` arithmetic. As
  with the existing `Long`/`Long` rule, `Integer`/`Integer` (and any purely
  integral combination) under `/` still promotes to `Double`. `WFC0009`
  reports `Integer`-range overflow for exact `Integer` `+`/`-`/`*`; `\` and
  `Mod` continue to coerce any numeric operand (including `Integer`) to
  `Long` before dividing, matching the existing rule.
- Comparisons between any combination of `Integer`, `Long`, `Currency`,
  `Single`, `Decimal`, and `Double` evaluate numerically by widening to
  `Double`.
- `TypeName` returns `Integer` for an `Integer` value; `VarType` returns the
  `vbInteger` value `2`.
- `CInt(<value>)` now returns a genuine `Integer` (previously it narrowed to
  the -32768..32767 range but stored the result in the `Long` slot, per the
  original tailoring, because no distinct `Integer` type existed — the same
  situation `CSng` was in before `REQ-0195`). `CLng`, `CByte`, `CBool`,
  `CStr`, `CDbl`, `CSng`, `CCur`, `CDec`, `IsNumeric`, `Abs`, `Sgn`, `Int`,
  `Fix`, `Round`, `Str`, and `Hex`/`Oct` now accept an `Integer` argument the
  same way they already accept `Long`. `Abs`, `Int`, `Fix`, and `Round`
  preserve `Integer` in the result (`Int`/`Fix`/`Round` are no-ops on an
  already-integral `Integer`, matching the existing `Long` precedent; `Abs`
  reports `WFC0009` for the one unrepresentable magnitude, `Integer`'s
  minimum value, matching `Long`'s existing `Abs(-2147483648)` precedent).

## Diagnostics

`WFC0006` reports an `Integer` literal with a fractional or exponent form,
or a magnitude outside -32768 through 32767. `WFC0009` reports `Integer`
arithmetic overflow or a narrowing conversion (assignment, `Const`, or
`CInt`) outside `Integer`'s range. `WFC0104` reports `CInt` called with
`Null` (see `REQ-0197`), matching every other `CXxx` conversion. `WFC0072`/
`WFC0073`/`WFC0088` report `CInt`'s wrong arity, a non-numeric argument, or
a malformed numeric `String`, the same way every other conversion function
reports these.

## Scope

This requirement does not add:

- functions whose "Long argument" parameters were never extended to accept
  `Single`/`Currency`/`Decimal` either (`Choose`'s index, `QBColor`'s color
  index, `RGB`'s components, `MacID`, `Space`, `String$`'s count, `Chr`, the
  position/length arguments of `Mid`/`Left`/`Right`/`InStr`, `Round`'s digit
  count, and similar): these remain strictly `Long`, an existing scope
  boundary from every prior numeric-type increment, not something newly
  narrowed by this one;
- comma-separated multiple variable declarations in one `Dim`/`Const`
  statement (`Dim x As Integer, y As Long`) — this evaluator has never
  supported declaring more than one variable per `Dim`/`Const` statement,
  for any type, and this increment does not add it;
- calling any intrinsic function without parentheses, which remains deferred
  with every other function under the evaluator's existing
  parenthesized-call-only architecture.

## Verification

- `tests/evaluator_tests.cpp` covers the `%` literal suffix (boundary-range,
  out-of-range, and rejected-fractional forms), the `%` identifier
  type-declaration character, `Dim`/`Const As Integer`, checked narrowing
  from `Long` on assignment, `TypeName`/`VarType`, exact `Integer`+`Integer`
  arithmetic (including overflow), mixed `Integer`/`Long`/`Single`/
  `Currency`/`Decimal`/`Double` promotion, `\`/`Mod`, cross-type comparison,
  and the extended `CInt` (now genuinely `Integer`-typed)/`CLng`/`CByte`/
  `CBool`/`CStr`/`CDbl`/`CSng`/`CCur`/`CDec`/`IsNumeric`/`Abs`/`Int`/`Fix`/
  `Round`/`Str`/`Hex` behavior over `Integer` arguments, including exact
  type preservation where applicable, and `CInt(Null)` reporting `WFC0104`.
- `TC-MP0002-integer-cli` verifies declaration, exact arithmetic promotion,
  and widening through `wfc --eval`.

## Traceability

This requirement adds the `Integer` type deferred by `REQ-0072`'s Tailoring
list and by `REQ-0195`'s and `REQ-0196`'s Scope sections (the `%` literal
suffix and identifier character), and extends the `Long`/`Single`/
`Currency`/`Decimal`/`Double` coverage in `REQ-0171`, `REQ-0176`, `REQ-0180`,
`REQ-0187`, `REQ-0189`, `REQ-0195`, `REQ-0196`, and `REQ-0198` to `Integer`.
