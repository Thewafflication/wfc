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
| `Single`/`Double` | The shortest round-tripping decimal form for the argument's type (`REQ-0181`/`REQ-0195`) |
| `Boolean` | `True` or `False` |
| `String` | The input unchanged |

Calls may be nested inside other expressions and intrinsic calls.

Date, Empty, Null, Error, Currency, locale-dependent formatting, and general
Variant coercion remain outside this requirement.

## Verification

- `tests/evaluator_tests.cpp` covers every current input type, negative
  values, concatenation, nested use, and both wrong-arity forms.
- `TC-MP0002-cstr-cli` verifies numeric and Boolean conversion through
  `wfc --eval`.
