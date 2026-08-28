# REQ-0158 — Option Explicit subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0141 and REQ-0149

## Requirement

`wfc --eval` shall recognize one case-insensitive module directive:

```vb
Option Explicit
```

The directive may follow blank lines, apostrophe comments, and `Rem` comments,
but must precede declarations and executable statements. It may appear at most
once and only at module level. The current evaluator already requires every
variable reference to resolve to an explicit declaration, so the directive
records and enforces source intent without changing that existing strict rule.

Other `Option` directives remain outside this increment.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0065` | `Option` is not followed by `Explicit`. |
| `WFC0066` | `Option Explicit` follows a declaration or executable statement. |
| `WFC0067` | A module contains more than one `Option Explicit`. |
| `WFC0068` | `Option Explicit` appears inside a control-flow block. |

## Verification

- `tests/evaluator_tests.cpp` covers leading comments, placement, duplication,
  unsupported options, and block rejection.
- `TC-MP0002-option-explicit-cli` executes a strict module through
  `wfc --eval`.
