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

The selector is compared with the lower bound by `>=` and with the upper
bound by `<=`, each as REQ-0313 says (revised 2026-11-04). Ranges whose lower
bound exceeds their upper bound do not match. Range clauses
participate in the existing ordered first-match and skipped-runtime behavior.

(Revised 2026-11-04: any pair of types can be compared; see REQ-0313.)

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0060` | No longer produced by `Select Case` (REQ-0313). |

## Verification

- `tests/evaluator_tests.cpp` covers numeric and string ranges, mixed lists,
  fallback behavior, and invalid range types.
- `TC-MP0002-select-range-cli` executes an inclusive range through
  `wfc --eval`.
