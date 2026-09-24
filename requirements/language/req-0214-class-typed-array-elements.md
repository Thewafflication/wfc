# REQ-0214 — Class-typed array elements

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0203, REQ-0212

## Requirement

The evaluator shall recognize `Dim identifier(...) As SomeClass` — a
fixed-size, dynamic, or multi-dimensional array whose elements are typed
to a specific class, the array-element counterpart of a scalar `Dim x As
SomeClass` declaration (`REQ-0205`).

- Each element behaves exactly like `REQ-0212`'s generic `As Object`
  element (`Nothing`-initialized, `Set`-only assignment, `Is` for
  identity), with one addition: a `Set arr(i) = source` requires `source`
  to be `Nothing` or an instance of exactly the declared class, reporting
  `WFC0137` otherwise — the same class-match check a scalar `As
  SomeClass` variable's own `Set` already performs.
- `TypeName` renders the array as the class's own display name plus
  `"()"` (for example `"Counter()"`), matching how a live instance's own
  `TypeName` reports its class name. `VarType` returns `vbObject Or
  vbArray` (`8201`), the same as the generic `As Object` form — VarType
  never encodes which class, matching real VB6.

## Diagnostics

`WFC0137` reports a `Set` source that does not match the array's declared
class. `WFC0108` reports a plain `=` assignment to an element (`Set`
required), reusing `REQ-0212`'s existing diagnostic. `WFC0012` reports an
unrecognized `As`-clause type, now including a known class name in the
array form's accepted list.

## Scope

This requirement adds only a `Dim`-declared class-typed array. It does
not add:

- `As New ClassName` for an array element (eager per-element
  instantiation) — every element still initializes to `Nothing`, matching
  the existing generic-Object-array default;
- a class-typed array-typed `Sub`/`Function` parameter — `REQ-0211`'s
  array parameters still require a concrete scalar element type or (since
  `REQ-0215`) the generic `Variant`/`Object` kind, not a specific class;
- a class-typed array *return type* for a `Function` — `REQ-0216`'s array
  return types are fixed-scalar only.

## Verification

- `tests/evaluator_tests.cpp` covers `TypeName`/`VarType`/`Nothing`
  defaults, a matching-class `Set`/`Is` round trip, a mismatched-class
  `Set` (`WFC0137`), and a plain `=` rejection (`WFC0108`).
- `TC-MP0002-class-typed-array-cli` verifies declaration, `TypeName`, and
  a matching `Set`/`Is` round trip through `wfc --eval`.

## Traceability

This requirement extends `REQ-0212`'s generic `As Object`-element array
with the class-match checking `REQ-0205` already gives a scalar `As
SomeClass` variable, reusing `assign_object_reference` unchanged (only
the `declared_class_name` it receives differs, now read from the array's
own `element_class_name` instead of an empty string).
