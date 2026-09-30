# REQ-0265 — Implicit declaration without `Option Explicit`

## Statement

Unless `Option Explicit` appears in the main source or any class module, an
assignment to an undeclared name declares it (a `Variant`, or the type named
by its suffix: `s$`, `n%`, ...) in the current scope, and reading an
undeclared name yields `Empty`. With `Option Explicit` anywhere in the
program both remain `WFC0015`.

## Scope

Implicit arrays (`a(1) = 2`), implicit declaration through `For`/`Input #`
control variables, and per-module `Option Explicit` (the flag is
program-wide) are not handled. Three earlier negative tests now prefix
`Option Explicit`.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-implicit-declaration-cli`.
