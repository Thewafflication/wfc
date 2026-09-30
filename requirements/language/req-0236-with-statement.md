# REQ-0236 — `With` statement

## Statement

`With expression` ... `End With` evaluates an object expression once; inside
the block a leading `.member` (read, write, `Property Let`/`Set`, method
call, or further chained access) refers to that object. Blocks nest; `.member`
binds to the innermost.

## Scope

- The expression must be an object reference or `Nothing` (`WFC0136`
  otherwise). Member access on `Nothing` raises `WFC0106` as usual.
- Not covered: `With` on a UDT, and `.member` inside a procedure called from
  the block (the callee does not see the caller's `With`).

## Verification

`tests/evaluator_tests.cpp` With case; `TC-MP0002-with-statement-cli`.
