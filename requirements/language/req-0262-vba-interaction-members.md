# REQ-0262 — Remaining VBA members reachable without a host UI

## Statement

- **Interaction:** `MsgBox` (function returns `vbOK`; statement form accepted;
  no dialog is shown), `InputBox` (returns the default argument or `""`),
  `Beep`, `DoEvents`; `CreateObject`/`GetObject` raise error 429 (COM
  activation is MP-0003 scope); `SaveSetting`/`GetSetting`/`DeleteSetting`
  use an in-memory store for the life of the run.
- **Conversion/Financial:** `CVDate`, `Rate`, `MIRR`.
- **FileSystem:** `FileAttr`, `FileDateTime`, `GetAttr` (0 or 16),
  `SetAttr`/`ChDrive` (accepted, no effect), `Reset`.

## Scope

Not implemented: `Shell`, `SendKeys`, `AppActivate`,
`GetAllSettings`, `MacScript`, `VarPtr`/`StrPtr`/`ObjPtr`, `IMEStatus`,
the `Calendar` property, and `Width #`. `FileDateTime` is computed from the
file's age against the wall clock. Registry-backed settings persistence is
intentionally not provided.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-vba-members-cli`.

## Addendum (REQ-0263): `CallByName`

`CallByName(obj, name, callType, args...)` (function or statement) dispatches
`vbMethod` (1), `vbGet` (2), `vbLet` (4) and `vbSet` (8) against a class
instance's methods, properties and fields; an unknown member is error 438 and
`Nothing` is error 91. Dispatch is by the member's declared name, honouring no
`Private` restriction.
