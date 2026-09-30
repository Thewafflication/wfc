# REQ-0239 — `Array`, `Split`, `Join`, `Filter`

## Statement

- `Array(v0, v1, ...)` returns a zero-based Variant array.
- `Split(text[, delimiter = " "[, limit = -1[, compare]]])` returns a
  zero-based `String` array; `Split("")` (or limit 0) returns an empty array
  (`UBound = -1`). `compare` is accepted but splitting is always binary.
- `Join(array[, delimiter = " "])` concatenates the elements' string forms.
- `Filter(array, match[, include = True[, compare]])` returns the elements
  containing (or, with `include = False`, not containing) `match`;
  `compare = 1` (`vbTextCompare`) or `Option Compare Text` ignores ASCII case.

## Scope

One-dimensional arrays only; element text uses the evaluator's `CStr`
rendering. `Null` or object elements in `Join` are rejected (`WFC0073`).

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-array-split-join-cli`.
