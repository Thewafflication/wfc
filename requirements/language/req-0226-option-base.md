# REQ-0226 — Option Base

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0158, REQ-0159, REQ-0201, REQ-0210, REQ-0219

## Requirement

`wfc --eval` shall recognize the case-insensitive module directive
`Option Base 0` or `Option Base 1`, following the same placement rules
`Option Explicit`/`Option Compare` already establish (`REQ-0158`/
`REQ-0159`): it may follow blank lines, comments, and the other `Option`
directives (in either order), but must precede every declaration and
executable statement, and may appear at most once per module.

`Option Base 1` changes the lower bound a *bound-less* array dimension
gets — `Dim identifier(n)` (or one dimension of a multi-dimensional
`Dim identifier(n1, n2, ...)`), and the same shorthand in `ReDim` — from
`0` (VB6's undeclared default) to `1`. An explicit `<lower> To <upper>`
dimension is never affected: it always uses its own written `<lower>`
regardless of `Option Base`. A `ParamArray`'s own array is always
`0`-based no matter what `Option Base` says — a real, documented VB6
exception, not an oversight.

## Diagnostics

`WFC0152` reports more than one `Option Base` in a module. `WFC0153`
reports anything other than a literal `0` or `1` following `Option
Base`. `WFC0065`/`WFC0066`/`WFC0068` (module-level-only, must-precede-
declarations, no-directives-inside-a-block) are the same diagnostics
`REQ-0158`/`REQ-0159` already report for the other two `Option`
directives, now also covering `Option Base`.

## Scope

This requirement adds `Option Base 0`/`Option Base 1` for the bound-less
dimension case above. It does not add:

- any effect on an explicit `<lower> To <upper>` dimension, a
  `ParamArray`'s array, or a dynamic array's comma-only dimension-count
  pre-declaration (`Dim arr(,) As Type`, `REQ-0219`) — none of those has
  a bound-less lower bound for `Option Base` to change in the first
  place;
- per-module or per-class scoping — this evaluator has only one
  standard module plus a flat list of classes (`--class`); like `Option
  Explicit`/`Option Compare` before it, `Option Base` is a single,
  evaluator-wide setting, not scoped independently per class;
- any value other than `0` or `1` (real VB6 only ever accepts these
  two).

## Verification

- `tests/evaluator_tests.cpp` covers `Option Base 1` changing a 1-D
  bound-less `Dim`'s lower bound, a multi-dimensional `Dim`'s every
  dimension, `ReDim`'s own bound-less shorthand, an explicit `<lower>
  To <upper>` dimension staying unaffected, a `ParamArray` staying
  `0`-based, the default (`0`) behavior with no `Option Base` at all,
  and the `WFC0152`/`WFC0153` diagnostics.
- `TC-MP0002-option-base-cli` verifies `Option Base 1` for a 1-D array,
  a multi-dimensional array, and an explicit-bound dimension, through
  `wfc --eval`.

## Reference

- [Microsoft VBA `Option Base` statement reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/option-base-statement)

## Traceability

This requirement closes the `Option Base` exclusion `REQ-0201`,
`REQ-0210`, and `REQ-0219` each independently listed in their own Scope
sections as their array-declaration forms were added, and follows
`REQ-0158`/`REQ-0159`'s existing `Option` directive placement rules
unchanged.
