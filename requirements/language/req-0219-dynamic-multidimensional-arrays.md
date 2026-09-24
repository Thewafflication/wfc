# REQ-0219 — Dynamic multi-dimensional arrays

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0207, REQ-0210

## Requirement

The evaluator shall recognize a dynamic array whose dimension count is
greater than one, combining `REQ-0207`'s dynamic (`ReDim`-allocated) array
with `REQ-0210`'s multi-dimensional array.

- `Dim identifier()` (no bounds, no commas) declares a dynamic array whose
  dimension count is unconstrained until its first `ReDim`: that first
  `ReDim`'s own comma count fixes the dimension count for the rest of the
  array's lifetime.
- `Dim identifier(,)`, `Dim identifier(,,)`, etc. — N commas and no bound
  expressions — pre-declare a dynamic array with a fixed dimension count
  (N+1) but no bounds yet; it is unallocated until its first `ReDim`, the
  same as a 1-D `Dim identifier()`.
- Once an array's dimension count is fixed (by comma pre-declaration or by
  its first `ReDim`), every later `ReDim` on that array must supply exactly
  that many comma-separated bounds, or report `WFC0115`.
- `ReDim Preserve` on a multi-dimensional array may only change the *last*
  dimension's bounds; every other dimension's `[lower, upper]` range must
  be byte-for-byte identical to the array's current shape, or the statement
  reports `WFC0151` and leaves the array unchanged. Preserving copies every
  element whose full index tuple falls within both the old and new last
  dimension (with every other dimension's range held fixed), the same
  overlap rule `REQ-0207`'s 1-D `ReDim Preserve` already uses, generalized
  to the row-major flat layout `REQ-0210` established.
- `Erase` on a dynamic multi-dimensional array clears its per-dimension
  bounds along with its elements (see `REQ-0208`'s updated Requirement
  section), leaving the dimension count locked but every bound forgotten;
  a subsequent `LBound`, `UBound`, or indexed read/write reports `WFC0111`,
  matching the existing 1-D dynamic-array behavior.
- `LBound`/`UBound` on an unallocated dynamic array (whether or not its
  dimension count has been pre-declared via commas) reports `WFC0111` for
  every dimension argument, not just dimension 1 — the array's own
  dimension count is not yet an authoritative shape to validate a
  dimension argument against until it is actually allocated.

## Diagnostics

`WFC0115` reports a `ReDim` whose comma-separated bound count does not
match the array's already-fixed dimension count — extending the meaning
`REQ-0210` gave this code for indexing to `ReDim` as well, replacing
`REQ-0207`'s narrower original behavior of unconditionally rejecting any
comma in a `ReDim` bound list. `WFC0151` (new) reports a `ReDim Preserve`
that attempts to change any dimension other than the last on a
multi-dimensional array. `WFC0111` reports `LBound`/`UBound`, or an
indexed read/write, on an unallocated dynamic array — reused unchanged
from `REQ-0207`.

## Scope

This requirement adds dynamic multi-dimensional arrays: comma
pre-declaration, dimension-count locking, and `ReDim`/`ReDim Preserve`/
`Erase` generalized to N dimensions. It does not add:

- `ReDim` as an *implicit* first declaration — `REQ-0207`'s existing
  exclusion (a prior `Dim identifier()` or `Dim identifier(,...)` is still
  required) is unchanged;
- `Option Base 1`, or any bound-less dimension meaning anything other than
  `0 To <bound>` — unchanged from `REQ-0201`/`REQ-0210`;
- a dynamic multi-dimensional array as a class field — this evaluator has
  no array-typed class fields at all yet (`REQ-0203`'s Scope);
- verifying `ReDim Preserve`'s last-dimension-only restriction, or the
  comma pre-declaration form, against the reference runtime beyond this
  requirement's own tests and the documented VB6 behavior described in the
  Reference below — no live Microsoft VBA/VB6 instance was used to confirm
  edge cases such as `ReDim Preserve` on a 4+ dimensional array.

## Verification

- `tests/evaluator_tests.cpp` covers: `Dim arr()` fixing its dimension
  count on first `ReDim` (2-D, with mixed 0-based and explicit indices);
  `Dim arr(,)` pre-declaring 2 dimensions before any `ReDim`; a later
  `ReDim` with the wrong dimension count reporting `WFC0115` (both for a
  comma-pre-declared array and for one whose count was fixed by its first
  `ReDim`); `ReDim Preserve` growing and shrinking a 2-D array's last
  dimension while keeping overlapping elements and defaulting new ones;
  `ReDim Preserve` attempting to resize a non-last dimension reporting
  `WFC0151`; and the `Erase`-then-reindex regression this requirement's
  bug fix addresses (`Erase` on a 2-D dynamic array followed by an indexed
  read reporting `WFC0111` instead of hanging, and a subsequent `ReDim`
  with the wrong dimension count still reporting `WFC0115`).
- `TC-MP0002-dynamic-multidim-array-cli` verifies declaration, `ReDim`,
  indexed read/write, `ReDim Preserve` growing the last dimension, and
  `Erase` followed by a same-dimension-count `ReDim`, through `wfc --eval`.

## Reference

- [Microsoft VBA `ReDim` statement reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/redim-statement)

## Traceability

This requirement combines `REQ-0207`'s dynamic-array `ReDim`/`ReDim
Preserve` machinery with `REQ-0210`'s multi-dimensional array shape,
generalizing both to N dimensions behind one shared code path, and fixes a
bug in `REQ-0208`'s `Erase` that this requirement's work uncovered (see
`REQ-0208`'s updated Requirement and Scope sections).
