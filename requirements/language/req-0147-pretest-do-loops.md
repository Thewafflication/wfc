# REQ-0147 — Pre-test Do loop subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140 through REQ-0146

## Requirement

`wfc --eval` shall execute pre-test `Do` loops in these forms:

```vb
Do While <Boolean expression>
    <statements>
Loop

Do Until <Boolean expression>
    <statements>
Loop
```

`Do While` executes while its condition is `True`; `Do Until` executes while
its condition is `False`. The condition is evaluated before every iteration.
An initially unselected body is parsed and type-checked but does not produce
output, modify variables, or raise arithmetic runtime failures. Do loops may
nest with existing loops and conditional statements.

This increment excludes post-test `Loop While` / `Loop Until`, `Exit Do`, and
declarations inside loop bodies. A colon cannot replace the required line break
after the condition.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0035` | A `Do While` or `Do Until` condition is not Boolean. |
| `WFC0036` | `Do` is not followed by `While` or `Until`. |
| `WFC0037` | A `Do` block reaches end of input without `Loop`. |
| `WFC0038` | A `Loop` appears outside its matching `Do` block. |
| `WFC0039` | A `Dim` declaration appears inside a `Do` block. |

## Verification

- `tests/evaluator_tests.cpp` covers `While` and `Until` execution, skipped
  bodies, and all REQ-0147 diagnostics.
- `TC-MP0002-do-cli` executes a variable-driven `Do Until` loop through
  `wfc --eval`.
