# REQ-0167 — VbCompareMethod expression subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0089, REQ-0159, and REQ-0160

## Requirement

The evaluator shall expose these case-insensitive VBA constants in expressions:

| Constant | Long value |
| --- | ---: |
| `vbBinaryCompare` | 0 |
| `vbTextCompare` | 1 |
| `vbDatabaseCompare` | 2 |

The names are reserved and cannot be redeclared. `InStr` shall accept the
four-argument form `InStr(start, string1, string2, compare)`, and `StrComp`
shall accept `StrComp(string1, string2, compare)`. An explicit binary or text
method shall override module-level `Option Compare` for that call.

`vbDatabaseCompare` is exposed for source compatibility but is not executable
without a database host and shall produce a stable unsupported-method failure
when passed to these functions. Other comparison values shall fail likewise.

The extended `Replace` arguments and runtime type-information publication remain
outside this increment.

## Diagnostics

`WFC0081` reports `vbDatabaseCompare` or any comparison value other than 0 or 1
when used by a supported function. Existing call arity and type diagnostics
remain applicable.

## Verification

- `tests/evaluator_tests.cpp` covers all three values, explicit overrides in
  both directions, type mismatches, and unsupported comparison values.
- `TC-MP0002-compare-method-cli` verifies named explicit text comparison through
  `wfc --eval`.
