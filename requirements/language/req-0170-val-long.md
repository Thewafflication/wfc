# REQ-0170 — Val Long subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0160, and REQ-0072

## Requirement

`Val(<string>)` shall return a `Long` parsed from the decimal integer prefix of
its one `String` argument. Spaces, tabs, carriage returns, and line feeds are
removed before parsing, matching VBA's treatment of embedded blanks. An
optional leading sign is supported. Parsing stops at the first character that
is not part of the integer; if no digits precede it, the result is zero.

Commas and currency symbols are not numeric characters. Values outside the
signed 32-bit range fail with the existing integer-overflow diagnostic.

The VBA function returns `Double`, but WFC currently has only a `Long` numeric
value. Decimal fractions and exponents therefore fail explicitly with
`WFC0084`; hexadecimal/octal prefixes fail with `WFC0085`. This avoids silently
returning a truncated or incorrectly typed value. Those forms shall be enabled
when the corresponding numeric types and radix conversion semantics exist.

Variant coercion and legacy type-declaration suffix validation remain outside
this requirement.

## Verification

- `tests/evaluator_tests.cpp` covers signs, embedded blanks, prefix termination,
  empty/non-numeric input, commas, type mismatch, overflow, and deferred forms.
- `TC-MP0002-val-cli` verifies embedded-blank parsing through `wfc --eval`.

## Reference

- [Microsoft VBA `Val` reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/val-function)
