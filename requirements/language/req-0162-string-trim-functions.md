# REQ-0162 — String trim function subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0160, REQ-0161, and REQ-0071

## Requirement

`wfc --eval` shall accept case-insensitive `Trim`, `LTrim`, and `RTrim` calls,
along with their `Trim$`, `LTrim$`, and `RTrim$` spellings, anywhere an
expression is valid. Each call shall require one `String` argument and return a
`String`.

`LTrim` shall remove consecutive space characters (`0x20`) from the beginning,
`RTrim` shall remove them from the end, and `Trim` shall remove them from both
ends. Empty strings and strings made entirely of spaces shall produce an empty
string. Other whitespace bytes, including tabs, shall remain unchanged.

Variant and Null propagation remain outside this requirement.

## Diagnostics

The shared call-expression diagnostics from REQ-0160 apply.

## Verification

- `tests/evaluator_tests.cpp` covers left, right, and both-end trimming, `$`
  spellings, all-space input, nesting, and preservation of other bytes.
- `TC-MP0002-string-trim-cli` verifies trimming through `wfc --eval`.
