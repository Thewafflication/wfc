# REQ-0157 — Typed constant subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140 through REQ-0142

## Requirement

`wfc --eval` shall declare immutable module constants in this form:

```vb
Const <name> As Long = <constant expression>
Const <name> As String = <constant expression>
Const <name> As Boolean = <constant expression>
```

Initializers may use literals, operators, parentheses, and previously declared
constants. The initializer type must exactly match the declared type. Constant
names are case-insensitive, participate in expressions, and cannot be assigned
after declaration.

This increment excludes inferred types, multiple declarations per statement,
procedure-local constants, and initializers that reference mutable variables.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0062` | An assignment targets a constant. |
| `WFC0063` | A constant declaration appears inside a control-flow block. |
| `WFC0064` | A constant initializer references a mutable variable. |

Existing declaration, duplicate-name, reserved-name, initializer, and
type-mismatch diagnostics remain applicable.

## Verification

- `tests/evaluator_tests.cpp` covers every supported type, constant references,
  immutability, scope rejection, and mutable-reference rejection.
- `TC-MP0002-const-cli` evaluates a derived constant through `wfc --eval`.
