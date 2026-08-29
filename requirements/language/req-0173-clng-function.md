# REQ-0173 — CLng function subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0141, REQ-0160, and REQ-0072

## Requirement

`CLng(<expression>)` shall return a `Long` from each current evaluator type:

| Input | Result |
| --- | --- |
| `Long` | The input unchanged |
| `Boolean` | `True` becomes -1; `False` becomes 0 |
| `String` | A complete signed decimal integer, allowing surrounding whitespace |

String conversion shall consume the entire trimmed input and fail on empty,
fractional, exponent, or trailing text. Values outside the signed 32-bit range
shall use the existing overflow diagnostic.

VBA rounds fractional numeric inputs to the nearest even integer, but WFC does
not yet have fractional numeric values. Locale-aware numeric strings, Empty,
Null, Date, Currency, floating-point, and general Variant coercion remain
outside this requirement.

## Diagnostics

`WFC0086` reports a String that is not a complete whole decimal value.

## Verification

- `tests/evaluator_tests.cpp` covers every current input type, signs,
  whitespace, nested conversion, malformed strings, fractions, and overflow.
- `TC-MP0002-clng-cli` verifies String and Boolean conversion through
  `wfc --eval`.

## Reference

- [Microsoft VBA type-conversion functions](https://learn.microsoft.com/en-us/office/vba/language/concepts/getting-started/type-conversion-functions)
