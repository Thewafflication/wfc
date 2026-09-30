# REQ-0230 — Static Variant object-lifetime fixes

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0200, REQ-0206

## Requirement

The evaluator shall correctly run `Class_Terminate` for an object
instance reachable only through a `Static` Variant local, in both cases
this was previously missed:

- **Mid-program replacement.** When a call's own body replaces a
  `Static` Variant's object reference (`Set v = New Foo`), the *previous*
  instance — if that call's own persistent storage was its last
  reference — is now terminated at the point the call's final value is
  copied back into persistent storage, the same way an ordinary `Set`
  assignment already terminates whatever it overwrites (`REQ-0200`).
- **Program end.** An instance still reachable only through a `Static`
  Variant when the program finishes is now terminated as part of the
  same cleanup pass that already drains the module scope — extended to
  also drain every module-level procedure's own `statics`, and every
  class method's/property accessor's own `statics`.

Neither case required a new language feature: a `Static` Variant local
could already hold an object reference via `Set` (`REQ-0200`), so both
were live, silently-wrong bugs, not gaps behind an unimplemented form.

## Diagnostics

None specific to this requirement — `Class_Terminate` now runs where it
previously silently did not; no new rejection is introduced.

## Scope

This requirement fixes the two lifetime gaps above. It does not add:

- `Static` arrays or `Static` object-*typed* (as opposed to
  Variant-holding-an-object) locals — both remain excluded per
  `REQ-0206`'s own Scope, a separate, larger extension (parsing `Static`
  arr() As Type`/`Static o As SomeClass`) this requirement does not
  attempt;
- any change to how a `Static`'s value is loaded into a call's own frame
  at the start of a call, or to ordinary (non-`Static`) `Class_Terminate`
  timing — both are unchanged.

## Verification

- `tests/evaluator_tests.cpp` covers a `Static Variant` reassigned via
  `Set` across two separate calls (the first call's own instance
  terminates exactly once, at the point the second call's own copy-back
  replaces it) and an instance left in a `Static Variant` at program end
  (terminated once, after the program's own last statement, not during
  the call itself).
- `TC-MP0002-static-object-lifetime-cli` verifies the mid-program
  replacement and program-end cases together through `wfc --class ...
  --eval`.

## Traceability

This requirement fixes two bugs found while an `Explore` subagent was
scoping the `Static` arrays/object references item on the class-features
backlog (`REQ-0228`'s own work log entry): both are pre-existing,
independent of any array/typed-object `Static` extension, reachable
today through `Static Variant` + `Set` alone.
