# Arithmetic probe

**Content type:** Reference evidence

The arithmetic and logical operators `+ - * / \ Mod ^ & And Or Xor Eqv Imp` and
the unary `-` and `Not` were measured under Visual Basic 6 the same way as the
comparison operators (see `../operator-probe`). The answers are in
`vb6-reference.txt` (16,920 lines); the rules that came out of them are
REQ-0315.

`scripts/New-ArithmeticProbe.ps1` writes the project into this folder (the
generated files are not committed). Each probe sets two variables, typed or
Variants that hold a value, and returns what `a op b` gives: the type name and
text of the result (`Integer:10`), `Null`, or `err<number>`. The operands are
35 kinds (Integer, Long, Single, Double, Currency, Byte, Boolean, Date, Strings
and Variants of these, Empty and Null), with values chosen for the overflow,
rounding and zero cases.

To measure again:

```powershell
pwsh -NoProfile -File scripts/New-ArithmeticProbe.ps1
python scripts/run_vb6_probe.py evidence/reference/arithmetic-probe/ArithmeticProbe.vbp evidence/reference/arithmetic-probe/arithmetic-probe.txt --timeout 900
```

Compiling the 93 modules takes Visual Basic a few minutes; the desktop must be
unlocked while it runs.

WFC reproduces every line, in the interpreter and in native code
(`TC-MP0002-arithmetic-probe-interpret` and `-native`).

The same method showed how Visual Basic writes a number below 1: `CStr`, `Str$`,
`&` and `Print` agree, and a Double or Single is written in full while the zeros
after the point and the significant digits fit its 15 or 7 digits (`0.00001`,
`0.0000000001`, but `1.23456789012345E-02` and, for a Single, `1.234568E-02`).
