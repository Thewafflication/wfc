# REQ-0217 — Parenthesis-free statement and dotted calls

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0202, REQ-0203, REQ-0213

## Requirement

The evaluator shall recognize two further zero-argument, parenthesis-free
call forms `REQ-0213` left unsupported:

- A bare `name` full statement (no `Call` keyword at all) invokes a
  module-level or unqualified class-sibling `Sub`/`Function`, discarding
  any `Function` result — checked only when `name` is not a declared
  variable and nothing else follows it on the statement (no `=`, no `(`,
  no argument list), so it never competes with an assignment or an
  array-element write.
- `obj.Method` (dotted access, no parentheses) invokes a zero-argument
  method, both as a statement (with or without `Call`) and inside an
  expression — the same as `obj.Method()` — checked after `Property Get`
  (a class cannot declare both a method and a property under the same
  name) and before a field.

Both reuse `REQ-0213`'s own zero-argument call machinery
(`parse_call_argument_list` tolerating a missing `(`) and its arity
behavior: a procedure or method that actually requires one or more
arguments still reports `WFC0072`.

## Diagnostics

`WFC0072` reports a bare call to a procedure/method that requires one or
more arguments. `WFC0135` reports `obj.Method` naming neither a method, a
`Property Get`, nor a field. Every other diagnostic these forms can reach
(`WFC0122` for a `Sub` in an expression, `WFC0142` for an inaccessible
`Private` member, `WFC0106` for `Nothing`) is unchanged from the
parenthesized forms.

## Scope

This requirement adds only the two zero-argument forms above. It does
not add:

- a parenthesis-free call *with* arguments in either form (`name arg1,
  arg2`, or `obj.Method arg1, arg2`) — the same ambiguity `REQ-0213`
  already excludes;
- `Set obj.Prop = expr`'s or `obj.Prop = expr`'s own dispatch gaining a
  method-call fallback beyond the bare-statement case above — an indexed
  `Property Let`/`Set` (`obj.Prop(i) = expr`) is unaffected and still
  resolves exactly as `REQ-0205` defined it.

## Verification

- `tests/evaluator_tests.cpp` covers a bare module-level `Function`
  statement (`Static` state advancing, result discarded), a bare `Sub`
  statement, a bare unqualified-sibling-unaffected ordinary variable
  assignment (confirming the new bare-name check does not misfire for a
  real variable), a class method invoked bare (`obj.Method`) and via
  `Call obj.Method`, a field assignment and an indexed `Property Let`
  both still working unaffected, and a bare method call that actually
  requires an argument reporting `WFC0072`.
- `TC-MP0002-bare-call-cli` verifies a bare module-level `Function`
  statement and a bare class-method call through `wfc --eval`.

## Traceability

This requirement closes the remaining two items `REQ-0213`'s own Scope
section listed as still deferred, reusing its zero-argument call
machinery unchanged.
