# REQ-0215 — Variant- and Object-element array parameters

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0211, REQ-0212

## Requirement

The evaluator shall recognize `name() As Variant` and `name() As Object`
as array-typed `Sub`/`Function` parameters (`REQ-0211`), extending it to
`REQ-0212`'s two element kinds alongside the existing fixed-scalar
element types.

- A `Variant`-element array parameter binds only to a caller argument
  array whose own elements are `Variant` (`ArrayValue.is_variant_element`);
  an `Object`-element array parameter binds only to a caller argument
  array whose own elements are `Object` (`ArrayValue.is_object_element`).
  Neither matches a fixed-scalar-element array, nor the other kind, nor
  does a fixed-scalar-element parameter match a `Variant`/`Object`-element
  argument array — `WFC0016` reports every such mismatch, the same
  diagnostic a fixed-element-type mismatch already reports.
- Everything else about an array parameter (`REQ-0211`) is unchanged:
  always effectively `ByRef`, the argument must be a bare-identifier array
  variable, and every mutation the callee makes — including retyping a
  `Variant` element or `Set`-assigning an `Object` element — writes back
  to the caller's array when the call returns.
- A class-typed array parameter (`name() As SomeClass`) remains out of
  scope; only the generic `Object` kind is accepted for an array
  parameter.

## Diagnostics

`WFC0016` reports an element-kind mismatch between the argument array and
the parameter (fixed-scalar vs. `Variant` vs. `Object`, or two different
fixed-scalar types). `WFC0149` reports every other array-parameter
malformation `REQ-0211` already defines (missing element type, `ByVal`,
`Optional`, or a non-variable argument), now including a missing element
type after a `Variant`/`Object`-shaped attempt.

## Scope

This requirement adds only the parameter-binding extension. It does not
add a class-typed array parameter (see `REQ-0214`'s own Scope) or an
array-typed `ParamArray` element (a `ParamArray`'s own elements are
always scalar, unchanged from `REQ-0206`).

## Verification

- `tests/evaluator_tests.cpp` covers a `Variant`-element array parameter
  retyping two elements to different types with the mutation written back
  to the caller, an `Object`-element array parameter `Set`-assigning an
  instance with the mutation written back, and a fixed-scalar argument
  array rejected against a `Variant`-element parameter (`WFC0016`).

## Traceability

This requirement extends `REQ-0211`'s array-parameter foundation with
`REQ-0212`'s `Variant`/`Object` element kinds, reusing `REQ-0211`'s
existing `ByRef` write-back mechanism unchanged.
