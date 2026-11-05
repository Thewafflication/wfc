# REQ-0154 — Select Case value-list subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0153

## Requirement

A `Case` branch may contain a comma-separated list of same-type equality
expressions:

```vb
Select Case <expression>
Case <value>, <value>, ...
    <statements>
End Select
```

Values are considered from left to right. A branch matches when any value is
equal to the selector. Values after a match and values in later branches are
parsed and type-checked without raising arithmetic runtime failures. A value
is compared as REQ-0313 says; the values after a match are not evaluated.

This increment excludes ranges and `Case Is` relational clauses.

## Diagnostics

| Code | Condition |
| --- | --- |
| `WFC0053` | No longer produced by `Select Case` (REQ-0313). |
| `WFC0059` | A value is missing before or after a comma. |

## Verification

- `tests/evaluator_tests.cpp` covers numeric and string lists, short-circuited
  runtime failures, and missing list values.
- `TC-MP0002-select-list-cli` executes a value-list branch through
  `wfc --eval`.
