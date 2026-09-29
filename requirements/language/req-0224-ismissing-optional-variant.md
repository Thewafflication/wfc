# REQ-0224 — IsMissing for an omitted Optional Variant argument

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0206

## Requirement

The evaluator shall make `IsMissing(paramName)` return `True` when
`paramName` names an `Optional Variant` parameter of the currently
executing procedure, that parameter has no explicit default value, and
the caller did not supply a corresponding argument. Every other case
continues to answer `False`, matching this evaluator's existing
constant-`False` stub (`REQ-0206`'s Scope) for the case real VB6 itself
also does not distinguish:

- a required parameter;
- a non-`Variant` `Optional` parameter (`Optional x As Long`);
- a `Variant` `Optional` parameter that *does* have an explicit default
  (`Optional x As Variant = 5`) — real VB6 treats the default as having
  been supplied, so `IsMissing` is `False` even when the caller omits
  the argument;
- an identifier that does not name a parameter of the current procedure
  at all — including any variable reference at module level, where
  there is no enclosing procedure to have parameters in the first place.

`IsMissing`'s argument is `paramName` itself, a bare parameter
reference, not an evaluated expression — the same distinction real VB6
makes: `IsMissing(x + 1)` or `IsMissing(42)` is not valid syntax, only a
direct parameter name is.

## Diagnostics

`WFC0072` reports a call with zero or more than one argument.
`WFC0011` reports an argument that is not a bare identifier (for
example a numeric literal or expression).

## Scope

This requirement makes `IsMissing` meaningful for exactly the one case
described above. It does not add:

- `IsMissing` reporting anything other than `False` for a required
  parameter, a non-`Variant` `Optional` parameter, or a defaulted
  `Variant` `Optional` parameter — this is real VB6's own behavior, not
  a simplification;
- a diagnostic for `IsMissing` called on an identifier that is not a
  parameter of the current procedure (a local `Dim`, a module-level
  variable, or an undeclared name that happens to parse as an
  identifier) — this evaluator answers `False` rather than rejecting the
  call, since real VB6's own compile-time behavior for this exact case
  was not independently verified;
- `CVErr`/error-value Variants, `IsError`, and late binding — unchanged,
  still excluded per `REQ-0206`'s own Scope.

## Verification

- `tests/evaluator_tests.cpp` covers `IsMissing` reporting `True` for an
  omitted `Optional Variant` argument and `False` once supplied; `False`
  for an omitted non-`Variant` `Optional` parameter; `False` for a
  required `Variant` parameter; `False` for a module-level `Variant`
  variable passed to `IsMissing` outside any procedure; `False` for a
  `Variant` `Optional` parameter that has an explicit default, whether
  omitted or supplied; and the `WFC0072`/`WFC0011` diagnostics for a
  wrong argument count and a non-identifier argument.
- `TC-MP0002-ismissing-cli` verifies the omitted/supplied `Optional
  Variant` case and the always-`False` non-`Variant` `Optional` case
  through `wfc --eval`.

## Reference

- [Microsoft VBA `IsMissing` function reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/ismissing-function)

## Traceability

This requirement makes good on `REQ-0206`'s own Scope note that
`IsMissing` "is not made meaningful by this requirement," implementing
exactly the one case that note said real VB6 itself distinguishes.
