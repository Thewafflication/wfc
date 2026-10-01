# REQ-0279 — `Scripting.Dictionary` and default-member assignment

## Statement

- `CreateObject("Scripting.Dictionary")` returns a built-in Dictionary
  (provided as VB source, like `Collection`) with `Add`, `Item` (get / let /
  set, default member), `Exists`, `Remove`, `RemoveAll`, `Count`, `Keys`,
  `Items`, `Key`, `CompareMode`, and `For Each` over the keys. Keys compare
  case-sensitively unless `CompareMode = 1`; reading a missing key adds an
  `Empty` entry, as the real object does. `TypeName` is `"Dictionary"`.
- `obj(args) = value` and `Set obj(args) = ref` assign through the class's
  default member (Property Let / Set).
- Other ProgIDs still raise error 429.

## Verification

Corpus `37-dictionary`, `38-default-let`.

Addendum: a numeric (or numeric-string / "True"/"False") condition is accepted by
`If`, `ElseIf`, `While`, `Do While/Until` and `IIf` (non-zero is True); `Len(Empty)` is 0
and `Len(Null)` is Null; the global `App` object offers `Path`, `Title`, `EXEName`,
`PrevInstance`, `Major/Minor/Revision` and the version-info strings; `Declare`d
`GetTickCount`/`timeGetTime` and `Sleep` are emulated natively (other `Declare`d
routines still raise error 453). Corpus `39-conditions-app-declare`.

Addendum: `GetAllSettings` returns the saved key/value pairs as a 0-based
`n x 2` array (Empty when none); `AppActivate` and `SendKeys` are accepted as
no-ops; `Shell(command)` runs the command to completion through the host shell and returns
a task id (error 53 when the command fails). Corpus `40-settings-and-stubs`.

Addendum: `label: statement` on one line; `Width #n, w`, `Lock` and `Unlock` are accepted
as no-ops. Corpus `41-labels-and-file-stubs`.

Addendum: standard modules may declare `Property Get|Let|Set` procedures
(`Count = 5` calls the Property Let; reading `Count` calls the Get; indexed
accessors work); `Exit Property` is accepted. Corpus `42-module-properties`.

Addendum: an unsuffixed decimal integer literal beyond the `Long` range is a
`Double` literal (`Print 3000000000`, `TypeName(3000000000)` is "Double");
`&`-suffixed literals still raise `WFC0006`. (Supersedes the REQ-0140 subset limit.)

Addendum: `Dim b(2)` with no `As` is a Variant array; `EnumName.Member` resolves
(`Level.High`). Corpus `43-declarations`.

Addendum: `ReDim obj.field(...)` resizes a UDT/class array field; assigning a UDT
(or an array of UDTs, including one inside a UDT) copies every element
(value semantics). Corpus `46-nested-udt`.
