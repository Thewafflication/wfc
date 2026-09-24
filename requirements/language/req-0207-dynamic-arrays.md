# REQ-0207 — Dynamic arrays: ReDim and ReDim Preserve

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0201

## Requirement

The evaluator shall recognize `Dim identifier() As Type` (no bound expression)
as a dynamic, one-dimensional array: declared but unallocated until its first
`ReDim`. The case-insensitive `ReDim [Preserve] identifier(<bound>)` /
`ReDim [Preserve] identifier(<lower> To <upper>)` statement then allocates or
reallocates it, using the same bound-expression grammar, `Long` coercion, and
`<lower> <= <upper>` validation as `Dim`'s own fixed-size array form
(`REQ-0201`).

- `ReDim` is an executable statement, not a declaration: it may appear
  anywhere a statement may (including inside `If`/`While`/`Do`/`For` blocks,
  unlike `Dim`/`Const`/`Static`), and it never carries an `As Type` clause —
  the element type is fixed by the array's original `Dim` and is not
  re-specified.
- `ReDim`'s target must already be a variable declared `Dim identifier()`
  (a dynamic array). Reporting `WFC0145`: the target does not exist, does
  not currently hold an array, or holds a *fixed*-size array (one declared
  `Dim identifier(<bound>)`, which `ReDim` may never resize, matching real
  VB6).
- Without `Preserve`, `ReDim` discards the array's current contents (if any)
  and allocates a new range with every element reset to the element type's
  default value — even if the new bounds are identical to the old ones.
- With `Preserve`, `ReDim` allocates the new range and copies every element
  whose absolute index falls within both the old and new bounds; an index
  only in the new range is reset to the default value, and an index only in
  the old range is dropped. `Preserve` on an array's first-ever `ReDim`
  (nothing yet allocated) behaves exactly like a plain `ReDim`.
- Reading `LBound`, `UBound`, or an element of a dynamic array that has not
  yet been `ReDim`'d reports `WFC0111`, the same "subscript out of range"
  diagnostic an ordinary out-of-bounds index reports — matching real VB6,
  which raises the same run-time error 9 for both cases. This is distinct
  from a legitimately zero-length allocated array (for example a
  `ParamArray` called with no extra arguments), which answers `LBound`/
  `UBound` normally (`UBound < LBound`) without error.
- `IsArray`, `TypeName`, and `VarType` recognize a dynamic array (allocated
  or not) as an array of its declared element type, the same as a
  fixed-size array.

## Diagnostics

`WFC0145` reports a `ReDim` whose target is not a previously declared
dynamic array (undeclared, non-array, or fixed-size). `WFC0111` reports an
out-of-range index, or `LBound`/`UBound` called on an unallocated dynamic
array, reusing `REQ-0201`'s existing diagnostic. `WFC0117` reports a `ReDim`
whose lower bound exceeds its upper bound, reusing `REQ-0201`'s existing
declaration-time diagnostic. `WFC0115` originally reported any comma inside
a `ReDim` bound expression (multi-dimensional, out of scope); `REQ-0219`
repurposes it to instead report a `ReDim` whose dimension count does not
match the array's already-fixed dimension count, once dynamic
multi-dimensional arrays exist. `WFC0116`
("`Dim identifier()` with no bound is unsupported") is retired: that form is
now the dynamic-array declaration this requirement adds.

## Scope

This requirement adds only `ReDim`/`ReDim Preserve` for a one-dimensional
array. It does not add:

- multiple dimensions (`ReDim arr(i, j)`) — originally excluded here (a
  comma inside `(...)` unconditionally reported `WFC0115`, even after
  `REQ-0210` added fixed-size multi-dimensional arrays); `REQ-0219` later
  added dynamic multi-dimensional arrays, generalizing this requirement's
  `ReDim`/`ReDim Preserve` machinery to N dimensions;
- `Erase` (see `REQ-0208`), `For Each` iteration over an array (see
  `REQ-0209`), or array-typed `Sub`/`Function` parameters (see
  `REQ-0211`) — all three are now covered by their own requirements;
- `Variant`- or `Object`-element dynamic arrays were originally excluded
  here too; `REQ-0212` later added both for a dynamic array the same as a
  fixed-size one;
- `ReDim` as an *implicit* declaration (real VB6 allows `ReDim x(5)` with no
  prior `Dim` at procedure scope, treating it as the declaration itself).
  This evaluator's `ReDim` always requires a prior `Dim identifier()`;
- a dynamic array as a class field — this evaluator has no array-typed class
  fields at all yet (`REQ-0203`'s Scope), dynamic or fixed.

## Verification

- `tests/evaluator_tests.cpp` covers `Dim arr() As Type` (unallocated,
  `IsArray`/`TypeName` still correct), `WFC0111` for `LBound`/`UBound`/
  indexed read/write on an unallocated array, `ReDim` allocating and
  populating a `Long` and a `String` array, `ReDim Preserve` growing
  (old values kept, new slots defaulted) and shrinking (surviving values
  kept, dropped values gone), a plain `ReDim` resetting previously-written
  values, `ReDim`/`ReDim Preserve` on an array's first allocation, `ReDim`
  inside an `If` block, and the `WFC0145`/`WFC0117`/`WFC0115` diagnostics
  (fixed-size target, undeclared target, non-array target, reversed bound,
  and a comma).
- `TC-MP0002-redim-cli` verifies declaration, `ReDim`, and `ReDim Preserve`
  (growing) through `wfc --eval`.

## Traceability

This requirement extends `REQ-0201`'s fixed-size array foundation with the
`ReDim`/`ReDim Preserve` dynamic resizing its own Scope section explicitly
deferred, and retires the `WFC0116` diagnostic that form previously reported.
