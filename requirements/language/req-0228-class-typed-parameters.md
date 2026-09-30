# REQ-0228 — Class-typed and generic Object parameters

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0200, REQ-0203, REQ-0205

## Requirement

The evaluator shall recognize `As Object` or `As SomeClassName` for any
`Sub`/`Function`/`Property` parameter, not only `Property Set`'s own
single value parameter (the only place this was previously accepted).

- `As Object` accepts `Nothing` or an instance of any class. `As
  SomeClassName` additionally requires the argument to be exactly an
  instance of that class (or `Nothing`) — a mismatched class reports
  `WFC0137`, the same diagnostic a class-typed field or return type
  already reports for the identical case (`REQ-0203`/`REQ-0205`).
- A `Set param = ...` inside the callee's own body is class-checked
  against the parameter's declared class the same way, since the
  parameter's class name is threaded into the callee's own frame
  (`Scope::object_class_names`) exactly as a class-typed field's or
  return slot's already is.
- Every object/class-typed parameter is a reference type: it is always
  effectively passed the same way a scalar `ByRef` parameter is (the
  default, absent an explicit `ByVal`) — a `Set param = ...` inside the
  callee writes back to the caller's own variable when the call returns,
  exactly like a scalar `ByRef` parameter's own value does.
- `Property Set`'s own last-parameter requirement (`WFC0132`: it must be
  declared `As Object` or `As SomeClassName`) is unchanged, now simply
  reusing the same general parameter grammar instead of a Property-Set-
  specific one — a `Property Set` may now accept a specific class there
  too, not only the generic `As Object` form it was previously limited
  to.
- `Optional param As Object` (with no explicit default) binds `Nothing`
  when the caller omits the argument, the same zero value an ordinary
  `Dim`/field of the same type gets. `Optional param As Object = Nothing`
  behaves identically (the only value a *constant* expression can
  produce for an object reference).

Manual testing while implementing this requirement surfaced two real
bugs, both now fixed:

- **ByRef object write-back skipped `Class_Terminate`.** The existing
  ByRef write-back copied the callee's final parameter value over the
  caller's variable with a plain assignment, never checking whether the
  value the caller's variable *already held* was an `ObjectInstance`
  about to lose its last reference. Before this requirement, no
  parameter could ever hold an `ObjectInstance`, so this path was
  unreachable; enabling object-typed parameters made it reachable
  immediately. Fixed by calling the same `terminate_if_last_reference`
  helper `Set` already calls before overwriting a target, just before
  each ByRef write-back copy — a no-op for every non-object value, as
  everywhere else it is already called.
- **An omitted Optional object parameter bound `False`, not `Nothing`.**
  `zero_value_for_index` (used to synthesize a default for any omitted
  Optional argument with no explicit default) had no case for `Nothing`'s
  own `Value::index()`, falling through to a `Boolean` `False` — every
  other synthesized-default type already had its own case there. Fixed
  by adding one.

## Diagnostics

`WFC0106` reports a non-object argument for an object/class-typed
parameter (reusing the existing diagnostic, generalized from its
previous "Property Set requires..." wording). `WFC0137` reports an
argument that does not match a specific class-typed parameter's declared
class. `WFC0132` (`Property Set`'s own last-parameter requirement) is
unchanged.

## Scope

This requirement adds a class-typed/generic-Object *scalar* parameter
for any Sub/Function/Property. It does not add:

- a class-typed *array* parameter (`name() As SomeClass`) — still
  excluded per `REQ-0214`/`REQ-0215`'s own Scope, an entirely separate
  array-parameter mechanism this requirement does not touch;
- array-of-class/array-of-`Object` *elements* in general — unchanged
  from `REQ-0205`'s own Scope;
- lazy `As New` auto-instantiation, class inheritance, and
  `CreateObject`/COM interop — still deferred exactly as `REQ-0203`/
  `REQ-0204`/`REQ-0205` already recorded (`Class_Terminate` cascading
  through a terminated instance's own fields was closed by `REQ-0234`;
  interfaces were closed by `REQ-0233`);
- verifying real VB6's own exact behavior for `IsMissing` on an omitted
  object-typed `Optional` parameter — `REQ-0224` already scoped
  `IsMissing`'s real (non-`False`) answer to an `Optional Variant`
  parameter specifically; an `Optional Object` parameter's own
  `IsMissing` behavior was not separately investigated here and remains
  the pre-existing constant `False`.

## Verification

- `tests/evaluator_tests.cpp` covers a class-typed Sub parameter reading
  a field through it, a generic `As Object` parameter accepting two
  different classes, a class mismatch reporting `WFC0137`, a `Property
  Set` with a specific class-typed parameter, a ByRef object parameter's
  `Set`-reassignment writing back to the caller (with `Class_Terminate`
  firing for the instance it replaced), an unchanged ByRef object
  parameter *not* being prematurely terminated at write-back, and an
  `Optional Object` parameter (omitted and with an explicit `= Nothing`
  default) both binding `Nothing`.
- `TC-MP0002-class-typed-parameter-cli` verifies a class-typed parameter
  read and a ByRef `Set`-reassignment (with both `Class_Terminate` calls)
  through `wfc --class ... --eval`.

## Reference

- [Microsoft VBA Sub statement — Arglist](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/sub-statement)

## Traceability

This requirement closes the "class-typed parameter" exclusion
`REQ-0205`'s Scope section listed, generalizing the `As Object` grammar
`REQ-0203` originally restricted to `Property Set`'s own value parameter
to every Sub/Function/Property parameter, and reuses `REQ-0203`'s/
`REQ-0205`'s existing `object_class_names` class-match machinery
unchanged.
