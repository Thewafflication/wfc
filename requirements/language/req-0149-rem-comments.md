# REQ-0149 — Rem comment subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0141

## Requirement

`wfc --eval` shall recognize case-insensitive `Rem` statements as comments.
Text from `Rem` through the physical line ending is ignored. `Rem` may begin a
line or follow a colon statement separator, and it is reserved from use as a
variable name. Existing apostrophe comments remain unchanged.

This increment does not treat `Rem` as a trailing comment after another
statement unless a colon separates it into its own statement.

## Verification

- `tests/evaluator_tests.cpp` covers line-leading, colon-separated, mixed-case,
  and reserved-identifier behavior.
- `TC-MP0002-rem-cli` executes both comment forms through `wfc --eval`.
