# REQ-0209 — For Each over an array

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0201, REQ-0207

## Requirement

The evaluator shall recognize the case-insensitive `For Each identifier In
arrayExpression ... Next [identifier]` statement, iterating over an array's
elements in ascending index order. An array is the only iterable collection
this evaluator has; `arrayExpression` evaluating to anything else reports
`WFC0147`.

- `identifier` must already be a declared variable (`WFC0015` if not,
  matching the ordinary numeric `For`'s control-variable requirement). A
  `Variant`-declared control variable retypes to each element in turn, the
  same as any other `Variant` assignment; a fixed-type control variable
  requires the array's element type to match exactly, using the same
  widening/narrowing and `WFC0016` type-mismatch rule as an ordinary scalar
  assignment.
- The loop iterates a snapshot of the array taken once when the `For Each`
  statement is first reached; changes the array undergoes during the loop
  (for example a `ReDim` in the body) do not affect the ongoing iteration.
- An empty array (an unallocated dynamic array, or an array with zero
  elements) iterates zero times without error.
- `Exit For` (`REQ-0152`) terminates a `For Each` loop the same way it
  terminates a numeric `For` loop, sharing the same nesting-depth counter.
- `Next` optionally repeats the control variable's name, which must match
  the `For Each` variable, the same as the numeric `For`'s `Next` form.

## Diagnostics

`WFC0147` reports a missing `In` after the control variable, or a
non-array collection expression. `WFC0015` reports an undeclared control
variable. `WFC0016` reports a control-variable/element type mismatch.
`WFC0043`/`WFC0046`/`WFC0049` report a malformed control variable, a
missing `Next`, and a `Next` naming a different variable, reusing the
numeric `For`'s existing diagnostics (the two forms share one loop-body
parser).

## Scope

This requirement adds only `For Each` over an array. It does not add:

- iterating any other collection type (a `Collection` object, a `Dictionary`,
  or any other enumerable) — none exist in this evaluator yet;
- iterating a multi-dimensional array (tracked separately, not yet
  implemented) or an array nested inside a `Variant`;
- verified behavior for `For Each` over an unallocated dynamic array
  against the reference runtime specifically. Zero iterations was chosen by
  reasoned analogy with a zero-length `ParamArray` (REQ-0206), which does
  iterate zero times without error; this specific case (a `Dim identifier()`
  never `ReDim`'d) was not independently probed against local VB6, and is a
  disclosed, reasoned-but-unverified choice.

## Verification

- `tests/evaluator_tests.cpp` covers summing a `Long` array with a `Long`
  control variable, concatenating a `String` array with a `Variant`
  control variable, `Exit For` stopping mid-iteration, zero iterations over
  an unallocated dynamic array, a control-variable/element type mismatch
  (`WFC0016`), and a non-array collection expression (`WFC0147`).
- `TC-MP0002-for-each-cli` verifies iteration and accumulation through
  `wfc --eval`.

## Traceability

This requirement extends `REQ-0201`'s fixed-size array foundation and
`REQ-0207`'s dynamic array foundation with the `For Each` iteration form
both Scope sections listed as deferred, and reuses `REQ-0151`/`REQ-0152`'s
existing `For`/`Exit For` loop-body machinery.
