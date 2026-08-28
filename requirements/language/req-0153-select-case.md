# REQ-0153 — Select Case equality subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140 through REQ-0142

## Requirement

`wfc --eval` shall execute ordered equality branches in this form:

```vb
Select Case <expression>
Case <same-type expression>
    <statements>
[Case <same-type expression>
    <statements>]...
[Case Else
    <statements>]
End Select
```

The selector is evaluated once. Case values are considered in source order and
only the first equal branch executes. `Case Else` is optional and final.
Unselected values and bodies are parsed and type-checked without output,
mutation, or arithmetic runtime failures. Selectors and case values may be
`Long`, `String`, or `Boolean` but must have the same type.

This increment excludes comma-separated values, `Case Is`, ranges, and
declarations inside branch bodies.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0053` | A case value does not match the selector type. |
| `WFC0054` | `Select Case`, an initial `Case`, or `End Select` is incomplete. |
| `WFC0055` | `End` is not followed by `Select`. |
| `WFC0056` | A block contains more than one `Case Else`. |
| `WFC0057` | A `Case` appears after `Case Else`. |
| `WFC0058` | A `Case` appears outside `Select Case`. |

## Verification

- `tests/evaluator_tests.cpp` covers all value types, fallback, skipped runtime
  failures, and structural diagnostics.
- `TC-MP0002-select-case-cli` executes ordered selection through `wfc --eval`.
