# REQ-0194 — Rnd and Randomize

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0071, REQ-0140, REQ-0160, and REQ-0181

## Requirement

The MP-0002 evaluator shall recognize the case-insensitive `Rnd` function,
called as `Rnd()` or `Rnd(number)`, and the case-insensitive `Randomize`
statement, used as `Randomize` or `Randomize number`. `number` may be a
`Long`, `Double`, or `Boolean` (a `Boolean` widens to -1/0).

`Rnd()`, called with no argument or with a positive `number`, returns the
next value in a process-global sequence and advances the generator. The
sequence is produced by a 24-bit linear congruential generator:

```
state' = (state * 0x43FD43FD + 0xC39EC3) mod 2^24
result = state' / 2^24
```

starting from the default state `327680`. This formula and default state are
verified byte-for-byte against a local VB6 6.00.8176 / `MSVBVM60.DLL`
6.00.9848 reference probe: the first `Rnd()` call from the default state
returns `0.7055475`, matching the well-known VB6 fingerprint value.

`Rnd(0)` returns the most recently generated value without advancing the
generator. Before any value has been generated in the current program,
`Rnd(0)` returns `0`.

`Rnd(number)` with a negative `number` reseeds the generator from `number`
(see below) and returns the resulting value directly, without an additional
step. A later `Rnd()` call continues the sequence from that point.

`Randomize` with no argument reseeds the generator from a non-deterministic,
time-based entropy source, matching VB6's documented system-timer behavior.
`Randomize number` reseeds the generator from `number`.

Both `Randomize number` and `Rnd(negative)` reseed using the same WFC-owned
deterministic hash of the `Double`-widened seed value: its 64-bit IEEE-754
bit pattern, XOR-folded to 24 bits. The same seed always produces the same
subsequent sequence.

## Diagnostics

`WFC0072` reports wrong arity for `Rnd` (more than one argument) or a
`Randomize number` form with more than one expression. `WFC0073` reports a
non-numeric `Rnd` or `Randomize` argument.

## Scope

This increment does not implement:

- calling `Rnd` without parentheses (for example, bare `x = Rnd`). This
  matches the evaluator's existing parenthesized-call-only architecture,
  which every intrinsic function currently shares;
- reproducing the reference VB6 runtime's specific `Randomize(number)` or
  `Rnd(negative)` sequence for a given seed. A local probe of the reference
  runtime found that `Randomize number` does **not** reproduce the same
  subsequent sequence across repeated calls with the same explicit number,
  even though Microsoft's public documentation describes it as deterministic;
  no formula could truthfully claim to match VB6's specific per-seed output,
  so WFC defines its own deterministic seed hash instead. This is a
  documented, evidence-based variance, not a silent approximation;
- a distinct `Single` result type. `Rnd` returns a `Double` in the current
  value model, matching the established `CSng` precedent (`REQ-0182`).

## Verification

- `tests/evaluator_tests.cpp` covers the verified default sequence (the
  first three generated values), `Rnd(0)` repeating the last value (including
  before any value has been generated), `Rnd(positive)` advancing like
  `Rnd()`, `Rnd(negative)` reseeding deterministically (same argument twice
  gives the same result, and a later `Rnd()` continues the resulting
  sequence), `Randomize number` reseeding deterministically (repeating the
  seed repeats the sequence), argument-less `Randomize` keeping subsequent
  values in `[0, 1)`, `TypeName`, non-numeric arguments, and arity.
- `TC-MP0002-rnd-randomize-cli` verifies deterministic reseeding through
  `wfc --eval`.

## Reference

- [Microsoft VBA `Rnd` function reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/rnd-function)
- [Microsoft VBA `Randomize` statement reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/randomize-statement)
- Local reference probe: VB6 6.00.8176 (`VB6.EXE`) / `MSVBVM60.DLL` 6.00.9848,
  per `planning/reference-environment.md`. Probe source and captured output
  are recorded in `planning/work-log-mp-0002-strings.md` (increment #78).

## Traceability

This requirement partially implements `REQ-0077` (VBA `Math` functions).
