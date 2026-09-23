# REQ-0212 — Variant- and Object-element arrays

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0197, REQ-0200, REQ-0201, REQ-0207, REQ-0210

## Requirement

The evaluator shall recognize `Dim identifier(...) As Variant` and
`Dim identifier(...) As Object` for a fixed-size, dynamic, or
multi-dimensional array (`REQ-0201`/`REQ-0207`/`REQ-0210`), giving the
array's elements per-element `Variant`/`Object` behavior matching the
corresponding scalar type.

- A `Variant`-element array's element retypes freely on plain `arr(i) =
  expr` assignment, across the same value types a scalar `Variant`
  accepts (`REQ-0197`) — no fixed per-element type is enforced. Each
  element initializes to `Empty`.
- An `Object`-element array's element holds `Nothing` or a live object
  reference (`REQ-0200`), assigned only through `Set arr(i) = expr`; a
  plain `arr(i) = expr` reports `WFC0108`, the same as a scalar `Object`
  variable. Each element initializes to `Nothing`.
- `LBound`, `UBound`, indexed read, `Erase` (`REQ-0208`), `For Each`
  (`REQ-0209`), and a multi-dimensional shape (`REQ-0210`) all work the
  same as for any other array kind; `ReDim`/`ReDim Preserve` (`REQ-0207`)
  refill a newly-created slot with `Empty`/`Nothing` as appropriate.
- `TypeName` renders the array as `"Variant()"`/`"Object()"` (the array's
  own declared kind, not any particular element's current runtime type);
  `VarType` returns `vbVariant Or vbArray` (`8204`) or `vbObject Or
  vbArray` (`8201`).
- The array *variable* itself keeps the existing array assignment/copy
  behavior (`REQ-0201`'s disclosed `arr1 = arr2` simplification): a
  Variant-element array is not itself added to the Variant-retyping
  tracking a scalar `Variant` variable uses, and an Object-element array
  is not itself added to the Set-only tracking a scalar `Object` variable
  uses — those apply to the array's *elements*, not to the array value as
  a whole.

## Diagnostics

`WFC0108` reports a plain `=` assignment to an `Object`-element array's
element (`Set` required). `WFC0111`/`WFC0115`/`WFC0117`/`WFC0148` report
the same indexing/declaration diagnostics every other array kind reports,
unchanged. `WFC0106`/`WFC0137` report `Set`'s existing object-reference/
class-mismatch checks, reused for an `Object`-element array's element.

## Scope

This requirement adds per-element `Variant`/`Object` behavior for an
array declared `As Variant`/`As Object`. It does not add:

- a class-typed array element (`Dim arr() As SomeClass`) — only the
  generic `As Object` form is covered, matching `REQ-0200`'s own
  generic-vs-class-typed distinction for scalars;
- an array-typed `Sub`/`Function` parameter (`REQ-0211`) declared
  `nums() As Variant`/`As Object` — `REQ-0211`'s parameter parser still
  requires a concrete scalar element type, the same restriction
  `ParamArray` (`REQ-0206`) already has;
- `Class_Terminate` participating in an `Object`-element array's
  lifetime tracking. A scalar `Object`/`Variant` variable's assignment
  path calls `terminate_if_last_reference` before overwriting; an array
  element's assignment does not, so an instance reachable only through an
  array element is not proactively terminated when that element is
  overwritten, `Erase`d, or the array goes out of scope. This mirrors an
  existing, disclosed gap in `REQ-0204`'s own lifetime-tracking scope
  (cascading through a field), now extended to array elements rather than
  independently solved here;
- `IsArray`/`TypeName`/`VarType` reflecting a `Variant` element's *current*
  runtime type when asked about the whole array (`TypeName(arr)` always
  answers `"Variant()"`, matching real VB6, not the type of any specific
  element — use `TypeName(arr(i))` for that).

## Verification

- `tests/evaluator_tests.cpp` covers a `Variant`-element array retyping
  across `Long`/`String`/`Boolean` per element with correct per-element
  `TypeName` and array-level `TypeName`/`VarType`, an `Object`-element
  array's `Nothing` default and `Set`-assigned identity (`Is`) both at
  module level and through a class instance, a plain `=` rejection on an
  `Object` element (`WFC0108`, both a literal and an existing instance),
  a dynamic `Variant`-element array's `ReDim Preserve` refilling new slots
  with `Empty`, a multi-dimensional `Object`-element array, and
  whole-array assignment between two `Variant`-element arrays.
- `TC-MP0002-variant-object-array-cli` verifies per-element retyping and
  the `Object`-element default through `wfc --eval`.

## Traceability

This requirement extends `REQ-0201`/`REQ-0207`/`REQ-0210`'s array
foundation with the `Variant`/`Object` element types their own Scope
sections listed as deferred, reusing `REQ-0197`'s Variant-retyping
semantics and `REQ-0200`'s Object/`Set` semantics at element granularity.
