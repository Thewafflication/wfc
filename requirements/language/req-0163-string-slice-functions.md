# REQ-0163 — String slice function subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0160 through REQ-0162, and REQ-0071

## Requirement

`wfc --eval` shall accept case-insensitive `Left` and `Right` calls, along with
their `Left$` and `Right$` spellings, anywhere an expression is valid. Each call
shall require a `String` followed by a `Long` length and return a `String`.

The length counts bytes in WFC's current byte-oriented string representation. A
zero length shall return an empty string. A length greater than the input size
shall return the entire input. A negative length shall fail.

Call-expression parsing shall accept comma-separated argument expressions and
shall preserve nested-call grouping. Empty arguments and incorrect argument
counts shall fail deterministically.

Variant coercion, named or optional arguments, and `LeftB`/`RightB` remain
outside this requirement.

## Diagnostics

The shared call-expression diagnostics from REQ-0160 apply. `WFC0075` reports a
negative string length.

## Verification

- `tests/evaluator_tests.cpp` covers left and right extraction, `$` spellings,
  computed, zero, oversized, and negative lengths, type mismatches, arity, and
  empty arguments.
- `TC-MP0002-string-slice-cli` verifies both functions through `wfc --eval`.
