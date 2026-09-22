# REQ-0203 — Class modules foundation

**Status:** Implemented (see `REQ-0204` for the `Me` keyword and the
`Class_Initialize`/`Class_Terminate` lifecycle hooks, deferred from this
requirement's original Scope and since implemented as a follow-on increment)
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0141, REQ-0176, REQ-0197, REQ-0200, REQ-0201, and REQ-0202

## Requirement

The evaluator shall support class modules supplied as separate source texts
alongside the standard module (`wfc::ClassModuleSource{name, source}`; CLI
`wfc [--class <Name> <class source>]... --eval <module source>`), mirroring a
real VB6 project's separate `.cls` files, since this evaluator otherwise
takes one flat, single-module source per run.

A class module's own source declares only these top-level items — no
executable statements, since nothing runs a class module's own body
directly:

```
[Dim|Public] name [As Type]

[Public] Sub Name([[ByVal|ByRef] parameter [As Type]] {, ...})
    statements
End Sub

[Public] Function Name([[ByVal|ByRef] parameter [As Type]] {, ...}) As Type
    statements
End Function

Property Get Name() As Type
    statements
End Property

Property Let Name(parameter [As Type])
    statements
End Property

Property Set Name(parameter As Object)
    statements
End Property
```

- A field declaration (`Dim` or `Public` — treated identically; see Scope)
  declares one instance field. `Type` is one of the same eight scalar types
  `Dim` already accepts at module level (`Integer`, `Long`, `Double`,
  `Single`, `Currency`, `String`, `Boolean`, `Variant`) minus `Object`; a
  bare declaration with no `As` clause is implicitly `Variant`, matching a
  bare module-level `Dim`. Every declared field is reachable via `.` access
  from outside the class and directly (unqualified) from within its own
  methods/properties — this evaluator does not model `Public`/`Private`
  visibility for class members, matching `REQ-0202`'s existing precedent for
  module-level procedures.
- `Sub`/`Function` methods have exactly the grammar and capabilities
  `REQ-0202` already gives a module-level procedure (`ByVal`/`ByRef`
  parameters, recursion, `Exit Sub`/`Exit Function`), except that a method's
  unqualified name/variable lookups see only its own parameters/locals and
  its *instance's* fields — never the standard module's variables, and
  never another instance's fields — matching VB6's own module-to-module
  isolation (a class module and a standard module are separate scopes).
  Calling a sibling method or `Property Get` of the same class unqualified
  (including a method calling itself, for recursion) is the implicit-`Me`
  equivalent of `Me.Method(...)`; `REQ-0204` later adds an explicit `Me`
  keyword covering every case this implicit form does not (an unqualified
  *write* to a sibling `Property Let`/`Set`, and passing the current
  instance itself to another call).
- `Property Get Name() As Type` returns a value via assignment to its own
  name, exactly like a `Function`; it accepts no parameters (an indexed
  property is deferred — see Scope). `Property Let Name(value [As Type])`
  and `Property Set Name(value As Object)` each accept exactly one
  parameter — the value being assigned — and return nothing. A class cannot
  declare a field and a `Property` accessor under the same name, so
  `obj.Member` unambiguously resolves to a method call (`(` follows), a
  `Property Get` (if the class declares one for that name), or a plain field
  read, in that order.
- `New ClassName` is an expression producing a new instance: every declared
  field is initialized to its type's zero value (matching a `Dim`-declared
  variable's own default). `Dim x As New ClassName` eagerly creates that
  instance at the `Dim` statement itself — a documented simplification of
  real VB6's lazy auto-instantiation, which only creates the instance on
  first use of `x` (see Scope). `Dim x As ClassName` (no `New`) declares a
  variable fixed to that specific class, initialized to `Nothing`, exactly
  like `Dim x As Object` (`REQ-0200`) except `Set`-checked against that one
  class rather than accepting any class's instance.
- `obj.Field`/`obj.Property` reads the field or invokes the `Property Get`;
  `obj.Field = expr` writes the field directly or invokes `Property Let` (a
  `Property Let` of the same name takes precedence over a same-named field —
  moot in practice, since a class cannot declare both); `Set obj.Property =
  expr` invokes `Property Set` (the only member-access `Set` target this
  evaluator supports — a class-typed field would need its own `Set` target,
  which is deferred; see Scope). `obj.Method(args)` is an expression
  (`Function` methods) or, via `Call obj.Method(args)`, a statement (`Sub`
  methods), mirroring `REQ-0202`'s existing module-level-procedure
  convention exactly.
- Two `ObjectInstance` values are the same object (`Is`) exactly when they
  were produced by the same `New` (reference identity, not field-by-field
  equality); assigning one to another variable via `Set` shares that same
  identity, so mutating a field through one reference is visible through the
  other. `TypeName` returns the class's own name (its as-supplied spelling);
  `VarType` returns `9` (`vbObject`), matching `Nothing`'s existing result —
  VarType never encodes *which* class, only that the value is an object
  reference. `IsObject` is `True` for a live instance, exactly as it already
  is for `Nothing`.

## Diagnostics

`WFC0126` reports a duplicate or reserved `--class` name. `WFC0127` reports
a class-body line that is not a field, `Sub`, `Function`, or `Property`
declaration. `WFC0128` reports a duplicate or reserved class member name
(a field/method colliding with any existing member, or a `Property`
accessor colliding with an existing field/method — Get/Let/Set of the
*same* property name are meant to coexist). `WFC0129` reports `Property`
not followed by `Get`, `Let`, or `Set`. `WFC0130` reports a `Property Get`
declared with parameters. `WFC0131` reports a `Property Let`/`Set` not
declared with exactly one parameter. `WFC0132` reports a `Property Set`
parameter not declared `As Object`. `WFC0133` reports a `Property` missing
its matching `End Property`. `WFC0134` reports `New`/`As` naming an unknown
class. `WFC0135` reports `.member` naming neither a method, a `Property`
accessor, nor a field. `WFC0136` reports `.member` used on a non-object
expression. `WFC0137` reports `Set` assigning an instance whose class does
not exactly match the target's `As ClassName` declaration (this evaluator
has no class hierarchy/interfaces, so "match" is always exact identity).
`WFC0106` (`REQ-0200`'s existing "Invalid use of Nothing") reports member
access on a `Nothing`-valued reference. Every other diagnostic a class
method/property body can raise reuses the identical module-level-procedure
codes `REQ-0202` already defines (`WFC0072` wrong arity, `WFC0122` a `Sub`
used in an expression, `WFC0123` call-nesting depth, `WFC0016` argument/
assignment type mismatch, `WFC0124`/`WFC0125` misplaced `Exit Sub`/
`Exit Function`).

## Scope

This is a foundation, not a complete VB6 object model. It does not add:

- multiple instances requiring garbage collection semantics beyond ordinary
  C++ `shared_ptr` reference counting — an instance is freed once every
  `Value` holding it is destroyed, with no explicit `Nothing`-assignment
  requirement, unlike VB6's own (also reference-counted) COM object model;
- `CreateObject`, `GetObject`, or any COM/host interop — this evaluator has
  no host to interop with;
- class-typed or `Object`-typed fields, and array-of-class/array-of-`Object`
  elements — a field's `Type` is the same eight-scalar-type list `Dim`
  accepts minus `Object` (matching `REQ-0202`'s existing parameter/return-
  type precedent); an object reference can still be *stored* in a `Variant`
  field (already possible since `REQ-0197`/`REQ-0200`), which is how this
  increment's own `Property Set` verification stores one internally;
- a dedicated `As ClassName` method/`Property Get` return type — a method
  can still effectively return an object by declaring `As Variant` and
  using `Set Name = New Other`/`Set Name = expr` inside its own body (the
  existing `Variant` return-type machinery from `REQ-0202` already supports
  this without any class-specific code);
- indexed `Property Get`/`Let`/`Set` (a parameter list beyond the single
  value parameter) — `Property Get` accepts zero parameters, `Property
  Let`/`Set` accept exactly one;
- `Optional` parameters, `ParamArray`, default parameter values, `Static`
  methods, and `Public`/`Private` visibility modifiers on any class member —
  the same exclusions `REQ-0202` already lists for module-level procedures,
  extended uniformly to class members;
- class inheritance, interfaces (`Implements`), `TypeOf ... Is ...`, late
  binding, or a default member (`obj` used bare implying a default
  property) — this evaluator's class model has no hierarchy at all, which
  is why `Set`'s class-match check (`WFC0137`) is always exact identity;
- lazy auto-instantiation for `Dim x As New ClassName` — this evaluator
  creates the instance eagerly, at the `Dim` statement itself, rather than
  deferring to `x`'s first actual use (a disclosed, deliberate
  simplification: VB6's lazy semantics would require tracking "declared
  `As New` but not yet instantiated" as a third state distinct from both
  `Nothing` and a live instance, for every read of `x`, not only after an
  explicit `Set x = Nothing`).

