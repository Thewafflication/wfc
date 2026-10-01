# REQ-0274 — `For` over any numeric control variable; bare calls in single-line `If`

## Statement

- The `For` control variable may be `Integer`, `Long`, `Byte`, `Single`,
  `Double` or a `Variant`. Bounds and `Step` may be any numeric type; whole-number
  variables round them (banker's rounding). A body that assigns the control
  variable changes the next iteration, as in VB6.
- A `Variant` control variable counts as `Double` when any of start/end/step is
  floating, otherwise as `Long`.
- `If c Then Proc a, b` (a bare call with arguments, no `Call`) works in a
  single-line `If`/`Else` branch.

## Verification

Corpus `27-for-types`; `tests/evaluator_tests.cpp`.

Addendum: `Select Case` compares numeric selectors and `Case` values of different
numeric types by value (`Integer` selector, `Long` literal; `Case 1 To 2` on a
`Double`); an array element alone in an argument slot (`Bump a(1)`) is passed
ByRef. Corpus `28-integer-programs`.
