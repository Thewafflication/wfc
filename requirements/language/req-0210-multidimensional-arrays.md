# REQ-0210 — Multi-dimensional arrays (fixed-size only)

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0201

## Requirement

The evaluator shall recognize `Dim identifier(<bound1>, <bound2>, ...) As
Type`, where each comma-separated `<boundN>` is either `<bound>` (meaning
`0 To <bound>`) or `<lower> To <upper>`, using the same bound-expression
grammar and `Long` coercion as the existing one-dimensional form
(`REQ-0201`). Two or more bounds declare a fixed-size array of that many
dimensions; a single bound remains the existing 1-D form.

- `identifier(<index1>, <index2>, ...)` reads or writes one element: the
  number of index expressions must match the array's declared dimension
  count, each coerced to `Long` the same way a 1-D index is, and an index
  outside its own dimension's declared `[lower, upper]` range reports
  `WFC0111`. Elements are stored in row-major order (the last dimension
  varies fastest).
- `LBound(<array>[, <dimension>])` and `UBound(<array>[, <dimension>])`
  accept an optional 1-based `<dimension>` argument (defaulting to `1`),
  returning that dimension's lower/upper bound. A `<dimension>` outside
  `1` through the array's actual dimension count reports `WFC0148`.
- `IsArray`, `TypeName`, and `VarType` behave exactly as they do for a 1-D
  array (VB6 does not encode dimension count in either), and `Erase`
  (`REQ-0208`) and `For Each` (`REQ-0209`) both work unchanged, operating
  on the array's flat element list regardless of dimension count.

## Diagnostics

`WFC0111` reports an out-of-range index in any dimension (read or write).
`WFC0115` reports an index-expression count that does not match the
array's declared dimension count (repurposing the diagnostic that
previously rejected any comma inside an index or `ReDim` bound outright).
`WFC0117` reports a reversed bound in any dimension, reusing the existing
1-D diagnostic. `WFC0148` reports an out-of-range `LBound`/`UBound`
dimension argument. `WFC0016` reports an element-assignment type mismatch,
reusing the existing scalar-assignment diagnostic.

## Scope

This requirement adds fixed-size multi-dimensional arrays only. It does
not add:

- dynamic multi-dimensional arrays: `Dim identifier(,)` (or any bound-less
  multi-dimensional form) is not a recognized declaration, and `ReDim`/
  `ReDim Preserve` (`REQ-0207`) continue to reject any comma in their own
  bound list with `WFC0115`, exactly as before this requirement. A
  multi-dimensional array's shape is therefore fixed for its entire
  lifetime, the same as a 1-D fixed-size array;
- three or more dimensions being independently verified against the
  reference runtime — the flattening formula is a standard row-major
  generalization exercised by this requirement's own 2-D and 3-D-shaped
  tests, but higher dimension counts were not separately probed;
- `Variant`/`Object`-element multi-dimensional arrays were excluded here
  too; `REQ-0212` later added both, for a multi-dimensional array the
  same as a 1-D one. A multi-dimensional array *can* be passed to an
  array-typed parameter (`REQ-0211`), since that parameter form is
  dimension-count-agnostic;
- `Option Base 1` — a bound-less dimension in any position still means
  `0 To <bound>`, matching `REQ-0201`'s existing 1-D rule.

## Verification

- `tests/evaluator_tests.cpp` covers a 2-D array with default (`0 To n`)
  bounds and with explicit `To` bounds in every dimension, indexed
  read/write, `LBound`/`UBound` with and without a dimension argument,
  `TypeName`/`VarType`/`IsArray` (unchanged from the 1-D case), `For Each`
  summing every element of a 2-D array (verifying row-major flat
  iteration), `Erase` on a 2-D array, an out-of-range index (`WFC0111`), a
  too-few/too-many index count (`WFC0115`), an out-of-range `LBound`/
  `UBound` dimension (`WFC0148`), and `ReDim` still rejecting a
  multi-dimensional bound (`WFC0115`).
- `TC-MP0002-multidim-array-cli` verifies declaration, indexed read/write,
  and dimension-argument `LBound`/`UBound` through `wfc --eval`.

## Traceability

This requirement extends `REQ-0201`'s fixed-size array foundation with the
multi-dimensional form its own Scope section listed as deferred, and
integrates unchanged with `REQ-0208`'s `Erase` and `REQ-0209`'s
`For Each`, both of which operate on `ArrayValue`'s flat element list
without needing dimension-aware logic of their own.
