# REQ-0276 — `DefType` statements, untyped Function results, type-suffixed Functions

## Statement

- `DefInt|DefLng|DefSng|DefDbl|DefCur|DefStr|DefBool|DefByte|DefDec|DefDate|DefVar|DefObj`
  `A-C, X` set the default type for untyped names starting with those letters:
  implicit declarations (no `Option Explicit`), `Dim x` with no `As`, and
  parameters with no `As`. `DefVar`/`DefObj` leave the default as Variant.
- A `Function` or `Property Get` with no `As` clause returns Variant (or the DefType
  of its name).
- `Function Name$(...)`, `Name%`, `Name&`, `Name#`, `Name!`, `Name@` declare the
  return type by suffix and can be called as `Name$(...)`.

## Scope

The DefType table is program-wide (all modules share one table), not
per-module. `Sub` names may not carry a suffix.

## Verification

Corpus `32-deftype`.
