# REQ-0145 — Block ElseIf branch subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140 through REQ-0144

## Requirement

`wfc --eval` shall execute ordered `ElseIf` branches in block conditionals:

```vb
If <Boolean expression> Then
    <statements>
[ElseIf <Boolean expression> Then
    <statements>]...
[Else
    <statements>]
End If
```

Conditions are considered in source order and only the first matching branch
executes. Once a branch is selected, later conditions and all unselected bodies
are parsed and type-checked but do not produce output, modify variables, or
raise arithmetic runtime failures. An optional `Else` remains the final branch.

Existing nested block conditionals, REQ-0143 single-line conditionals, statement
separators, comments, and the REQ-0144 prohibition on branch-local declarations
remain unchanged. This increment does not add single-line `ElseIf` syntax.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0028` | An `ElseIf` condition is not Boolean. |
| `WFC0029` | An `ElseIf` condition is not followed by `Then`. |
| `WFC0030` | An `ElseIf` appears after the block's `Else` branch. |

REQ-0144 structural diagnostics remain applicable to the enclosing block.

## Verification

- `tests/evaluator_tests.cpp` covers ordered selection, fallback behavior,
  skipped runtime failures, and all REQ-0145 diagnostics.
- `TC-MP0002-elseif-cli` executes an ordered `ElseIf` chain through
  `wfc --eval`.
