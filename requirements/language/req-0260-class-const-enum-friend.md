# REQ-0260 — Class-level `Const`, `Enum`, and `Friend`

## Statement

A class module may declare `[Public|Private|Friend] Const NAME = expr [, ...]`
and `[Public|Private] Enum Name ... End Enum`. Constants and Enum members are
evaluated once at scan time and installed as read-only fields of every
instance (usable unqualified inside members and as `obj.NAME`). Enum names
declared in a class are valid `Long`-backed types throughout the program.
`Friend` members are accepted and treated as `Public`.

## Scope

Constant expressions may use literals and other constants only; a `Public
Enum` in a class is not exposed as a global name outside the class (use the
numeric value or `obj.Member`), and `Private` is not enforced for constants.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-class-const-enum-cli`.
