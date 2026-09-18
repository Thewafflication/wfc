# REQ-0182 — CDbl and CSng conversions

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0072 and REQ-0181

## Requirement

`CDbl(<value>)` and `CSng(<value>)` shall each require one argument. `CDbl`
returns a `Double`; `CSng` returns a `Single` (`REQ-0195`):

- a `Long`, `Single`, or `Double` argument converts to its numeric value;
- a `Boolean` argument converts to `-1` for `True` and `0` for `False`;
- a `String` argument is trimmed of surrounding whitespace, may carry a leading
  `+` or `-`, and is parsed as a decimal/exponent number; an empty or
  non-numeric String fails with `WFC0095`.

A numeric String outside the `Double` range fails with `WFC0009`. `CSng`
likewise fails with `WFC0009` when the converted value is outside the finite
IEEE single-precision range instead of returning an infinity.

`CSng` narrows the result to IEEE single precision and returns it as a
genuine `Single` value (`REQ-0195`); conversions of values not exactly
representable in single precision reflect single-precision rounding.

Wrong arity fails with `WFC0072`.

## Scope

This requirement adds the floating-point conversions enabled by the `Double`
type (`REQ-0181`) and, since `REQ-0195`, the `Single` type. `CCur` and `CDec`
remain deferred until the `Currency` and `Decimal` types exist.

## Verification

- `tests/evaluator_tests.cpp` covers `Long`, `Single`, `Double`, `Boolean`, and
  `String` inputs, a signed and whitespace-padded String, single-precision
  narrowing, round-trips through `CLng`, empty/malformed rejection, `Double`
  String overflow, positive/negative Single-range overflow, `CSng` returning a
  genuine `Single` (verified through `TypeName`), and both wrong-arity forms
  of each function.
- `TC-MP0002-cdbl-cli` verifies `CDbl` and `CSng` through `wfc --eval`.

## Traceability

This requirement partially implements `REQ-0072`, consumes `REQ-0181`, and is
extended by `REQ-0195`, which changed `CSng` from narrowing-but-storing-as-
`Double` to returning a genuine `Single` value.
