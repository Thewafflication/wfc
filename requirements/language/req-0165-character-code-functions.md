# REQ-0165 — Character code function subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0160 and REQ-0071

## Requirement

`wfc --eval` shall accept case-insensitive `Asc(<string>)`, `Chr(<code>)`, and
`Chr$(<code>)` calls anywhere an expression is valid.

`Asc` shall require a non-empty `String` and return the unsigned value of its
first byte as a `Long`. `Chr` shall require a `Long` from 0 through 127 and
return the corresponding one-byte ASCII `String`. Calls may be nested to round
trip ASCII characters.

The 0–127 restriction is deliberate for WFC's current byte-oriented, locale-
neutral string subset. ANSI code-page mapping, `AscB`, `AscW`, `ChrB`, `ChrW`,
and Variant coercion remain outside this requirement.

## Diagnostics

The shared call-expression diagnostics from REQ-0160 apply. `WFC0077` reports
an empty `Asc` input, and `WFC0078` reports a `Chr` code outside 0–127.

## Verification

- `tests/evaluator_tests.cpp` covers both directions, `$` spelling, nesting,
  type mismatches, empty input, and lower and upper range violations.
- `TC-MP0002-character-code-cli` verifies nested conversion through `wfc --eval`.
