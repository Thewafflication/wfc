# REQ-0233 — `Implements` interfaces

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0203, REQ-0206, REQ-0228

## Requirement

The evaluator shall support VB6's `Implements` interface feature as a
same-level dispatch contract between two ordinary `--class` classes (VB6
itself has no separate "interface" keyword — any class can serve as one):

- **`Implements InterfaceName`.** A class-body line, valid anywhere a field/
  method/property declaration is (checked ahead of `Dim`), naming another
  `--class` class. A class may `Implements` more than one interface. The
  named class is not required to be declared `Interface`-only in any way —
  real VB6 itself allows an ordinary class with actual method bodies to be
  used as an interface, and this evaluator does not verify that every member
  the interface declares is actually implemented (see Scope).
- **Naming convention.** A class that `Implements InterfaceName` provides its
  own version of each interface member under the name
  `InterfaceName_MemberName` (matching real VB6's own generated-name
  convention exactly), e.g. `Private Sub IShape_Draw()`. This evaluator does
  not itself require every interface member to have a matching
  `InterfaceName_MemberName` definition — an interface method the
  implementing class never provides simply fails with the pre-existing
  `WFC0135` (unknown member) the first time dispatch actually reaches it,
  the same way a genuinely missing member always has.
- **Type compatibility.** A variable, field, parameter, or `Property Set`
  parameter declared `As InterfaceName` accepts `Nothing` or an instance of
  any class that `Implements InterfaceName` (in addition to an instance of
  `InterfaceName` itself, since it is an ordinary class), generalizing the
  previously-exact-identity check `REQ-0203`'s `WFC0137` used. A new shared
  `class_satisfies(actual_class, declared_class)` helper implements this,
  called from both `Set`'s existing class-match check and `REQ-0228`'s
  parameter-binding class-match check.
- **Polymorphic dispatch.** A `.member`/`.member(args)` access through a
  reference statically declared `As InterfaceName` — a `Dim`/`Static`/field/
  parameter of that declared type — resolves to the *implementing* class's
  own `InterfaceName_MemberName`, not a literal member named `MemberName`,
  regardless of which implementing class the reference actually holds at
  that point. This is a static-declaration-driven rewrite: it is based on
  how the base expression was declared, not on the live instance's class.
  Dispatch is supported through: a `Dim`/`Static`-declared interface-typed
  variable (`Call s.Draw()`), and an interface-typed `Sub`/`Function`/
  `Property` parameter (`Sub Render(s As IShape) ... Call s.Draw() ...`).
- **`Private`-implementation-member access through the interface.** A
  `Private Sub InterfaceName_MemberName(...)` — the common real-world
  pattern, since an interface implementation is usually not meant to be
  called any other way — is reachable through a genuinely interface-typed
  dispatch even though `REQ-0206`'s own per-class `Private` visibility check
  would otherwise reject it (the calling code is not textually inside the
  implementing class). Direct access to the same member by its literal
  `InterfaceName_MemberName` name (bypassing the interface reference
  entirely) still correctly reports `WFC0142`, exactly as any other
  `Private` member does from outside its own class — only dispatch that is
  genuinely routed through the declared interface type is exempted.

## Diagnostics

`WFC0134` (existing) reports an unknown class name after `Implements`.
`WFC0137` (existing, widened per above) reports a `Set`/parameter-binding
source class that neither is nor implements the declared interface/class.
`WFC0142` (existing) reports direct, non-interface-routed access to a
`Private` interface-implementation member from outside its own class.
`WFC0135` (existing) reports dispatch reaching an interface member the
implementing class never actually defined (no completeness check — see
Scope), or any other unknown member. No new diagnostic codes were needed.

## Scope

This requirement adds `Implements` interface declarations, widened type
compatibility, and polymorphic method/property-read dispatch through a
`Dim`/`Static`-declared variable or a parameter. It does not add:

- **completeness verification** — nothing checks, at scan time, that a class
  declaring `Implements InterfaceName` actually defines an
  `InterfaceName_MemberName` for every member `InterfaceName` declares; a
  missing one is only ever discovered the first time dispatch reaches it
  (`WFC0135`), matching this evaluator's general preference for reporting
  errors where they are actually hit rather than doing separate up-front
  static verification passes;
