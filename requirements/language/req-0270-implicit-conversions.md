# REQ-0270 — Implicit scalar conversions on assignment

## Statement

Assigning (or passing ByVal, or storing into an array element or a `For
Each` variable) converts between scalar types the way VB6 does, instead of
reporting a mismatch for every non-identical pair:

- number ↔ number, rounding **half to even**: `Dim n As Long: n = 10 / 4`
  gives `2`, `n = 2.5` gives `2`, `n = 3.5` gives `4`; out of range is
  overflow (error 6);
- `Boolean` → number (`True` is `-1`), number → `Boolean` (non-zero is
  `True`);
- numeric `String` → number (`n = "12"`), `"True"`/`"False"` → `Boolean`; a
  non-numeric string is error 13 (`WFC0016`, now catchable as error 13);
- number / `Boolean` / `Date` → `String`;
- `Empty` → the target's zero value; `Null` → error 94 for non-Variant
  targets.

A **variable** passed by reference must already have the parameter's exact
type (VB6 "ByRef argument type mismatch", `WFC0016`); previously a `Long`
variable passed to a `Double` ByRef parameter silently changed type in the
caller. Pass an expression (`CDbl(x)`) or declare the parameter `ByVal`.

## Scope

Implicit conversion of `String` to `Date` keeps its existing rules
(`REQ-0242`); conversion into objects, arrays and UDTs is never implicit.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-implicit-conversions-cli`; the
corpus programs. Existing negative tests that expected `WFC0016` for a
`Long`-to-`String` loop variable now expect the conversion.
