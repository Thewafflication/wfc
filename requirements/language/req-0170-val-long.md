# REQ-0170 — Val numeric prefix

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0160, REQ-0072, and REQ-0181

## Requirement

`Val(<string>)` shall return the numeric value parsed from the leading numeric
prefix of its one `String` argument. Spaces, tabs, carriage returns, and line
feeds are removed before parsing, matching VBA's treatment of embedded blanks.
An optional leading sign is supported. Parsing stops at the first character that
is not part of the number; if no digits precede it, the result is zero.

The prefix may include a decimal point and an exponent (`e`/`E` with an optional
sign). When it does, `Val` returns a `Double` (`REQ-0181`); a bare integer
prefix returns a `Long`, so `Val` of a whole number remains usable where a
`Long` is required. Leading-dot forms (`.5`) are accepted.

Commas and currency symbols are not numeric characters. Hexadecimal (`&H`) and
octal (`&O`) prefixes are recognized and return a `Long`, including VBA's
signed 16-bit interpretation for values from `&H8000` through `&HFFFF` (and
their octal equivalents) and signed 32-bit interpretation when the high Long
bit is set. A value outside the representable 32-bit range fails with the
existing overflow diagnostic.

Variant coercion and legacy type-declaration suffix validation remain outside
this requirement.

## Verification

- `tests/evaluator_tests.cpp` covers signs, embedded blanks, prefix termination,
  empty/non-numeric input, commas, type mismatch, overflow, fractional and
  exponent prefixes returning `Double`, a leading-dot form, hexadecimal/octal
  prefixes, signed-width behavior, radix prefix termination, and overflow.
- `TC-MP0002-val-cli` verifies embedded-blank and radix parsing through
  `wfc --eval`.

## Reference

- [Microsoft VBA `Val` reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/val-function)
