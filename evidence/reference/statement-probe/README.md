# Statement probe

**Content type:** Reference evidence

Small programs were run under Visual Basic 6: each snippet of `snippets.txt`
is the body of a procedure that reports through `Say` and `SayV` (the type
and value of a Variant), and the probe records what it said, or the error that
stopped it (`|err<number>:<description>`; `|propagated...` when an error
leaves the procedure unhandled). The answers are in `vb6-reference.txt` (316
lines); the rules that came out of them are REQ-0320.

`scripts/New-StatementProbe.ps1` writes the project into this folder from
`snippets.txt` (the generated files are not committed). To add a snippet, add
a `@@ name` block to `snippets.txt`, generate, compile the project with
`VB6.EXE /make` (no elevation: see `../argument-probe`), run
`StatementProbe.exe` from its folder and copy `statement-probe.txt` to
`vb6-reference.txt`. VB6 refuses some programs at compile time (a label named
`Fix`, an Integer passed by reference to a Long), so those cannot be probed.

Not covered yet: `TypeName(Err)` (WFC has no `Err` as a value), `5 Mod 0` as a
constant expression (Visual Basic does not stop), the entries of `Option Base`
and `Option Compare` modules.

WFC reproduces every line, in the interpreter and in native code
(`TC-MP0002-statement-probe-interpret` and `-native`).
