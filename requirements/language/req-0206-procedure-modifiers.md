# REQ-0206 — Optional/ParamArray/Static parameters and Public/Private visibility

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0202, REQ-0203, REQ-0205

## Requirement

The evaluator shall support four procedure-system refinements, applying to
both module-level procedures (`REQ-0202`) and class members (`REQ-0203`):

- **`Optional` parameters.** `Optional name [As Type] [= default]` may
  follow every required parameter; every parameter after the first
  `Optional` one must itself be `Optional` (or the trailing `ParamArray`
  below). Omitting a trailing `Optional` argument at the call site binds
  `default` (a constant expression, evaluated once at scan time) if
  written, or the type's own zero value otherwise (`Empty` for an implicit
  or explicit `Variant` parameter). `IsMissing` is not made meaningful by
  this requirement — see Scope.
- **`ParamArray` parameters.** `ParamArray name() As Type` — always the
  last parameter, always effectively `ByVal` — collects every remaining
  call argument, from its own position onward (including zero of them),
  into a fresh zero-based array bound to `name`, using the exact same
  fixed-size `ArrayValue` `REQ-0201` already defines (`LBound`/`UBound`/
  indexing all work on it unchanged; an empty call gives `LBound = 0`,
  `UBound = -1`, matching a genuinely empty VB6 array). `Type` must be one
  of the seven non-`Variant` scalar types `Dim`-declared arrays already
  accept (REQ-0201's own scope) — an explicit `As Type` is required.
- **`Static` local variables.** `Static name [As Type]`, declared directly
  inside a `Sub`/`Function`/`Property` body (not inside a nested
  `If`/`While`/`Do`/`For` block, matching the existing restriction on
  `Dim`), persists its value from one call to the next, rather than being
  reinitialized every call the way an ordinary local `Dim` is. Storage
  lives on the procedure's own declaration, found once during the module's
  initial scan and never re-scanned, so it survives across every call to
  that specific procedure for the lifetime of the program. Scoped to
  scalar/`Variant` types only — no `Static` arrays and no `Static` object
  references (see Scope).
- **`Public`/`Private` visibility.** A class member (a field, a `Sub`/
  `Function` method, or a `Property` accessor) may be declared `Public` or
  `Private`; a bare `Dim` field is implicitly `Private`, matching real
  VB6's own module-level default (this corrects `REQ-0203`'s original
  simplification, under which every field/method/property was reachable
  via `.` access regardless of the `Dim`/`Public` keyword written). A
  `Private` member is accessible from code executing inside *any* method of
  the *same class* (including, but not limited to, `Me` — VB6's visibility
  is per-class, not per-instance, so one instance's method can reach
  another instance of the same class's `Private` members), and from nowhere
  else: not from module-level code, and not from a different class's own
  method. `Public`/`Private` is also accepted before a module-level `Sub`/
  `Function` declaration (real-world VB6 source commonly writes one), but
  is not enforced there — this evaluator has only one standard module, so
  there is no second module for a `Private` module-level procedure to be
  hidden from.

## Diagnostics

`WFC0140` reports a required parameter following an `Optional` one.
`WFC0141` reports a malformed `ParamArray` declaration: missing the `()`
array marker, missing or `Variant` element type, or not being the last
parameter. `WFC0142` reports `.`/`Call`/`Set` access to a `Private` class
member from outside its declaring class. `WFC0144` reports `Static` used
outside a `Sub`/`Function`/`Property` body (a module-level or, by the
pre-existing `allow_declarations_` mechanism, a conditional-block `Static`
reports the existing `WFC0027` instead, exactly as a misplaced `Dim`
already does). `WFC0016` (the existing assignment/argument type-mismatch
diagnostic) additionally reports an `Optional` parameter's default value or
a `ParamArray` element failing to match its declared type. Every other
diagnostic reuses an existing code (`WFC0072` wrong argument count, now
covering a range rather than a single fixed number when `Optional`/
`ParamArray` are involved).

## Scope

This is a procedure-system increment, not a class-specific one, but it
does not extend either system beyond what real VB6 source most commonly
uses:

- `IsMissing` remains hardcoded `False` (`REQ-0176`'s existing scope
  boundary) — it is not made meaningful for an omitted `Optional Variant`
  parameter with no default. Real VB6's own `IsMissing` only ever
  distinguishes this one specific case (a non-`Variant` `Optional`
  parameter, or one with an explicit default, always reports `False` for
  `IsMissing` even when omitted); implementing it would need tracking a
  "this call omitted this specific argument" bit per parameter per call,
  entirely separate from the value the parameter was bound to, for a
  narrow, rarely-relied-on piece of introspection;
- `Static` arrays and `Static` object references — only a scalar/`Variant`
  `Static` local is supported, matching a fixed-type `Dim` local's own
  existing scope boundary one level further (arrays and object references
  would each need their own persistent-storage treatment beyond a plain
  `Scope` slot);
- a default value that is itself an expression referencing another
  parameter, a class field, or anything other than a literal/constant
  — `Optional`'s default is parsed the same way a `Const`'s initializer is
  (`constant_expression_ = true`), so it can only be a compile-time
  constant, matching real VB6's own requirement that a parameter default be
  a constant expression;
- `Public`/`Private` on a `Property` accessor is tracked independently per
  `Get`/`Let`/`Set` entry (matching how each is already stored
  independently for other purposes) rather than requiring all three
  accessors of one property name to share one visibility — a real but
  unusual VB6 capability (mismatched accessor visibility) that costs
  nothing extra to allow here, since nothing forces them to agree;
- a `Private` *class* declaration itself (hiding an entire class from
  other classes) — `--class` supplies every class as a flat, equally-
  visible list; there is no project-level module/class visibility to model
  beyond individual members.

## Verification

- `tests/evaluator_tests.cpp` covers: `Optional` with and without a
  default value; a wrong-count call both under and over the valid range
  (`WFC0072`); a required parameter after `Optional` (`WFC0140`); an
  `Optional` default whose type does not match its declared type
  (`WFC0016`); `ParamArray` alone, with a leading fixed parameter, and
  called with zero extra arguments (`LBound`/`UBound`/indexing all
  correctly reflecting an empty array); `ParamArray` not being the last
  parameter (`WFC0141`); `Static` persisting a counter across three
  separate calls; `Static` used outside any procedure (`WFC0144`); a bare
  `Dim` class field now rejecting external `.` access (`WFC0142`); explicit
  `Public`/`Private` on fields and methods, including a `Private` method
  called only from a `Public` sibling method (verifying internal access
  still works); and `Private`-is-per-class-not-per-instance, both the
  positive case (one `Foo` instance's method reading another `Foo`
  instance's `Private` field) and the negative case (a different class's
  method rejected from the same access, `WFC0142`).
- A live bug was found and fixed during this increment's own testing: an
  empty `ParamArray`'s zero-length `ArrayValue`, when indexed from inside a
  dry-run (`execute_ = false`) type-check pass over an unreached `For`-loop
  body, called `.front()` on an empty `std::vector` in
  `parse_array_index`'s existing `!execute_` short-circuit — undefined
  behavior, observed as a process crash. Fixed by returning a placeholder
  value instead of `.front()` when the array is empty in that dry-run-only
  path; see the work log for why the crash did not manifest until this
  increment (fixed-size `Dim`-declared arrays can never actually be empty).
- `TC-MP0002-procedure-modifiers-cli` verifies `Optional` with a default,
  `ParamArray`, `Static`, and a `Private` field/method pair together
  through `wfc --class ... --eval`.

## Traceability

This requirement implements the fourth item the owner selected (alongside
the three `REQ-0205` items) from `REQ-0203`'s own work-log "Remaining next
increments" list, via the same `AskUserQuestion` choice.
