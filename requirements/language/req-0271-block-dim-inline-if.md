# REQ-0271 — Declarations inside blocks; colon-separated single-line `If`

## Statement

- `Dim`, `Static` and `Const` are legal inside `If`/`Select`/`For`/`Do`/
  `While`/`With` bodies, as in VB6 (a variable is procedure-scoped). A
  declaration executed again (a loop iteration) is a no-op and does **not**
  reset the variable; `Dim x As New C` is not re-instantiated. This lifts the
  former `WFC0027`/`WFC0034`/`WFC0039`/`WFC0050`/`WFC0063` restrictions.
- In a single-line `If`, every colon-separated statement after `Then` (and
  after `Else`) belongs to that branch:
  `If n = 0 Then F = "zero": Exit Function` exits only when `n = 0`.

## Scope

A block `Dim` that sits in a branch which never runs is still declared (the
declaration is hoisted by parsing); `Dim` of an already-declared name outside
a block is still `WFC0013`.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-block-dim-inline-if-cli`; corpus
`19-numbers`. Five older negative tests now expect success.
