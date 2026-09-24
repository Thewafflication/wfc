# REQ-0223 — Format custom picture `%` scaling

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0218, REQ-0221, REQ-0222

## Requirement

The evaluator shall recognize an unescaped, unquoted `%` character
anywhere in a `Format`/`Format$` custom numeric picture section
(`REQ-0218`) as a scaling instruction: the value is multiplied by `100`
before any digit is matched against the section's placeholders, the
same scaling the named `Percent` style (`REQ-0193`) already applies.
The `%` character itself needs no separate handling to appear in the
output — it was never one of this format's own special characters
(`0`, `#`, `.`, `,`, `\`, `"`, `;`), so it already renders as an
ordinary literal at its own position once scaling is applied to the
value it is combined with.

- Scaling is decided per section (checked after `REQ-0220`'s section
  split and `REQ-0221`/`REQ-0222`'s escape/quote expansion), so a
  picture may scale in one section and not another — for example
  `"0%;(0)"` scales a positive value by 100 but not a negative one.
- An escaped `\%` (`REQ-0221`) or a quoted `"%"` (`REQ-0222`) is a plain
  literal `%` and does not trigger scaling, matching how those two
  mechanisms already suppress every other special meaning a character
  might otherwise have.
- Multiple `%` characters in the same section still scale by exactly
  `100` once, not once per `%` — matching real VB6, where `%` is a
  presence flag for the section, not a per-occurrence multiplier.

## Diagnostics

None specific to this requirement — a `%` is never rejected, only
applied as a scaling flag (or rendered as a literal, if escaped/quoted).

## Scope

This requirement adds `%` scaling within an otherwise-ordinary custom
picture (digit placeholders, decimal point, comma grouping, sections,
escapes, quoted text). It does not add:

- `E+`/`E-` scientific-notation combined with a custom picture — a
  materially larger feature (custom mantissa/exponent digit counts),
  still excluded per `REQ-0218`'s original Scope;
- overflow detection for the scaled value (the way the named `Percent`
  style's own `WFC0009` check already covers) — a custom picture's
  underlying digit-rendering machinery has no overflow check for any
  input today, scaled or not, so none was added specifically for this
  case either; a value large enough to overflow after `x100` scaling is
  an extreme, unverified edge case.

## Verification

- `tests/evaluator_tests.cpp` covers scaling with a positive and a
  negative value, an escaped `\%` and a quoted `"%"` both suppressing
  scaling, and a two-section picture scaling only its selected section.
- `TC-MP0002-format-percent-picture-cli` verifies scaling, the escaped/
  quoted-literal `%` cases, and the two-section case, through
  `wfc --eval`.

## Reference

- [Microsoft VBA `Format` function reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/format-function)

## Traceability

This requirement extends `REQ-0218`'s custom numeric picture with the
`%` scaling token its own Scope section listed as deferred, reusing the
same `x100` scaling the named `Percent` style (`REQ-0193`) already
applies.
