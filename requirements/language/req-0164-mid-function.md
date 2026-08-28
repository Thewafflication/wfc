# REQ-0164 — Mid function subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0160, REQ-0163, and REQ-0071

## Requirement

`wfc --eval` shall accept case-insensitive `Mid` and `Mid$` calls with either
two or three arguments:

```vb
Mid(<string>, <start>)
Mid(<string>, <start>, <length>)
```

The first argument shall be a `String`; start and length shall be `Long` values.
Start is one-based and must be positive. When length is omitted, the result
extends to the end of the string. A zero length or a start beyond the input
shall return an empty string. An oversized length shall return the available
suffix, and a negative length shall fail.

Indices and lengths count bytes in WFC's current byte-oriented string
representation. The `Mid` assignment statement, `MidB`, Variant coercion, and
named arguments remain outside this requirement.

## Diagnostics

The shared call-expression diagnostics from REQ-0160 and the negative-length
`WFC0075` diagnostic from REQ-0163 apply. `WFC0076` reports a non-positive start.

## Verification

- `tests/evaluator_tests.cpp` covers omitted and present lengths, `$` spelling,
  zero and oversized lengths, beyond-end and invalid starts, type mismatches,
  and arity failures.
- `TC-MP0002-mid-cli` verifies three-argument extraction through `wfc --eval`.
