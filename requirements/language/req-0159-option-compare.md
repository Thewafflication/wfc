# REQ-0159 — Option Compare subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0142, REQ-0153 through REQ-0156, and REQ-0158

## Requirement

`wfc --eval` shall recognize one module-level string comparison directive:

```vb
Option Compare Binary
Option Compare Text
```

`Binary` is the default and compares string bytes ordinally. `Text` performs
case-insensitive ASCII comparison for the current execution subset. The chosen
mode applies consistently to string equality, ordering, `Select Case` equality
clauses, ranges, and `Case Is` relations.

The directive follows the same leading-comment and pre-body placement rules as
`Option Explicit`; each module may contain at most one `Option Compare` and may
combine it with one `Option Explicit` in either order.

Locale-aware text comparison and `Option Compare Database` are excluded.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0069` | A module contains more than one `Option Compare`. |
| `WFC0070` | `Option Compare` is not followed by `Binary` or `Text`. |

Existing option placement and block-scope diagnostics remain applicable.

## Verification

- `tests/evaluator_tests.cpp` covers default/binary and text equality,
  ordering, ranges, directive combination, duplication, and unsupported modes.
- `TC-MP0002-option-compare-cli` verifies text comparison through `wfc --eval`.
