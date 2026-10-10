# REQ-0284 — Implements interface completeness

**Content type:** Project requirement

**Status:** Accepted

## Statement

A class module that names another class module in an `Implements` statement
shall define, for every public member of that interface (each `Sub`,
`Function`, and `Property Get`/`Let`/`Set`), a member named
`Interface_Member`. If any is missing, loading the program fails with
`WFC0154` naming the missing member, as VB6 reports "Class must implement"
at compile time. Private members of the interface class and the
`Class_Initialize`/`Class_Terminate` handlers are not part of the interface.

## Verification

The evaluator unit test for `WFC0154` and the existing `Implements` corpus
cases.
