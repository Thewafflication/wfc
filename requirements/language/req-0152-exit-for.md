# REQ-0152 — Exit For subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0151

## Requirement

`Exit For` shall terminate the innermost active `For...Next` loop. It may
appear directly in the loop body or inside nested conditionals and loop blocks.
Statements after an executed exit are parsed and type-checked but do not
produce output, modify variables, or raise arithmetic runtime failures.

An `Exit For` in an unselected branch has no runtime effect. In nested
`For...Next` loops, the exit terminates only the innermost loop.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0041` | `Exit` is not followed by `Do` or `For`. |
| `WFC0052` | `Exit For` appears outside an active `For` loop. |

## Verification

- `tests/evaluator_tests.cpp` covers conditional exits, skipped statements,
  innermost-loop targeting, and the outside-loop diagnostic.
- `TC-MP0002-exit-for-cli` terminates a counter loop through `wfc --eval`.
