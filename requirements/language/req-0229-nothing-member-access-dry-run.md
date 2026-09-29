# REQ-0229 — Member access on Nothing inside a not-taken branch

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0200, REQ-0203

## Requirement

The evaluator shall not raise `WFC0106` ("Invalid use of Nothing") for
`.member` access on a `Nothing`-valued object reference when the
enclosing branch is not actually executing (a not-taken `If`/`Else`,
the untaken side of a `Select Case`, a dead loop body, ...) — matching
this evaluator's established convention that a value-dependent runtime
failure (arithmetic overflow, division by zero, and now this) is
suppressed during the dry-run parse of code that will not actually run,
while the source is still fully parsed and type-checked for shape.

This applies to every dotted-member form: a field/Property Get read
(`o.Member`), a Property Let/field write (`o.Member = expr`), a
`Property Set` write (`Set o.Member = expr`), and a method call with or
without arguments (`o.Method(args)`, `Call o.Method(args)`). In every
case, a `Nothing` base during a dry run returns or assigns a placeholder
(a `Long` `0` for a read; nothing, for a write) rather than resolving
the member for real — there is no class to resolve it against at all
when the base is genuinely `Nothing`, unlike a live instance (which
always has one, dry-run or not, which is why calls through a real
instance already worked correctly in a dead branch before this fix).
Any argument list still parses through normally, so its own shape is
validated the same as it would be for a live instance.

## Diagnostics

None new. `WFC0106` is unchanged for the case that matters — real
execution reaching `.member` access on `Nothing` still reports it.

## Scope

This requirement fixes the dry-run behavior of `.member` access on
`Nothing` specifically. It does not add:

- a dry-run placeholder that reflects the field/return's *real* declared
  type — since `Nothing` carries no class information at all, there is
  no way to know what type `.member` would have resolved to; the
  placeholder is always a generic `Long` `0`, which may occasionally be
  the wrong shape for some other expression built on top of it in a
  different dead branch (an accepted, disclosed limitation of dry-run
  parsing in general, not specific to this fix);
- `CStr(Nothing)` or `Nothing` concatenation's own `WFC0106` — those are
  a different, always-invalid-regardless-of-live-state type mismatch
  (`REQ-0200`), not a value-dependent runtime check, and continue to
  fire in a dead branch the same way other type mismatches already do.

## Verification

- `tests/evaluator_tests.cpp` covers a field read, a Property Let write,
  a Property Set write, and a method call with arguments, all through a
  `Dim o As Object`/`As SomeClass` whose `.member` access sits in the
  untaken `Else` branch of `If o Is Nothing Then ... Else ... End If`.
- `TC-MP0002-nothing-dead-branch-cli` verifies all three access shapes
  (read, Property Let write, method call) through `wfc --class ...
  --eval`.

## Traceability

This requirement fixes a bug found while implementing `REQ-0228`
(class-typed/`Object` parameters): manual testing hit this exact failure
using a plain `Dim o As Object` with no parameters involved at all,
confirming it was unrelated to that requirement and fixing it separately
here instead of widening that one's own diff.
