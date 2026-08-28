# REQ-0160 — Len function subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0141, and REQ-0071

## Requirement

`wfc --eval` shall accept a case-insensitive `Len(<expression>)` call anywhere
an expression is valid. The argument shall evaluate to a `String`, and the
result shall be a `Long` containing the number of bytes in WFC's current
byte-oriented string representation.

Calls may be nested inside other expressions and may consume string variables
or composed string expressions. Function calls are excluded from the current
constant-expression subset.

This increment establishes parenthesized call-expression syntax; REQ-0163
extends that syntax to comma-separated arguments. Variant coercion, user-defined
functions, qualified calls, named arguments, `LenB`, Unicode character counting,
and other VBA functions remain outside this requirement.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0071` | A call names an unsupported function. |
| `WFC0072` | A supported function receives an empty or incorrect number of arguments. |
| `WFC0073` | A supported string function receives an argument of the wrong type. |
| `WFC0074` | A constant initializer contains a function call. |

Missing closing parentheses continue to use `WFC0005`.

## Verification

- `tests/evaluator_tests.cpp` covers literal, variable, composed, and nested
  expression use plus unsupported-name, arity, type, and constant-expression
  failures.
- `TC-MP0002-len-cli` verifies the function through `wfc --eval`.