## Verification

- `tests/evaluator_tests.cpp` covers: a field and a method reading/writing
  it; `New`/`TypeName`/`VarType`/`IsObject` on a live instance; `Property
  Get`/`Let` round-tripping a computed value through a differently-named
  backing field; `Property Set`/`Get` storing and retrieving another class's
  instance through a `Variant` backing field, with `Is` confirming shared
  reference identity after `Set`; a method calling a sibling method of its
  own class unqualified, including self-recursion (`Factorial` calling
  itself from within `DoubleFactorial`, both unqualified); `Nothing`'s
  existing `Is`/class-typed-`Dim` behavior extended to a class-typed
  variable; and diagnostics for member access on `Nothing` (`WFC0106`), an
  unknown class (`WFC0134`), a `Set` class mismatch (`WFC0137`), wrong
  method arity (`WFC0072`), a duplicate class member name (`WFC0128`), a
  parameterized `Property Get` (`WFC0130`), and an unknown member
  (`WFC0135`).
- `TC-MP0002-class-cli` verifies a field, a method, `Property Get`/`Let`,
  `New`, `TypeName`/`VarType`/`IsObject`, and `Is` identity through
  `wfc --class ... --eval`.

## Traceability

This requirement implements the class-module layer `REQ-0202`'s own Scope
section named as "the prerequisite the owner's 'finish objects' request
surfaced as missing" and explicitly deferred to "a distinct, separately-
scoped future increment" — chosen next via `AskUserQuestion` from MP-0002's
work-log "Remaining next increments" list, then scoped further via three
more `AskUserQuestion` choices (separate-file classes over an inline
`Class...End Class` block; fields + methods + `Property` accessors over
fields/methods alone; `New`-only over also adding `Class_Initialize`/
`Class_Terminate`).
