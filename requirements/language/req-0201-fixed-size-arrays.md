# REQ-0201 — Fixed-size one-dimensional arrays

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0160, REQ-0176, REQ-0195, REQ-0196, and
REQ-0199

## Requirement

The evaluator shall represent a fixed-size, one-dimensional VB6/VBA array:
`Dim identifier(<bound>) As Type` or `Dim identifier(<lower> To <upper>) As
Type`, where `<bound>`/`<lower>`/`<upper>` are expressions evaluated at
declaration time and coerced to `Long` the same way `\`/`Mod`'s operands
are. `Dim identifier(<bound>)` declares indices `0` through `<bound>`
inclusive (VB6's default lower bound); the `To` form declares `<lower>`
through `<upper>` inclusive, and `<lower>` must not exceed `<upper>`. Every
element initializes to its type's default value (the same default a scalar
of that type would have).

- `Type` may be `Integer`, `Long`, `Double`, `Single`, `Currency`, `String`,
  or `Boolean` — the same fixed, non-retyping scalar types `Dim` already
  supports. `Variant` and `Object` array elements are outside this
  requirement's scope (see Scope).
- `identifier(<index>)` reads or writes one element: the index expression
  is coerced to `Long` the same way an array bound is, and an index outside
  the array's declared `[lower, upper]` range reports `WFC0111`. Writing
  requires the assigned value's type to match the array's element type,
  using the same widening/narrowing (`coerce_numeric_value`) and
  type-mismatch (`WFC0016`) rules as assigning to a fixed-type scalar
  variable.
- `LBound(<array>)` and `UBound(<array>)` return the array's declared lower
  and upper bound as a `Long`; a non-array argument reports `WFC0073`.
- `IsArray(<value>)` returns whether `<value>` currently holds an array.
- `TypeName` returns the element type's name followed by `()` (for example
  `"Long()"`); `VarType` returns the element's `VarType` code bitwise-ORed
  with the `vbArray` value `8192` (for example `8195` for a `Long` array,
  matching real VB6's `vbLong Or vbArray`).
- A bare array reference (`identifier` with no `(...)`) evaluates to the
  array's current `Value` as a whole — usable as an argument to `LBound`,
  `UBound`, `IsArray`, `TypeName`, and `VarType`, or in a whole-array
  assignment between two array variables of the same element type (see
  Scope for how this differs from real VB6).

## Diagnostics

`WFC0111` reports an out-of-range array index (read or write). `WFC0115`
reports a comma inside an index or declaration bound expression (a
multi-dimensional array, which is out of scope). `WFC0117` reports a
`Dim identifier(<lower> To <upper>)` whose lower bound exceeds its upper
bound. `WFC0073` reports `LBound`/`UBound` called on a non-array argument.
`WFC0016` reports an element-assignment type mismatch, reusing the existing
scalar-assignment diagnostic. `WFC0116` originally reported a
`Dim identifier()` with no bound; retired by `REQ-0207`, which gives that
form a real meaning (a dynamic array).

## Scope

This requirement implements only the agreed "fixed-size 1-D arrays only"
subset. It does not add:

- `ReDim`/`ReDim Preserve` for a `Dim identifier(<bound>)` fixed-size array
  — its size is fixed for the variable's lifetime, set at `Dim` time.
  `Dim identifier()` (no bound) instead declares a *dynamic* array, which
  `ReDim`/`ReDim Preserve` can resize; see `REQ-0207`;
- multiple dimensions (`arr(i, j)`) — a comma inside `(...)` reports
  `WFC0115`;
- `Erase` (see `REQ-0208`), `For Each` iteration over an array (see
  `REQ-0209`), or passing an array as a function argument to a
  user-defined procedure;
- `Option Base 1` (or any `Option Base` statement) — a bound-less `Dim
  identifier(n)` always means `0 To n`, matching VB6's un-declared default;
- `Variant`- or `Object`-element arrays — an array's per-element retyping
  would need the same tracking `Variant`'s scalar retyping uses, but at
  element granularity, which this increment does not add; `Dim arr() As
  Variant`/`As Object` reports `WFC0012`, the same "unrecognized As-clause
  type" diagnostic a genuinely unsupported type reports elsewhere;
- exact real-VB6 semantics for whole-array assignment (`arr1 = arr2`) and
  bare-array comparison. Real VB6 does not allow direct assignment or
  comparison between fixed-size array variables at all (a compile error);
  this evaluator instead permits `arr1 = arr2` between same-element-type
  arrays as an ordinary value-type copy (including of size/bounds), and
  array equality falls through to element-wise comparison. This is a
  disclosed simplification, not an attempt at exact fidelity, since neither
  operation is meaningful without `ReDim`/parameter passing to exploit it.

## Verification

- `tests/evaluator_tests.cpp` covers `Dim arr(n)`/`Dim arr(lo To hi)`
  (including a negative lower bound), indexed read/write, `LBound`/`UBound`,
  `IsArray`, `TypeName`/`VarType`, a `For`-loop-driven fill-and-read round
  trip, out-of-range read/write (`WFC0111`), an element-assignment type
  mismatch (`WFC0016`), a reversed bound (`WFC0117`), and rejected
  `Variant`/`Object` element types. `REQ-0207` covers the bound-less
  `Dim identifier()` dynamic-array declaration this requirement's original
  `WFC0116` rejected.
- `TC-MP0002-array-cli` verifies declaration, indexed read/write, `LBound`/
  `UBound`, and `IsArray` through `wfc --eval`.

## Traceability

This requirement implements the array value category deferred by
`REQ-0176`'s Scope section (`IsArray` previously stayed hardcoded `False`
because no array value existed) and by `REQ-0072`'s original Tailoring
list, and extends the `Dim`/assignment/`TypeName`/`VarType` model already
established by `REQ-0171`, `REQ-0195`, `REQ-0196`, and `REQ-0199`.
