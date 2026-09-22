# REQ-0204 — The Me keyword and Class_Initialize/Class_Terminate

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0203

## Requirement

The evaluator shall support the `Me` keyword and the `Class_Initialize`/
`Class_Terminate` lifecycle hooks REQ-0203 deferred:

- `Me` is a reserved keyword usable anywhere an expression is expected,
  evaluating to the current class member's own instance. It is valid only
  while executing inside a class method or `Property` accessor's body (see
  REQ-0203's `instance_scopes_`); used anywhere else, it is an error.
  `Me.Field`, `Me.Method(args)`, `Set Me.Property = expr`, and
  `Call Me.Method(args)` all work exactly like the equivalent access through
  an ordinary object-reference variable, since `Me` produces the same kind
  of value (an `ObjectInstance`) any such variable holds. `Me` can also be
  passed as an ordinary value — assigned (`Set x = Me`), returned
  (`Set GetSelf = Me` from a `Function`/`Property Get` declared `As
  Variant`), or used as a call argument — sharing the current instance's own
  identity (`x Is Me` is then `True`).
- `Class_Initialize` is an optional, parameterless `Sub` a class may declare.
  If present, `New ClassName` invokes it automatically, immediately after
  every declared field has been initialized to its type's zero value and
  before the resulting instance is handed back to the expression that
  requested it (`Dim x As New ClassName`, `Set x = New ClassName`, or `New
  ClassName` used inline). `Me` and every field are fully usable inside it.
- `Class_Terminate` is an optional, parameterless `Sub` a class may declare.
  It is invoked automatically when the evaluator determines a variable holds
  the *last* remaining reference to an instance (VB6's own reference-counted
  object lifetime, approximated here — see Scope for exactly which points
  this covers and which it does not), immediately before that reference is
  actually dropped, so `Me`/fields are still fully valid and readable inside
  it. The three points covered are: (1) `Set x = expr` (including
  `Set x = Nothing`) overwriting a variable that held the last reference;
  (2) a `Sub`/`Function`/`Property` call's own local variables, checked one
  at a time as the call returns; and (3) every module-level variable, once,
  at the end of a successful program.
- A class member's own signature check runs at scan time, not at the first
  `New`/drop: `Class_Initialize`/`Class_Terminate` must each be declared
  with no parameters and as a `Sub` (never a `Function`), independent of
  whether the class is ever instantiated.

## Diagnostics

`WFC0138` reports `Me` used outside a class member's body. `WFC0139`
reports `Class_Initialize` or `Class_Terminate` declared with any
parameters, or as a `Function` instead of a `Sub`. Every other diagnostic a
`Class_Initialize`/`Class_Terminate` body or a `Me`-qualified access can
raise reuses the identical codes REQ-0202/REQ-0203 already define for an
ordinary method/property call or member access (for example, `WFC0135` for
an unknown member reached through `Me`).

## Scope

`Class_Terminate`'s automatic firing is a documented approximation of VB6's
reference-counted object lifetime, not a full implementation of it:

- it fires only at the three explicit points listed in the Requirement
  section above — a bare temporary instance never bound to any variable
  (for example, `Call (New Foo).Method()`, where the instance is used and
  discarded within a single expression) does not get `Class_Terminate`
  called at all;
- it does not cascade: when an instance A is terminated/dropped and A
  itself was the last reference to another instance B (for example, held in
  one of A's own `Variant` fields), B's `Class_Terminate` is not
  automatically invoked. B is still correctly freed (no leak) through
  ordinary C++ reference-counted destruction once A's own storage is
  destroyed — only the *notification* does not propagate;
- when two variables in the *same* call frame or module scope alias the
  same instance (for example, `Dim a As New Foo` then `Set b = a`),
  `Class_Terminate` still fires exactly once, at whichever alias happens to
  be checked last, rather than being (incorrectly) skipped entirely or
  fired twice.

This approximation is deliberate, not an oversight: `Class_Terminate` was
considered as a hook on `InstanceData`'s own C++ destructor (fired the
moment its `shared_ptr` refcount reaches zero, wherever that happens to
occur) and rejected, because that refcount can reach zero from *inside*
another container's own teardown — a `Scope`'s `variables` map destroying
its `Value`s as part of a call frame's `scopes_.pop_back()`, or the
`Interpreter`'s own member destruction at the very end of the program —
and reentrantly calling back into the evaluator's mutable state (pushing a
new call frame onto `scopes_` while that same deque is mid-`pop_back()`)
from inside that teardown is undefined behavior. Firing only from the three
explicit, well-defined points above means `Class_Terminate` always runs
while the interpreter is fully alive and not itself mid-teardown of
anything, at the cost of the three gaps listed.

Also out of scope, matching REQ-0203's own precedent:

- `Class_Terminate` running as part of an *unsuccessful* program's cleanup —
  it fires only on the module-scope drain at the end of a *successful*
  evaluation; every other return path in `evaluate()` is already reporting
  a fatal error, and running more class-member code during that unwind was
  judged more likely to compound the failure than clean up after it;
- an indexed/parameterized default constructor or destructor — both hooks
  are fixed to zero parameters (`WFC0139` rejects anything else);
- `Set Me = expr` or any other way to reassign which instance `Me` itself
  refers to — `Me` always denotes the instance the current call was invoked
  against, for that call's entire duration.

## Verification

- `tests/evaluator_tests.cpp` covers: `Me.Field` read from within a method;
  `Class_Initialize` running before the instantiating expression's own
  first use of the new instance, and seeing its own just-initialized
  fields; `Class_Terminate` firing on `Set x = Nothing`, at the end of a
  `Sub` call for that call's own local, and at the end of the program for a
  surviving module-level variable; `Set y = Me`/`Set GetSelf = Me`
  preserving identity and field access through the result; two same-frame
  aliases of one instance terminating it exactly once; `Me` used outside a
  class member (`WFC0138`); `Me` rejected as a variable name (`WFC0017`,
  reusing the existing reserved-keyword diagnostic); and
  `Class_Initialize`/`Class_Terminate` declared with a parameter or as a
  `Function` (`WFC0139`).
- `TC-MP0002-class-lifecycle-cli` verifies `Class_Initialize`, `Me`, and
  `Class_Terminate` (via `Set x = Nothing`) together through
  `wfc --class ... --eval`.

## Traceability

This requirement closes the two gaps `REQ-0203`'s own Scope section
explicitly deferred and named as future work: "a `Me` keyword" and
"`Class_Initialize`/`Class_Terminate` lifecycle events". Requested directly
by the owner ("do the me keyword class initailize and terminate") as a
single follow-on increment covering all three together, since they compose
naturally (a lifecycle hook's own body is exactly where `Me` is most useful).
