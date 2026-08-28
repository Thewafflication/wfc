# REQ-0155 — Select Case range subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0153 and REQ-0154

## Requirement

A `Case` branch may contain inclusive `To` ranges, alone or mixed with equality
values in a comma-separated list:

```vb
Case <lower> To <upper>, <value>, ...
```

`Long` ranges use signed numeric ordering and `String` ranges use the current
ordinal string ordering. The selector and both bounds must have the same type.
Ranges whose lower bound exceeds their upper bound do not match. Range clauses
participate in the existing ordered first-match and skipped-runtime behavior.

This increment excludes Boolean ranges and `Case Is` relational clauses.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0060` | Range values differ in type or use unsupported Boolean ordering. |

## Verification

- `tests/evaluator_tests.cpp` covers numeric and string ranges, mixed lists,
  fallback behavior, and invalid range types.
- `TC-MP0002-select-range-cli` executes an inclusive range through
  `wfc --eval`.
