# REQ-0144 — Block conditional statement subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0141, REQ-0142, and REQ-0143

## Requirement

`wfc --eval` shall execute block conditionals in this form:

```vb
If <Boolean expression> Then
    <statements>
[Else
    <statements>]
End If
```

The selected block executes and the other block is parsed and type-checked but
does not produce output, modify variables, or raise arithmetic runtime failures.
Nested block conditionals are permitted. Existing statement separators, comments,
and REQ-0143 single-line conditionals remain supported.

This increment excludes `ElseIf`, branch-local declarations, and block syntax
using `:` in place of the line break after `Then`, `Else`, or before `End If`.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0024` | A block conditional reaches end of input without `End If`. |
| `WFC0025` | `End` is not followed by `If`. |
| `WFC0026` | A block conditional contains more than one `Else`. |
| `WFC0027` | A block conditional contains a `Dim` declaration. |

REQ-0143 diagnostics remain applicable to the condition and `Then` marker.

## Verification

- `tests/evaluator_tests.cpp` covers selected and unselected blocks, nested
  blocks, and all REQ-0144 diagnostics.
- `TC-MP0002-block-conditional-cli` runs a multiline conditional program
  through `wfc --eval`.
