# Statement probe

**Content type:** Reference evidence

Small programs were run under Visual Basic 6: each snippet of `snippets.txt`
is the body of a procedure that reports through `Say` and `SayV` (the type
and value of a Variant), and the probe records what it said, or the error that
stopped it (`|err<number>:<description>`; `|propagated...` when an error
leaves the procedure unhandled). The answers are in `vb6-reference.txt` (1,074
lines); the rules that came out of them are REQ-0320.

`scripts/New-StatementProbe.ps1` writes the project into this folder from
`snippets.txt` and `classes.txt` (classes with properties, events and an
interface, an enum, user-defined types; the generated files are not committed). To add a snippet, add
a `@@ name` block to `snippets.txt`, generate, compile the project with
`VB6.EXE /make` (no elevation: see `../argument-probe`), run
`StatementProbe.exe` from its folder and copy `statement-probe.txt` to
`vb6-reference.txt`. VB6 refuses some programs at compile time (a label named
`Fix`, an Integer passed by reference to a Long), so those cannot be probed.

Not covered yet: `InStr` and `Like` by text with letters that stand for two (`InStr("straﬂe", "ss")` is 5 and `"ﬂ" Like "ss"` is True in Visual Basic), an Empty key of a Scripting.Dictionary (in Visual Basic it is the same key as `""` and as 0), `LSet` between two records of different types (Visual Basic copies the bytes; WFC copies only records of one type), the order in which named arguments are evaluated (Visual Basic evaluates them in the order of the parameters, WFC in the order they are written), three answers of the compiled program that come from its native-code compiler skipping an overflow check and not from the language (a Double too large for a Single is stored as infinity, `20 * 20` on two Bytes gives 144, and `-i` of the Integer -32768 stays -32768; WFC raises overflow for all three, as the maintainer decided on 2026-11-16), an array that is locked (`ReDim` or `Erase` inside `For Each` over it or inside `With` of one of its elements is error 10 in Visual Basic; WFC does not lock arrays), `5 Mod 0` as a
constant expression (Visual Basic does not stop), the entries of `Option Base`
and `Option Compare` modules.

WFC reproduces every line, in the interpreter and in native code
(`TC-MP0002-statement-probe-interpret` and `-native`).
