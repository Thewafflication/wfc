# REQ-0208 — Erase statement

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0201, REQ-0207

## Requirement

The evaluator shall recognize the case-insensitive `Erase identifier[,
identifier...]` statement, clearing one or more array variables.

- For a fixed-size array (`Dim identifier(<bound>)`), `Erase` resets every
  element to the array's declared element type's default value; the array
  stays allocated at its original bounds (`LBound`/`UBound` are unchanged).
- For a dynamic array (`Dim identifier()`, see `REQ-0207`), `Erase`
  deallocates it entirely, as if it had never been `ReDim`'d: a subsequent
  `LBound`, `UBound`, or indexed read/write reports `WFC0111`, the same as
  before the array's first `ReDim`. A later `ReDim` reallocates it normally.
- Each target is looked up and cleared independently and in order; targets
  are separated by commas, matching a `Dim`-style identifier list.

## Diagnostics

`WFC0146` reports an `Erase` target that does not exist or does not
currently hold an array (of either kind). `WFC0011` reports a malformed
target (not an identifier).

## Scope

This requirement adds only `Erase` for a variable already holding an
`ArrayValue`. It does not add:

- `Erase` for anything other than an array — there is no other collection
  or reference-counted type in this evaluator that `Erase` affects in real
  VB6 (a `Variant` holding an array is out of scope alongside every other
  `Variant`-element-array exclusion `REQ-0201`/`REQ-0207` already list);
- multi-dimensional array support (tracked separately, not yet
  implemented).

## Verification

- `tests/evaluator_tests.cpp` covers `Erase` resetting a fixed-size array's
  elements in place while preserving its bounds, `Erase` deallocating a
  dynamic array (a subsequent `UBound` reports `WFC0111`), a
  comma-separated multi-target `Erase`, and the `WFC0146` diagnostic for a
  non-array and an undeclared target.
- `TC-MP0002-erase-cli` verifies both the fixed-size in-place reset and the
  dynamic-array deallocation through `wfc --eval`.

## Traceability

This requirement extends `REQ-0201`'s fixed-size array foundation and
`REQ-0207`'s dynamic array foundation with the `Erase` statement both
Scope sections listed as deferred.
