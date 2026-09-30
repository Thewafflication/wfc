# REQ-0264 — `Format` with a String value

## Statement

`Format(text, style)` applies a VB string format: `@` (character or space),
`&` (character or nothing) placeholders, `>` upper-case, `<` lower-case, `!`
left-to-right fill, literal text via `"..."` or `\`, and a second `;`-section
used for the empty string. Without placeholders only case conversion is
applied.

## Scope

Numeric/date formats are unchanged (`REQ-0193`, `REQ-0242`); a Null value and
the three-section numeric+text combined styles are not handled.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-string-format-cli`.
