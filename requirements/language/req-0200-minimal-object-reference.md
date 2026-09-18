# REQ-0200 — Minimal object-reference stub

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0160, REQ-0176, and REQ-0197

## Requirement

The evaluator shall represent the unset state of a VB6/VBA object
reference, `Nothing`, as a new `Value` state distinct from every other type
(including `Null` and `Empty`). This evaluator has no class modules, `New`,
`CreateObject`, or any other mechanism that can produce a non-`Nothing`
object value, so `Nothing` is the only object-reference value this
requirement's model can ever hold; see Scope for what remains deferred.

- The `Nothing` keyword is a literal usable anywhere an expression is
  expected.
- `Dim x As Object` declares an `Object`-typed variable, initialized to
  `Nothing`. Unlike `Variant`, an `Object`-typed variable is a fixed type:
  it can only ever hold `Nothing` in this evaluator, and any attempt to
  assign a non-object value to it (through `Set`) reports `WFC0106`.
  `Const As Object` is not supported, matching real VB6 (object references
  cannot be constants).
- `Set identifier = expression` is the only legal way to assign an object
  reference, matching real VB6's requirement that `Set` (not plain `=`)
  assign object references. The target must be a `Dim x As Object` variable
  or a `Variant`-declared variable; assigning any other variable with `Set`
  reports `WFC0109`. The source expression must itself evaluate to
  `Nothing` (the only representable object value); anything else reports
  `WFC0106`. Plain `=` on an `Object`-typed target reports `WFC0108`,
  directing the caller to `Set` (a `Variant`-declared target still accepts
  plain `=` for `Nothing`, the same as any other value it can hold, since
  `Variant` assignment already retypes freely).
- `x Is y` compares two object references for identity. Since `Nothing` is
  the only representable object value, `Is` is `True` exactly when both
  operands are `Nothing`; either operand not being an object reference
  reports `WFC0107`. `x = y`/`x <> y`/`x < y`/etc. on an object reference
  report `WFC0107` directing the caller to `Is`, matching real VB6 (plain
  relational comparison of an object reference requires a default member,
  which nothing in this model has). Real VB6/VBA has no `IsNot` operator
  (that is VB.NET syntax); negation is `Not (x Is y)`, unchanged from the
  existing `Not` operator.
- `IsObject(<value>)` returns whether `<value>` currently holds `Nothing`.
  This is `True` for `Nothing` itself and for any `Object`-typed or
  `Variant`-holding-`Nothing` variable (a verified VB6 fact: `Nothing` is
  still object-subtyped, unlike `Null`/`Empty`).
- `TypeName` returns `"Nothing"` for a `Nothing` value; `VarType` returns
  the `vbObject` value `9`.
- `CStr(Nothing)` and concatenating (`&`) a `Nothing` operand both report
  `WFC0106`, matching real VB6's "Object variable or With block variable not
  set" error. `Print`/general rendering of `Nothing` is a safe, non-crashing
  empty-string fallback (the same convention already used for `Null`/
  `Empty`), since only the explicit conversion/concatenation paths need to
  raise an error.

## Diagnostics

`WFC0106` reports `CStr`, concatenation, or `Set`'s source expression
receiving something other than an object reference ("Invalid use of
Nothing" / "Set requires an object reference", depending on call site).
`WFC0107` reports `Is` used with a non-object operand, or a plain
relational operator (`=`, `<>`, `<`, `<=`, `>`, `>=`) used on an object
reference. `WFC0108` reports plain `=` assignment to an `Object`-typed
variable. `WFC0109` reports `Set` used on a variable that is neither
`Object`-typed nor `Variant`-declared.

## Scope

This is deliberately the minimal object-reference stub, not a general
object model. It does not add:

- class modules, user-defined types with methods/properties, `New`, or any
  other way to instantiate an object — there is currently nothing for an
  object variable to point at other than `Nothing`;
- `CreateObject`, `GetObject`, or any COM/host interop;
- late binding, method calls, or property access on an object reference
  (`x.Method`, `x.Property`) — meaningless without a class model;
- array-of-`Object` (`Dim arr(3) As Object`), which `REQ-0201`'s array
  scope explicitly excludes alongside `Variant`-element arrays;
- `TypeOf ... Is ...`, which needs a real class/interface model to be
  meaningful.

## Verification

- `tests/evaluator_tests.cpp` covers the `Nothing` literal, `TypeName`/
  `VarType`, `Dim x As Object` initializing to `Nothing`, `IsObject` for
  both an object variable and a non-object value, `Set`/`Is` on both an
  `Object`-typed and a `Variant`-declared variable, and every diagnostic
  (`WFC0106` from `Set`'s source, `CStr(Nothing)`, and `Nothing`
  concatenation; `WFC0107` from `Is` on a non-object operand and from plain
  `=` on `Nothing`; `WFC0108` from plain `=` assignment to an `Object`
  target; `WFC0109` from `Set` on a non-object, non-`Variant` target).
- `TC-MP0002-object-cli` verifies declaration, `IsObject`, `Set`, and `Is`
  through `wfc --eval`.

## Traceability

This requirement implements the object-reference state deferred by
`REQ-0197`'s Scope section (`IsObject` previously stayed hardcoded `False`
because no object-reference value existed) and by `REQ-0072`'s original
Tailoring list.
