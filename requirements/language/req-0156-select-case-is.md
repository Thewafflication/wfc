# REQ-0156 — Select Case Is subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0153 through REQ-0155

## Requirement

A `Case` branch may contain relational clauses, alone or mixed with equality
values and ranges:

```vb
Case Is < <value>, Is >= <value>, ...
```

Supported operators are `=`, `<>`, `<`, `<=`, `>`, and `>=`. Values must match
the selector type. `Long` and `String` support every operator; `Boolean`
supports equality and inequality only. Clauses participate in ordered
first-match and skipped-runtime behavior.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0061` | `Case Is` is not followed by a relational operator. |

Existing `WFC0053` type and `WFC0018` Boolean-ordering diagnostics apply.

## Verification

- `tests/evaluator_tests.cpp` covers numeric, string, and Boolean clauses,
  mixed lists, and invalid operators.
- `TC-MP0002-select-is-cli` executes relational selection through `wfc --eval`.
