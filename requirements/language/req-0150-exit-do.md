# REQ-0150 — Exit Do subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0147 and REQ-0148

## Requirement

`Exit Do` shall terminate the innermost active pre-test or post-test `Do` loop.
It may appear directly in the loop body or inside nested block conditionals and
`While...Wend` loops. Statements after the executed exit are parsed and
type-checked but do not produce output, modify variables, or raise arithmetic
runtime failures. A post-test loop's trailing condition is likewise checked but
does not control execution after `Exit Do`.

An `Exit Do` in an unselected branch has no runtime effect. `Exit Do` targets
only `Do` loops; `While...Wend` has no exit statement in this increment.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0041` | `Exit` is not followed by `Do`. |
| `WFC0042` | `Exit Do` appears outside an active `Do` loop. |

## Verification

- `tests/evaluator_tests.cpp` covers pre-test and post-test exits, conditional
  exits, innermost-loop targeting, skipped statements, and both diagnostics.
- `TC-MP0002-exit-do-cli` terminates a variable-driven loop through
  `wfc --eval`.