- **property/field *writes* through an interface reference** — a
  `Property Let`/`Property Set`/plain-field write via a `.member =` on an
  interface-typed base is not rewritten to the implementing class's own
  `InterfaceName_MemberName` the way a method call and a `Property Get`
  read are; only the two read/call paths (`parse_member_access_after_dot`'s
  method-call and `Property Get` branches) were threaded with the
  interface-name rewrite, deliberately kept to the cases manually verified
  working;
- **`TypeOf ... Is InterfaceName`** — still excluded per `REQ-0203`'s own
  Scope, unchanged by this requirement;
- **interface inheritance** (`Implements` naming another interface, rather
  than a concrete class) — untested and not a design goal here;
- **dispatch through a non-bare-variable expression** — an interface-typed
  field read, a function's return value, or an array element used directly
  as the base of `.member` is not given the interface rewrite (only a
  `Dim`/`Static` local, and a `Sub`/`Function`/`Property` parameter, are);
  reassigning the expression's result to a local `Dim ... As InterfaceName`
  first works correctly;
- **dispatch through `Me`** — `Me` is always typed as its own concrete
  class, never as an interface it implements, so `Call Me.Draw()` inside an
  implementing class's own code still means the literal member `Draw`, not
  `InterfaceName_Draw`, matching real VB6;
- a pre-existing, unrelated gap this requirement's own manual testing
  confirmed still applies identically to interface and non-interface
  classes alike: a bare `obj.Method()` — parenthesized-call syntax, zero
  arguments, used as a top-level statement with no leading `Call` — reports
  `WFC0135` regardless of whether `Method` exists, because `REQ-0217`'s own
  no-`Call`/no-parens bare-statement support never covered the
  with-empty-parens shape; `Call obj.Method()` is unaffected and is the form
  every test/example here uses.

## Verification

- `tests/evaluator_tests.cpp` covers: two classes (`Circle`, `Square`) each
  `Implements IShape` with their own `Private Sub IShape_Draw()`, dispatched
  correctly in turn through one `Dim s As IShape` reference; the same
  dispatch through an interface-typed `Sub` parameter; direct access to the
  literal `IShape_Draw` name from outside the class still reporting
  `WFC0142`; `Set` rejecting an instance of a class that does not implement
  the declared interface (`WFC0137`); an unknown interface name in
  `Implements` reporting `WFC0134`; and one class implementing two different
  interfaces (`IShape`/`IArea`), with a `Property Get` dispatched correctly
  through the second interface's own reference.
- `TC-MP0002-interfaces-cli` verifies `Implements`, polymorphic method
  dispatch across two implementing classes, and `Property Get` dispatch
  through a second interface, together through `wfc --class ... --eval`.
- Manual testing while building this feature found and fixed a real bug
  (not merely an unverified edge case): dispatching to a `Private`
  interface-implementation method through a genuinely interface-typed
  reference incorrectly reported `WFC0142`, because the pre-existing
  per-class `Private` visibility check had no notion of a call being
  sanctioned by interface dispatch; fixed by threading a
  `bypass_for_interface_dispatch` flag from the exact point
  `parse_member_access_after_dot` determines the call is genuinely routed
  through the declared interface, through `member_accessible` and
  `call_class_method`. Verified the fix is correctly scoped: the same
  method is still rejected via its literal name from outside the class, and
  a non-interface reference's bare unprefixed method name still correctly
  reports `WFC0135` rather than silently succeeding.

## Reference

- [Microsoft VBA `Implements` statement reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/implements-statement)

## Traceability

This requirement closes the "interfaces (`Implements`)" item `REQ-0203`'s
own Scope section listed among this evaluator's class-model exclusions,
picked directly by the owner ("do interfaces") from that same backlog
cluster (alongside class inheritance, COM interop, lazy `As New`, and
`Class_Terminate` field-cascading) rather than through an `Explore`
subagent ranking, following `REQ-0230`/`REQ-0231`'s completion of the
`Static`-arrays half of that same cluster.
