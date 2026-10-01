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

Addendum: when a program fails, the command line prints the text it had already
printed (`Evaluation::partial_output`) before the diagnostic; `Evaluation::output` stays
empty on failure. Test `TC-MP0002-partial-output-cli`.

Addendum: `Dim x As New Cls` (variables, not arrays/UDTs/class fields) creates the object on
first use, and again on first use after `Set x = Nothing`; an unused variable
never runs `Class_Initialize`. `x Is Nothing` counts as a use. Corpus `48-lazy-new`.

Addendum: the interpreter runs on a thread with a large reserved stack (512 MB on
64-bit, 160 MB on 32-bit targets), so procedure recursion is allowed to a depth of
5000 (1500 on 32-bit) before error 28 "Out of stack space"; a measured-stack
guard raises the same error earlier if native frames are unexpectedly large. The
earlier fixed limit of 64 nested calls is gone. Corpus `49-deep-recursion`.

Addendum: `Debug.Print` text is collected in `Evaluation::debug_output` (never in `output`)
and the command line writes it to standard error. Test `TC-MP0002-debug-print-cli`.

Addendum: comparison operators chain left to right (`1 < 2 = True`); `And`/`Or` evaluate
both operands (no short circuit). Corpus `50-operators`.
