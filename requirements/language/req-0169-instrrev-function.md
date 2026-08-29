# REQ-0169 — InStrRev function subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0159, REQ-0167, and REQ-0071

## Requirement

`InStrRev(stringCheck, stringMatch[, start[, compare]])` shall search from right
to left and return the one-based position of the last eligible match, or zero
when no match exists.

`start` defaults to -1, meaning the end of `stringCheck`; an explicit start must
be -1 or positive. A match is eligible only when its final character is at or
before start. A start beyond the input and an empty input return zero. An empty
match returns the effective start.

Comparison defaults to module-level `Option Compare` within the current WFC
execution model. Explicit `vbBinaryCompare` or `vbTextCompare` overrides it;
unsupported methods fail with `WFC0081`. Search remains byte-oriented and uses
the current ASCII-only text comparison.

Null/Variant coercion and named or omitted middle arguments remain outside this
requirement.

## Diagnostics

Existing call, type, and comparison-method diagnostics apply. `WFC0083` reports
a start below -1 or equal to zero.

## Verification

- `tests/evaluator_tests.cpp` covers default and bounded reverse searches,
  match-end eligibility, empty values, explicit/default compare modes, arity,
  type failures, and invalid starts.
- `TC-MP0002-instrrev-cli` verifies the reverse search through `wfc --eval`.

## References

- [Microsoft VBA `InStrRev` reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/instrrev-function)
- [MS-VBAL `InStrRev` runtime semantics](https://learn.microsoft.com/en-us/openspecs/microsoft_general_purpose_programming_languages/ms-vbal/9d45a100-3a6d-4cb6-aa09-772ccffe9706)
