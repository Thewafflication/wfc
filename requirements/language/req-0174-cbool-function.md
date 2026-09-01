# REQ-0174 — CBool function subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0142, REQ-0160, REQ-0173, and REQ-0072

## Requirement

`CBool(<expression>)` shall return a `Boolean` from each current evaluator type:

| Input | Result |
| --- | --- |
| `Boolean` | The input unchanged |
| `Long` | Zero becomes `False`; every nonzero value becomes `True` |
| `Double` | Zero becomes `False`; every nonzero value becomes `True` |
| `String` | Case-insensitive `True`/`False`, or a complete finite decimal/exponent value |

String inputs allow surrounding whitespace and convert according to whether the
parsed finite numeric value is zero.

Locale-aware numeric strings, Empty, Null, Date, Currency, and general Variant
coercion remain outside this requirement.

## Diagnostics

`WFC0087` reports a String that is neither a Boolean name nor a complete finite
decimal or exponent value.

## Verification

- `tests/evaluator_tests.cpp` covers every current input type, case-insensitive
  names, signed numeric strings, whitespace, invalid strings, and overflow.
- `TC-MP0002-cbool-cli` verifies numeric and String conversion through
  `wfc --eval`.

## Reference

- [Microsoft VBA type-conversion functions](https://learn.microsoft.com/en-us/office/vba/language/concepts/getting-started/type-conversion-functions)
