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

Supported operators are `=`, `<>`, `<`, `<=`, `>`, and `>=`. The selector
and the value are compared as REQ-0313 says (revised 2026-11-04); every
operator works with every type. Clauses participate in ordered
first-match and skipped-runtime behavior.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0061` | `Case Is` is not followed by a relational operator. |

`WFC0053` and `WFC0018` no longer apply to a Case value (REQ-0313).

## Verification

- `tests/evaluator_tests.cpp` covers numeric, string, and Boolean clauses,
  mixed lists, and invalid operators.
- `TC-MP0002-select-is-cli` executes relational selection through `wfc --eval`.
