# REQ-0225 — Unconditional Do...Loop

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0148, REQ-0150

## Requirement

`wfc --eval` shall execute an unconditional `Do...Loop`:

```vb
Do
    <statements>
Loop
```

Neither the `Do` line nor the `Loop` line carries a `While`/`Until`
condition. The body repeats indefinitely, exactly the way `Do While
True` (or `Do Until False`) already lets a caller express, but without
writing a condition at all — ended only by `Exit Do` or an enclosing
`Exit For`, both already handled the same way every other `Do` loop
form handles them (`REQ-0150`). When an enclosing branch is not
selected, the body is parsed and type-checked once without output,
mutation, or arithmetic runtime failures, matching every other loop
form's own dry-run behavior.

An unconditional loop may nest with existing loops and conditionals the
same as any other `Do` form, including a `For`/`For Each` loop nested
inside it whose own `Exit Do` (reaching past the `For`'s own boundary)
ends the enclosing unconditional loop.

## Diagnostics

None specific to this requirement — an unconditional `Do...Loop` is
never rejected, only executed (or parsed once, in a dead branch).
`WFC0040` ("expected While or Until after Loop") continues to report a
genuinely malformed `Loop` line: one followed by neither `While`,
`Until`, nor a statement end (for example stray trailing text).

## Scope

This requirement adds the unconditional form only. It does not add:

- a maximum-iteration guard or any other protection against a body that
  never reaches `Exit Do`/`Exit For` — an unconditional loop with no
  exit hangs the process exactly the way `Do While True` with no `Exit
  Do` already could before this requirement; this is not a new risk,
  just a second way to write the same one;
- declarations inside the loop body — unchanged from `REQ-0147`'s
  existing `Do` body restriction, which applies to every `Do` form
  equally.

## Verification

- `tests/evaluator_tests.cpp` covers an unconditional loop ended by
  `Exit Do` from directly inside an `If` block, `Exit Do` reached from
  inside a nested `For` loop, and an unconditional loop inside a
  not-taken `If` branch parsing its body once without executing (no
  output, no hang). The pre-existing `WFC0040` test was updated from a
  bare `Do...Loop` with no condition anywhere (now valid syntax under
  this requirement, so no longer an error case) to a `Loop` followed by
  genuinely unexpected trailing text.
- `TC-MP0002-unconditional-do-loop-cli` verifies an unconditional loop
  ended by `Exit Do` through `wfc --eval`.

## Reference

- [Microsoft VBA `Do...Loop` statement reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/do-loop-statement)

## Traceability

This requirement extends `REQ-0148`'s post-test `Do` loop with the
unconditional form its own Scope section listed as deferred, reusing
`REQ-0150`'s existing `Exit Do`/`Exit For` handling unchanged.
