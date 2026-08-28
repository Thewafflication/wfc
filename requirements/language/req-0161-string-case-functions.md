# REQ-0161 — String case function subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0159, REQ-0160, and REQ-0071

## Requirement

`wfc --eval` shall accept case-insensitive `LCase(<expression>)` and
`UCase(<expression>)` calls, along with their `LCase$` and `UCase$` spellings,
anywhere an expression is valid. Each call shall require one `String` argument
and return a `String`.

For the current byte-oriented string subset, the calls shall map only ASCII
`A` through `Z` or `a` through `z`. Digits, punctuation, and bytes outside the
ASCII letter ranges shall remain unchanged. Calls may be nested and may consume
composed string expressions.

Locale-sensitive casing, Variant coercion, and full Unicode case mapping remain
outside this requirement.

## Diagnostics

The shared call-expression diagnostics from REQ-0160 apply: `WFC0071` for an
unsupported function, `WFC0072` for invalid arity, `WFC0073` for a non-string
argument, and `WFC0074` for a call in a constant initializer.

## Verification

- `tests/evaluator_tests.cpp` covers both return spellings, mixed-case function
  names, variables, nesting, unchanged nonletters, and argument rejection.
- `TC-MP0002-string-case-cli` verifies both functions through `wfc --eval`.
