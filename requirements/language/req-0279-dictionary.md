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

Addendum: `Static` locals in class methods are per instance; `Cls.EnumMember`
resolves for Public class enums/constants. Corpus `51-class-statics`.

Addendum: a procedure called as a statement with a parenthesized list (`Inc (x)`,
`Inc(x)`, `Show(a, b)`) is accepted; one parenthesized argument is evaluated as an
expression and so passed by value. Corpus `52-byval-parentheses`.

Addendum (`Format`): a zero integer part shows no digit for `#` placeholders or
when the picture has no integer placeholder (`Format(0.5, ".00")` is `.50`,
`Format(0, "#")` is empty, `Format(0, "0;-0;Zero")` is `Zero`); `Format(Null, ...)` is Null;
`FormatNumber` family accepts omitted middle arguments and honours the
leading-digit flag. (Amends the unverified `".00"` interpretation noted in REQ-0218.)

Addendum: `Err.Description` / `Error$(n)` carry the full VB6 trappable-error text table
(3, 5-18, 20, 28, 35, 47-76, 91-94, 321-394, 422-463, 481-521, ...). Corpus `55-error-descriptions`.

Addendum: a failing evaluation reports `vb_error_number` / `vb_error_description`
when the failure is a VB run-time error, and the command line prints
`Run-time error 'N': text` after the diagnostic. Test `TC-MP0002-runtime-error-text-cli`.

Addendum: `ReDim a(n) As Type` (type agrees, or defines the array held by a Variant), several
arrays in one `ReDim`, a plain `Dim a()` array may change its dimension count on a
non-Preserve `ReDim` (`Dim a(,)` still fixes it), and `Dim a(n) As String * k`
pads/truncates each element. Corpus `56-redim-and-fixed-arrays`.

Addendum: `Integer` and `Byte` arguments are accepted by every library routine that
takes a numeric argument (`Mid$(s, i, i)` with `i As Integer`); they reach the routine as
`Long`, except for functions whose result depends on the subtype (`TypeName`,
`VarType`, `Hex`, `Oct`, `Abs`, `Sgn`, `Int`, `Fix`, conversions, `IIf`, ...).
Without `Option Explicit`, `ReDim` of an undeclared name declares it. Corpus
`57-word-frequency`, `58-integer-arguments`.

Addendum: `CreateObject("Scripting.FileSystemObject")` returns a built-in FileSystemObject
(VB source on the native file statements): `FileExists`, `FolderExists`, `CreateTextFile`,
`OpenTextFile` (read / write / append), `GetFile` (`Name`, `Path`, `Size`,
`DateLastModified`), `GetFileName`, `GetBaseName`, `GetExtensionName`,
`GetParentFolderName`, `BuildPath`, `GetAbsolutePathName`, `GetTempName`, `CopyFile`,
`MoveFile`, `DeleteFile`, `CreateFolder`, `DeleteFolder`; TextStream has `Write`,
`WriteLine`, `WriteBlankLines`, `ReadLine`, `ReadAll`, `Read`, `SkipLine`, `AtEndOfStream`,
`Line`, `Close`. Corpus `59-file-system-object`.

Addendum: `Collection.Add item, key, before, after` honours `Before` / `After` (index or key;
both given is error 5); omitted middle arguments work in parenthesis-free calls
(`c.Add "a", , 1`); `Is` in a not-taken branch never raises. Corpus
`60-collection-before-after`.
