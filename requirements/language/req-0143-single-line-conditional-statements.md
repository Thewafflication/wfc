# REQ-0143 — Single-line conditional statement subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0141, and REQ-0142

## Requirement

`wfc --eval` shall execute a single-line conditional statement in this form:

```vb
If <Boolean expression> Then <branch> [Else <branch>]
```

The condition shall use the REQ-0142 expression grammar and have type
`Boolean`. The selected branch executes; the unselected branch is parsed and
type-checked but does not produce output, modify variables, or raise arithmetic
runtime failures. A branch shall be either `Print <expression>` or an assignment
to an existing variable, optionally introduced with `Let`.

`Else` is optional. Conditional statements participate in normal line-ending
and `:` statement separation. This increment deliberately excludes block
conditionals, `ElseIf`, nested conditional branches, declarations in branches,
and multi-statement branches.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0021` | The `If` condition is not Boolean. |
| `WFC0022` | `Then` is absent after the condition. |
| `WFC0023` | A required branch is absent or is neither `Print` nor an assignment. |

Existing expression and assignment diagnostics remain applicable to selected
and unselected branches where their syntax or types are invalid. Runtime
arithmetic failures in an unselected branch are suppressed.

## Verification

- `tests/evaluator_tests.cpp` covers selected `Then` and `Else` branches,
  assignment, suppressed unselected-branch output/mutation/runtime failure, and
  all REQ-0143 diagnostics.
- `TC-MP0002-conditional-cli` runs a conditional program through `wfc --eval`.
