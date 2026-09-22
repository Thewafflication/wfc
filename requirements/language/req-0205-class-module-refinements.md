# REQ-0205 — Class-module refinements

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0203, REQ-0204

## Requirement

The evaluator shall support three refinements to the class-module
foundation (`REQ-0203`) it originally deferred:

- **Unqualified sibling `Property Let`/`Set` writes.** Just as an
  unqualified call/read already reaches a sibling method or `Property Get`
  of the class currently executing (`REQ-0203`), a bare `Name = expr`/
  `Set Name = expr` statement inside a class member's body now checks for a
  sibling `Property Let`/`Set` of that name once no local variable,
  parameter, or field named `Name` exists, before falling back to
  "undeclared variable". A local variable/parameter always shadows a
  same-named property, matching ordinary lexical scoping — this matters in
  practice because a `Property Let`'s own value parameter is commonly given
  the same name as the property itself (`Property Let V(v As Long)`).
- **Class-typed and `As Object` fields, and `As ClassName`/`As Object`
  `Function`/`Property Get` return types.** A field, or a `Function`/
  `Property Get`, may now be declared `As Object` (accepting any class's
  instance, or `Nothing`) or `As SomeClass` (accepting only `Nothing` or an
  instance of that exact class — this evaluator has no class hierarchy, so
  "accepting" is always exact identity, matching `REQ-0203`'s existing
  `Set`-target class-match check). Such a field/return-value slot behaves
  exactly like an `Object`-typed variable already does: initialized to
  `Nothing`, assignable only via `Set` (`WFC0108` otherwise), with
  `Set obj.Field = expr` now a legal target when the class declares no
  `Property Set` for that name. Resolving `As SomeClass` against another
  class works regardless of `--class` argument order: `scan_classes` now
  registers every class's name in a first pass before any class's fields/
  methods/properties are actually scanned in a second pass, so a class can
  reference another declared after it on the command line.
- **Indexed `Property` accessors.** `Property Get Name(index [, ...]) As
  Type` may now declare one or more parameters (previously exactly zero);
  `Property Let`/`Set Name(index [, ...], value [As Type])` may now declare
  more than just the value parameter, with the value always last. Accessed
  as `obj.Name(args)` — indistinguishable, at the call site, from a method
  call (a class cannot declare both a method and a property under the same
  name, so the two never collide) — for both reads and
  `obj.Name(args) = expr`/`Set obj.Name(args) = expr` writes. The
  previously-zero-parameter forms (`obj.Name` with no parentheses at all)
  remain exactly as they were: a `Property Get`/`Let`/`Set` with zero index
  parameters is just the degenerate case of this same mechanism.

## Diagnostics

`WFC0131` (`REQ-0203`'s existing "Property Let/Set parameter count"
diagnostic) now reports zero parameters instead of any count other than
one, since an indexed property's own count is no longer fixed. `WFC0132`
now names the *last* parameter, not the only parameter, as the one that
must be declared `As Object` for a `Property Set`. `WFC0130` ("Property Get
does not accept parameters") is retired — indexed `Property Get` is now
supported, so this diagnostic can no longer occur. Every other diagnostic
an indexed property or a class-typed field/return value can raise reuses
the identical codes `REQ-0203`/`REQ-0200` already define (`WFC0072` wrong
arity for a mismatched index-argument count, `WFC0108` plain `=` on an
object-typed field/return slot, `WFC0137` a `Set` class mismatch,
`WFC0135` an unknown member).

## Scope

This closes three specific, previously-listed gaps in `REQ-0203`'s Scope
section; it does not add anything beyond them. In particular, still out of
scope:

- array-of-class/array-of-`Object` elements — only a scalar field or return
  type may be class-typed/`Object`-typed;
- a class-typed *parameter* (a method/property parameter still cannot be
  declared `As SomeClass`, only `As Object` for a `Property Set`'s own
  value parameter) — parameters were not part of this increment's scope;
- lazy `As New` auto-instantiation, `Class_Terminate` cascading through a
  terminated instance's own fields, class inheritance/interfaces, and
  `CreateObject`/COM interop — all still deferred exactly as `REQ-0203`/
  `REQ-0204` originally recorded;
- `Optional`/`ParamArray`/`Static`/visibility modifiers on a class member —
  a separate, larger procedure-system increment (see `REQ-0206`).

## Verification

- `tests/evaluator_tests.cpp` covers: an unqualified `Property Let` write
  whose value parameter shares the property's own name (verifying the
  local parameter correctly shadows it, after a bug in this exact scenario
  was found and fixed during this increment's own testing — see the work
  log); an unqualified `Property Set` write; a class-typed field assigned
  and read across two classes, with `Is` confirming shared identity; a
  `Set` class mismatch on a class-typed field (`WFC0137`); a generic
  `As Object` field accepting any class's instance; a forward reference
  (a class declared *before* the class it references on the `--class`
  command line) resolving correctly; an `As ClassName` `Function` return
  type, with plain `=` inside its own body rejected (`WFC0108`) and `Set`
  required; indexed `Property Get`/`Let` round-tripping two values through
  one instance; an indexed `Property Set` storing another class's instance;
  and a wrong-arity call to an indexed `Property Get` (`WFC0072`).
- `TC-MP0002-class-refinements-cli` verifies an indexed `Property Get`/
  `Let` and a class-typed field together through `wfc --class ... --eval`.

## Traceability

This requirement implements three items the owner selected (of four
offered via `AskUserQuestion`) from the "Remaining next increments" list
`REQ-0203`'s own work-log entry recorded; the fourth
(`Optional`/`ParamArray`/`Static`/visibility modifiers) is `REQ-0206`, a
separate procedure-system increment rather than a class-specific one.
