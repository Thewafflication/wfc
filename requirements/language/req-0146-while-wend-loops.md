# REQ-0146 — While...Wend loop subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140 through REQ-0145

## Requirement

`wfc --eval` shall execute pre-test loops in this form:

```vb
While <Boolean expression>
    <statements>
Wend
```

The condition is evaluated before each iteration. The body executes while the
condition is `True`; a body whose initial condition is `False` is parsed and
type-checked but does not produce output, modify variables, or raise arithmetic
runtime failures. Nested `While` loops and existing conditional statements are
permitted.

This increment excludes `Exit`, declarations inside loop bodies, `Do...Loop`,
and `For...Next`. A colon cannot replace the line break after the condition.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0031` | A `While` condition is not Boolean. |
| `WFC0032` | A `While` block reaches end of input without `Wend`. |
| `WFC0033` | A `Wend` appears outside its matching `While` block. |
| `WFC0034` | A `Dim` declaration appears inside a `While` block. |

Existing expression, assignment, conditional, and nested-block diagnostics
remain applicable.

## Verification

- `tests/evaluator_tests.cpp` covers repeated mutation, skipped bodies, nested
  loops, and all REQ-0146 diagnostics.
- `TC-MP0002-while-cli` executes a variable-driven loop through `wfc --eval`.
