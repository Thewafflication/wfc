# Argument probe

**Content type:** Reference evidence

The functions of the VBA library that take several arguments were measured
under Visual Basic 6 for edge values (see `../library-probe` for one argument
of every kind): what each call answers for zero, negative, fractional, Null,
empty and out-of-range arguments, for dates around the limits, for hexadecimal
and thousands text, and for `Like` patterns. The answers are in
`vb6-reference.txt` (3,511 lines); the rules that came out of them are
REQ-0319.

`scripts/New-ArgumentProbe.ps1` writes the project into this folder (the
generated files are not committed). Each probe is a function that returns what
a call gives: the type name and text of the result (`Long:5`), `Null`, or
`err<number>`. The arguments are literals, so each keeps its own type.

`known-differences.txt` lists the calls that WFC still answers differently
(`StrConv` with `vbFromUnicode` gives a Byte array here and a String there).

To measure again (the compile needs no elevation):

```powershell
pwsh -NoProfile -File scripts/New-ArgumentProbe.ps1
& 'C:\Program Files (x86)\Microsoft Visual Studio\VB98\VB6.EXE' /make evidence\reference\argument-probe\ArgumentProbe.vbp /outdir $env:TEMP\ap /out $env:TEMP\ap\make.log
```

then run `ArgumentProbe.exe` from that folder and copy `argument-probe.txt` to
`vb6-reference.txt`.
