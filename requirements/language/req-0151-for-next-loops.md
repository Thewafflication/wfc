# REQ-0151 — For...Next loop subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140 and REQ-0141

## Requirement

`wfc --eval` shall execute counter loops in this form:

```vb
For <declared Long> = <Long start> To <Long end> [Step <nonzero Long>]
    <statements>
Next [<matching variable>]
```

The start, end, and optional step expressions are evaluated once. Bounds are
inclusive; omitted `Step` is `1`, and negative steps iterate downward. The
control variable receives the start value even when the body executes zero
times and receives the next value after normal completion. `For` loops may nest
with existing control-flow statements. An unselected body is parsed and
type-checked without output, mutation, or arithmetic runtime failures.

This increment requires a previously declared `Long` control variable and
excludes `Exit For`, comma-separated `Next` variables, and declarations inside
the loop body.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0043` | A `For` control variable is missing. |
| `WFC0044` | The `To` keyword is missing. |
| `WFC0045` | The control variable, bounds, or step is not `Long`. |
| `WFC0046` | A `For` block reaches end of input without `Next`. |
| `WFC0047` | `Step` is zero or the control variable increment overflows. |
| `WFC0048` | `Next` appears outside a matching `For` block. |
| `WFC0049` | An optional `Next` variable does not match its `For` variable. |
| `WFC0050` | A `Dim` declaration appears inside a `For` block. |

## Verification

- `tests/evaluator_tests.cpp` covers ascending, descending, zero-iteration,
  nested-loop, and diagnostic behavior.
- `TC-MP0002-for-cli` executes an inclusive counter loop through `wfc --eval`.
