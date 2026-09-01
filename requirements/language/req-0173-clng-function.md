# REQ-0173 — CLng function subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0141, REQ-0160, and REQ-0072

## Requirement

`CLng(<expression>)` shall return a `Long` from each current evaluator type:

| Input | Result |
| --- | --- |
| `Long` | The input unchanged |
| `Double` | Rounded to the nearest even integer |
| `Boolean` | `True` becomes -1; `False` becomes 0 |
| `String` | A complete finite decimal/exponent value, allowing surrounding whitespace, rounded to nearest even |

String conversion shall consume the entire trimmed input and fail on empty or
trailing text. Values outside the signed 32-bit range after rounding shall use
the existing overflow diagnostic.

Locale-aware numeric strings, Empty, Null, Date, Currency, and general Variant
coercion remain outside this requirement.

## Diagnostics

`WFC0086` reports a String that is not a complete whole decimal value.

## Verification

- `tests/evaluator_tests.cpp` covers every current input type, signs,
  whitespace, nested conversion, malformed strings, fractions, and overflow.
- `TC-MP0002-clng-cli` verifies String and Boolean conversion through
  `wfc --eval`.

## Reference

- [Microsoft VBA type-conversion functions](https://learn.microsoft.com/en-us/office/vba/language/concepts/getting-started/type-conversion-functions)
