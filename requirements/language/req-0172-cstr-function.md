# REQ-0172 — CStr function subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0141, REQ-0160, and REQ-0072

## Requirement

`CStr(<expression>)` shall accept each value type in the current evaluator and
return a `String`:

| Input | Result |
| --- | --- |
| `Long` | Base-10 digits with a minus sign only when negative |
| `Boolean` | `True` or `False` |
| `String` | The input unchanged |

Calls may be nested inside other expressions and intrinsic calls.

Date, Empty, Null, Error, floating-point, Currency, locale-dependent formatting,
and general Variant coercion remain outside this requirement.

## Verification

- `tests/evaluator_tests.cpp` covers all three current input types, negative
  values, concatenation, and nested use.
- `TC-MP0002-cstr-cli` verifies numeric and Boolean conversion through
  `wfc --eval`.
