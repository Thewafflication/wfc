# REQ-0166 — StrReverse function subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0160 and REQ-0071

## Requirement

`wfc --eval` shall accept a case-insensitive `StrReverse(<expression>)` call
anywhere an expression is valid. The call shall require one `String` argument
and return a `String` containing the input bytes in reverse order.

An empty input shall return an empty string. Calls may be nested with other
supported string functions. Reversal is byte-oriented under the current string
representation; Unicode code-point and grapheme-aware reversal, Null
propagation, and Variant coercion remain outside this requirement.

## Diagnostics

The shared call-expression arity, type, and constant-initializer diagnostics
from REQ-0160 apply.

## Verification

- `tests/evaluator_tests.cpp` covers ordinary, empty, punctuation, and nested
  calls.
- `TC-MP0002-string-reverse-cli` verifies reversal through `wfc --eval`.
