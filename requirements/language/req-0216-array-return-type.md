# REQ-0216 — Array-typed Function return

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0201, REQ-0202, REQ-0207

## Requirement

The evaluator shall recognize `Function name(...) As Type()` — a
`Function` (module-level or class method) that returns an array of
`Type`, where `Type` is one of the fixed scalar element types an ordinary
array already accepts (`REQ-0201`).

- The `Function`'s own return-value slot (its implicit local, bound to
  its own name) starts as an unallocated dynamic array of `Type`
  (`REQ-0207`), exactly as `Dim name() As Type` would. The body may
  either assign a whole array to its own name (`name = someArray`) or
  `ReDim`/`ReDim Preserve` it directly by that same name, both through
  the array machinery every other dynamic array already uses, unmodified.
- Calling the function evaluates to whatever `ArrayValue` its return slot
  holds when it returns (`Empty`/unallocated if the body never assigned
  or `ReDim`'d it), assignable to a `Dim`-declared array variable the same
  way any other array-valued expression is.

## Diagnostics

`WFC0150` reports an array return-type marker (`()` immediately after the
type name) applied to a `Variant`, `Object`, or class-typed return type
(an array return type must be a fixed scalar), or a malformed marker
(anything other than an empty `()`).

## Scope

This requirement adds only a fixed-scalar array return type for a
`Function`. It does not add:

- `Property Get` returning an array — only a `Function` does;
- a `Variant`- or `Object`-element array return type, or a class-typed
  one — `REQ-0212`'s/`REQ-0214`'s element kinds are not available here;
  an array return type is always a fixed scalar;
- a multi-dimensional array return type (`REQ-0210`) — the marker accepts
  only an empty `()`, one dimension implied;
- indexing a function's array result directly (`Foo()(i)`) — the caller
  must first assign the result to a `Dim`-declared array variable, then
  index that variable;
- validating that every value the body assigns to its own name is
  actually an array of the *declared* element type, rather than any
  array. This inherits `REQ-0201`'s existing, disclosed whole-array-
  assignment simplification (`arr1 = arr2` is a same-variant-alternative
  value copy with no per-element type check) rather than introducing a
  new one.

## Verification

- `tests/evaluator_tests.cpp` covers a `Function` that builds and returns
  a `Long` array via a local variable, a `Function` that `ReDim`s and
  fills its own return name directly, a class method returning an array,
  and the `WFC0150` diagnostic for a `Variant`/`Object` array return type
  and a malformed marker.
- `TC-MP0002-array-return-cli` verifies building and returning an array
  through `wfc --eval`.

## Traceability

This requirement extends `REQ-0202`'s `Function` return-value model and
`REQ-0207`'s dynamic-array foundation with an array-typed return, the
combination neither original requirement's Scope covered.
