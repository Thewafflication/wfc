# Library probe

**Content type:** Reference evidence

The functions of the VBA library were measured under Visual Basic 6 the same
way as the operators (see `../arithmetic-probe`): what each function answers
for an operand of each kind, and how the functions that compare Strings follow
`Option Compare` of the module that calls them. The answers are in
`vb6-reference.txt` (5,933 lines); the rules that came out of them are
REQ-0316.

`scripts/New-LibraryProbe.ps1` writes the project into this folder (the
generated files are not committed). Each probe sets a variable (typed, or a
Variant that holds a value) and returns what `f(a)` gives: the type name and
text of the result (`Integer:2`), `Null`, or `err<number>`. 87 functions meet
28 kinds of operand (Integer, Long, Single, Double, Currency, Byte, Boolean,
Date, Strings and Variants of these, Empty and Null). In the modules `LbBinary`
and `LbText` (the second has `Option Compare Text`) InStr, InStrRev, StrComp,
Replace, Split, Filter, `=`, `<` and `Like` compare the strings Alpha, alpha,
ALPHA, a, A, pha, PHA and the empty string, as Strings and as Variants.

`Sin` is not in the probe: Visual Basic 6 computes it with the x87 and the
last digit of a Double differs from the one of the C library.

To measure again:

```powershell
pwsh -NoProfile -File scripts/New-LibraryProbe.ps1
python scripts/run_vb6_probe.py evidence/reference/library-probe/LibraryProbe.vbp evidence/reference/library-probe/library-probe.txt --timeout 900
```

WFC reproduces every line, in the interpreter and in native code
(`TC-MP0002-library-probe-interpret` and `-native`).
