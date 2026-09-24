# REQ-0202 — User-defined Sub/Function procedures

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0141, REQ-0160, REQ-0171, REQ-0176, REQ-0195,
REQ-0196, REQ-0197, and REQ-0201

## Requirement

The evaluator shall support module-level, user-defined procedures:

```
Sub Name([[ByVal|ByRef] parameter [As Type]] {, ...})
    statements
End Sub

Function Name([[ByVal|ByRef] parameter [As Type]] {, ...}) As Type
    statements
End Function
```

- A module-level `Sub`/`Function` declaration is callable from anywhere in
  the module, including textually before its own declaration and
  (recursively) from within itself. A lightweight pre-scan pass, run once
  before the module's top-to-bottom execution begins, finds every
  declaration and records its signature and body range; the main execution
  pass skips over each declaration where it is written (a declaration is
  not itself a statement that runs in place) and only runs its body when
  called.
- `Type` (for a parameter or a `Function`'s return type) is one of
  `Integer`, `Long`, `Double`, `Single`, `Currency`, `String`, `Boolean`, or
  `Variant` — the same fixed scalar types `Dim` already supports minus
  `Object` and array types (see Scope). A parameter with no `As` clause and
  no type-declaration character is implicitly `Variant`, matching real
  VB6's bare-parameter default.
- A parameter is `ByRef` unless declared `ByVal`, matching VB6's default.
  For a `ByRef` parameter, if the corresponding call argument's source text
  is a single bare variable reference (nothing else in that argument
  position), the parameter's final value is copied back into that variable
  after the call returns. Any other argument form (a literal, an operator
  expression, `arr(i)`, another call, ...) behaves as `ByVal` regardless of
  the parameter's own declaration, since there is no caller-visible
  variable for a mutation to reach — the same outcome a real VB6 compiler
  produces via a discarded temporary.
- Calling a `Function` is an expression: `Name(args)`, usable anywhere an
  expression is (assignment, `Print`, nested in another call, ...).
  `Call Name(args)` (since `REQ-0213`, `Call Name` with zero arguments;
  since `REQ-0217`, a bare `Name` alone with zero arguments and no `Call`
  keyword at all) is the only supported way to invoke a `Sub` as a
  statement, or to invoke a `Function` while discarding its result,
  matching real VB6's `Call` statement; this evaluator still does not
  support VB6's other, parenthesis-free `Name arg1, arg2` statement-call
  form with one or more arguments (see `REQ-0213`'s Scope).
- Each call has its own local scope: its parameters and any variables it
  `Dim`s are visible only within that call, and are not visible to (or
  visible from) any other call, including a recursive call to the same
  procedure or the procedure that called it. A local variable may shadow a
  module-level variable of the same name. Variable lookups inside a call
  see only that call's own local scope and the module scope — never an
  enclosing caller's locals — matching VB6's own module/procedure two-level
  scoping (VB6 has no nested block scope).
- A `Function` returns its result via assignment to its own name, exactly
  like a local variable of the declared return type (initialized to that
  type's zero value); if never assigned, the call returns that zero value.
- `Exit Sub` and `Exit Function` immediately end the current call (skipping
  the rest of its body, including unwinding any `Do`/`For`/`Select` blocks
  still active within it), the same way `Exit Do`/`Exit For` already end
  their own loop.
- Recursion is supported, bounded by a fixed nesting-depth guard (`WFC0123`
  beyond 64 levels) to fail cleanly instead of overflowing the native call
  stack, since each nested VB6 call recurses through this evaluator's own
  C++ call chain.

## Diagnostics

`WFC0118` reports a `Sub`/`Function` declaration missing its name.
`WFC0119` reports a declaration whose name is already used by another
procedure, or is a reserved keyword. `WFC0120`/`WFC0121` report a
`Function`/`Sub` declaration missing its matching `End Function`/`End Sub`.
`WFC0122` reports a `Sub` name used where an expression is expected (only a
`Function` may be called in expression position). `WFC0123` reports call
nesting beyond the depth guard. `WFC0072` reports a call with the wrong
number of arguments, reusing the existing "wrong arity" diagnostic every
intrinsic function shares. `WFC0016` reports an argument whose value does
not fit its parameter's declared type, reusing the existing assignment
type-mismatch diagnostic. `WFC0124`/`WFC0125` report `Exit Sub`/
`Exit Function` used outside, respectively, an active `Sub`/`Function` call
(including `Exit Sub` inside a `Function` or `Exit Function` inside a
`Sub`).

## Scope

This requirement implements only the subset scoped after the owner's
initial request ("implement arrays and object references next" /
"finish objects") surfaced that a full object model depends on user-defined
procedures, which did not exist at all; the owner then chose, via
`AskUserQuestion`, to build procedures first as their own increment, with
no class-module layer yet. Accordingly, this requirement does not add:

- class modules, `Property Let`/`Get`/`Set` members, or calling a method on
  an object reference — the prerequisite this increment was scoped to
  deliver, not the class-module layer itself, which remains a distinct,
  separately-scoped future increment;
- `Object` parameters or an `Object` return type — a return type is one
  of the eight scalar types `parse_type_keyword` recognizes (an array
  *parameter* was added later, see `REQ-0211`, and a fixed-scalar array
  *return* type, see `REQ-0216`);
- `Optional` parameters, `ParamArray`, or default parameter values;
- `Static` procedures (persisting local variables across calls);
- `Public`/`Private` visibility modifiers on a `Sub`/`Function` declaration
  — every module-level procedure is callable from anywhere in the module,
  and no modifier keyword is recognized before `Sub`/`Function` at all (a
  program that writes one gets a parse error at that point, rather than the
  modifier being silently accepted or silently ignored);
- eagerly validating an uncalled procedure's body. The pre-scan pass finds
  and validates every declaration's header (name, parameters, return type)
  before execution begins, but does not parse a procedure's body at all
  (only searches for its terminating `End Sub`/`End Function` line) — a
  syntax or semantic error inside a body is only discovered if and when
  that procedure is actually called. This is a disclosed simplification
  against this evaluator's usual "parse everything, execute conditionally"
  philosophy (see, for example, how an unreached `If False Then` branch is
  still parsed and type-checked with `execute_ = false`), made because
  eagerly validating a body would require binding placeholder argument
  values for a dry run with no actual call present, a meaningfully larger
  change than this increment's scope;
- the parenthesis-free `Name arg1, arg2` statement-call form for a
  user-defined `Sub` with one or more arguments (`REQ-0213` added `Call
  Name` with zero arguments; `REQ-0217` added a bare `Name` alone with
  zero arguments and no `Call` keyword).

## Verification

- `tests/evaluator_tests.cpp` covers a `Function` and a `Sub` declaration
  and call, forward reference (calling a procedure declared later in the
  source), direct and mutual recursion, `ByRef` write-back, `ByVal`
  isolating the caller, `ByRef` as the unwritten default, local-variable
  shadowing of a module-level variable of the same name, a `Variant`
  parameter accepting different argument types across calls, a `Function`
  that never assigns its own name returning its return type's zero value,
  `Exit Sub`/`Exit Function`, calling the same `Function` twice within one
  expression, an unreached call inside an `If False Then` branch not
  executing, a local array declared inside a `Sub`, calling a `Sub` in
  expression position (`WFC0122`), wrong arity (`WFC0072`), a duplicate
  procedure name (`WFC0119`), calling an undeclared procedure (`WFC0015`),
  and `Exit Sub`/`Exit Function` used outside their matching procedure kind
  (`WFC0124`/`WFC0125`).
- `TC-MP0002-procedure-cli` verifies recursion and `ByRef` through
  `wfc --eval`.

## Traceability

This requirement implements the procedure-scope gap `REQ-0141` explicitly
deferred to "later requirements" from the very start of MP-0002, and is the
prerequisite the owner's "finish objects" request surfaced as missing
before any class-module work can begin.
