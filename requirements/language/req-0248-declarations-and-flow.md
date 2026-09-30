# REQ-0248 — Multi-declarator statements, module `Public`/`Private`/`Global`, `End`, `GoSub`, fixed strings

## Statement

- `Dim a As Long, b As String`, `Static a, b`, `Const A = 1, B = 2`
  (a bare `Dim a, b As Long` makes `a` a Variant).
- Module-level `Public|Private|Global name As Type` and `Public Const`
  declare module variables/constants (visibility is not enforced; there is
  one standard module).
- `Next j, i` closing several nested `For`/`For Each` loops (closes the gap
  left by the reverted attempt: the inner loop leaves `, i` unconsumed and the
  enclosing loop's body recognizes it).
- `End` and `Stop` end the program normally (module-level variables are
  drained; no `Class_Terminate` on live instances, matching VB6).
- `GoSub label` / `Return`, `On n GoTo|GoSub l1, l2, ...` (frame-local,
  using the same jump machinery as `GoTo`); `Return` without `GoSub` is
  error 3.
- `Dim s As String * n`: assignment pads with spaces or truncates to `n`;
  the initial value is `n` spaces (VB6 uses NULs).
- `ParamArray name()` without `As`, or `As Variant`, is a Variant array.

## Scope

`Dim a(3), b` (untyped array declarators), fixed-length strings as arrays,
UDT members or class fields, and `Input #`/`Get` into fixed strings do not
apply the fixed length. `Public` in front of a declaration inside a
procedure body is not valid VB and is not specially handled.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-flow-and-declarations-cli`.
