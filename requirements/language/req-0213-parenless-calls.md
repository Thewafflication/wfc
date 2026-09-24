# REQ-0213 — Calling a niladic function without parentheses

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0194, REQ-0202, REQ-0203

## Requirement

The evaluator shall recognize a call to a zero-argument ("niladic")
intrinsic function, module-level `Function`, or unqualified sibling
class `Function`/method without a following `()`, wherever an
identifier that is not a declared variable, `Const`, or VBA constant
would otherwise be read as an expression — for example `Print Rnd`,
`x = NextId`, or `If IsReady Then`. `Call name` (no parentheses) is
likewise recognized as a statement, for a module-level or class-sibling
`Sub`/`Function` taking zero arguments.

- Resolution order for a bare identifier matches the existing lexical
  precedence: a local/module variable or `Const` (checked first,
  unchanged) always shadows a same-named procedure or intrinsic function.
  Failing that, a known module-level `Function` name is tried, then (from
  within a class member) a sibling class `Function`/method name, then a
  known intrinsic function name — in that order, matching the same
  fallback order the existing parenthesized/`.`-qualified forms already
  use for module-level vs. sibling-method resolution.
- A niladic call attempted this way is subject to the callee's own
  ordinary arity check: if the procedure or intrinsic actually requires
  one or more arguments, this reports the same `WFC0072` (wrong number of
  arguments) a parenthesized call with too few arguments would.
- A `Sub` referenced this way inside an expression (not via `Call`)
  reports `WFC0122`, the same as a parenthesized `Sub()` reference would.
- An identifier that matches none of the above (not a variable, not a
  known procedure, not a known intrinsic) still reports `WFC0015`
  ("undeclared variable"), not a confusing "unsupported function" message
  — the fallback chain is designed so a genuine typo gets the same
  diagnostic it always has.

## Diagnostics

`WFC0072` reports a parenthesis-free call to a procedure/intrinsic that
actually requires one or more arguments. `WFC0122` reports a `Sub`
referenced without `Call` in an expression context, parenthesized or not.
`WFC0015` reports a name that resolves to nothing at all (variable,
procedure, or intrinsic).

## Scope

This requirement adds only a *zero-argument* parenthesis-free call. It
does not add:

- a parenthesis-free call *with* arguments (`Foo 5, 6`, or `Print Format
  x, "0.00"`) — real VB6 syntax, deliberately excluded to avoid the
  classic ambiguity between that form and other statement/expression
  shapes (for example, distinguishing `Name (1)` — a call with one
  parenthesized argument — from `Name(1)` — an array index). Every
  argument-taking call still needs its parentheses;
- a bare `name` (zero arguments, no `Call` keyword) as a full *statement*
  was excluded here; `REQ-0217` later added it (`name arg1, arg2`, with
  arguments, remains excluded there too, for the same ambiguity reason);
- `obj.Method` (dotted access) without parentheses for a zero-argument
  method was excluded here; `REQ-0217` later added it, both as a
  statement (with or without `Call`) and in an expression.

## Verification

- `tests/evaluator_tests.cpp` covers a bare intrinsic (`Rnd`, matching its
  parenthesized rendering exactly), a bare module-level `Function` (with
  `Call name` also advancing its `Static` state and discarding the
  result), a bare `Sub` reference in an expression reporting `WFC0122`,
  an undeclared name still reporting `WFC0015`, a required-argument
  intrinsic called bare reporting `WFC0072` (both with and without a
  trailing argument that cannot be consumed), a bare call inside an `If`
  condition, and an unqualified bare sibling `Function` reference from
  within another class method.
- `TC-MP0002-parenless-call-cli` verifies a bare `Function` call, `Call
  name` (no parentheses), and a bare intrinsic call through `wfc --eval`.

## Traceability

This requirement narrows the parenthesis-free call gap `REQ-0202`'s Scope
originally flagged (`Name arg1, arg2`) to its zero-argument case, and
extends every existing niladic intrinsic function (`Rnd`, and any other
function whose arity permits zero arguments) and every module-level/
class-sibling `Function`/`Sub` to be callable that way.
