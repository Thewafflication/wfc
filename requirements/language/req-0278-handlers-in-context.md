# REQ-0278 — `On Error GoTo` handlers run in the failing statement's context

## Statement

- An `On Error GoTo label` handler executes in the context of the statement
  that failed. `Resume Next` and `Resume` therefore continue (or retry) *inside*
  any enclosing `For`/`Do`/`While`/`If`/`Select`/`With` body, as in VB6.
  `Resume label`, `GoTo` out of a handler, `Exit Sub|Function`, and falling off
  the end of the procedure leave the frame as before.
- A runtime error in the branch of a single-line `If` is handled by the
  frame's `On Error` mode.
- `Err.Raise Number, Source, Description` records `Err.Source`; `Err.HelpFile`
  (empty), `Err.HelpContext` and `Err.LastDllError` (0) are readable.
- Library argument/type errors map to VB error 13 (Type mismatch) and member
  access on a non-object to 424 (Object required), so `On Error` catches them.

## Verification

Corpus `36-error-handlers`.
