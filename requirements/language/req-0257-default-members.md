# REQ-0257 — Class default members

## Statement

A class member marked `Attribute Name.VB_UserMemId = 0` (as exported in
`.cls` files, inside the member's body) is the class's default member:
`obj(args)` on an object variable calls that method or indexed `Property
Get`. The `.vbp`/`.cls` loader keeps such lines (other `Attribute` lines are
blanked), `Attribute` lines inside bodies are no-ops, and the built-in
`Collection` uses this for `Item` (supersedes `REQ-0253`'s hard-coded case).

## Scope

Default members as assignment targets (`obj(1) = x`, default `Property
Let`), as a bare `obj` value (`x = obj` taking the default property), and
`obj!key` bang access are not implemented.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-default-member-cli`.
