# REQ-0148 — Post-test Do loop subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140 through REQ-0147

## Requirement

`wfc --eval` shall execute post-test `Do` loops in these forms:

```vb
Do
    <statements>
Loop While <Boolean expression>

Do
    <statements>
Loop Until <Boolean expression>
```

The body executes once before the condition is tested. `Loop While` repeats
while its condition is `True`; `Loop Until` repeats while its condition is
`False`. Post-test loops may nest with existing loops and conditionals. When an
enclosing branch is not selected, the body and condition are parsed and
type-checked once without output, mutation, or arithmetic runtime failures.

This increment excludes unconditional `Do...Loop`, `Exit Do`, and declarations
inside loop bodies.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0035` | A post-test `Loop While` or `Loop Until` condition is not Boolean. |
| `WFC0040` | A post-test `Loop` is not followed by `While` or `Until`. |

REQ-0147 diagnostics continue to cover missing or unmatched `Loop` tokens and
declarations inside the body.

## Verification

- `tests/evaluator_tests.cpp` covers guaranteed first execution, `While` and
  `Until` repetition, and post-test diagnostics.
- `TC-MP0002-posttest-do-cli` executes a post-test loop through `wfc --eval`.
