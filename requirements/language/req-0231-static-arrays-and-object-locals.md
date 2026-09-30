# REQ-0231 — Static arrays and class-typed/Object locals

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0201, REQ-0203, REQ-0206, REQ-0210, REQ-0226, REQ-0228, REQ-0230

## Requirement

The evaluator shall extend `Static` (`REQ-0206`) beyond the fixed-scalar/
`Variant` forms it originally supported alone, to two more forms real
VB6 allows:

- **`Static name As Object` / `As SomeClassName`.** Accepts an object
  reference the same way a class-typed field, return type, or parameter
  already does (`REQ-0203`/`REQ-0228`): `Nothing` or (for a specific
  class) exactly an instance of that class, with a mismatch reporting
  `WFC0137`. The two `Class_Terminate` lifetime fixes `REQ-0230` already
  made for a `Static Variant` holding an object reference apply here
  automatically, with no further code — both fixes operate on any
  object-holding `Static`, not specifically a `Variant`-declared one.
- **`Static name(<bounds>) As Type`.** A fixed-size array, 1-D or
  multi-dimensional, using the exact same bound grammar `Dim`'s own
  fixed-size array form already accepts (`REQ-0201`/`REQ-0210`,
  including `Option Base`'s effect on a bound-less dimension,
  `REQ-0226`) — persisted and copied element-for-element the same way a
  scalar `Static`'s single value already is. Unlike `Dim`, `Static` has
  no dynamic (bound-less or comma-only) array form at all: real VB6
  requires a `Static` array's bounds to be fixed at declaration time,
  with no `ReDim` ever possible for one, so `Static arr()` is rejected
  outright rather than treated as an unallocated dynamic array.

## Diagnostics

`WFC0149` reports a `Static arr()` with no bounds (real VB6 has no
dynamic `Static` array form), or a `Static` array element type other
than a fixed scalar (`Variant`/`Object` element arrays remain out of
scope — see below). `WFC0137` reports a `Set` source that does not
match a specific-class `Static`'s declared class, reusing the existing
diagnostic every other class-typed target already reports it with.

## Scope

This requirement adds fixed-size `Static` arrays and class-typed/generic
`Object` `Static` locals. It does not add:

- a `Variant`- or `Object`-*element* `Static` array (`Static arr(n) As
  Variant`/`As Object`, `REQ-0212`'s per-element retyping) — a `Static`
  array's element type must be one of the seven fixed scalar types,
  matching `Dim`'s own array element-type scope before `REQ-0212`
  extended it;
- a class-typed *array* `Static` (`Static arr(n) As SomeClass`) — still
  excluded the same way a class-typed array parameter is
  (`REQ-0214`/`REQ-0215`'s own Scope);
- `ReDim`/`ReDim Preserve` on a `Static` array — real VB6 has no such
  form; a `Static` array's bounds are fixed for the life of the
  procedure, exactly as declared.

## Verification

- `tests/evaluator_tests.cpp` covers a `Static` class-typed local
  persisting an instance across three calls, a class mismatch reporting
  `WFC0137`, a 1-D `Static` array persisting element values across
  calls, a multi-dimensional `Static` array, `Static arr()` reporting
  `WFC0149`, and a `Static` array with a `Variant` element type
  reporting `WFC0149`.
- `TC-MP0002-static-array-and-class-cli` verifies a `Static` class-typed
  local and a `Static` array together through `wfc --class ... --eval`.

## Reference

- [Microsoft VBA `Static` statement reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/static-statement)

## Traceability

This requirement closes the `Static` arrays/object-typed-locals
exclusion `REQ-0206`'s own Scope section listed, reusing `REQ-0201`'s/
`REQ-0210`'s array-bound grammar (factored into a new shared
`parse_fixed_array_bounds` helper, also now used by `Dim`'s own
fixed-size array form), `REQ-0203`'s/`REQ-0228`'s class-name resolver,
and `REQ-0230`'s object-lifetime fixes, all unchanged.
