# Operator probe

**Content type:** Reference evidence

The comparison operators `=`, `<` and `>` were measured under Visual Basic 6 the
same way as `Select Case` (see `../select-case-probe`). The answers are in
`vb6-reference.txt` (4,695 lines); the rules that came out of them are
REQ-0314.

`scripts/New-OperatorProbe.ps1` writes the project into this folder (the
generated files are not committed). Each probe sets two variables, typed or
Variants that hold a value, and returns what `a op b` gives: `True`, `False`,
`Null` or `err<number>`. The operands are 21 typed kinds (Integer, Long, Single,
Double, Currency, Byte, Boolean, Date and Strings, with fractions and
overflows) and 17 Variants (Empty, Null, numbers, Strings, a Boolean and a
Date); every pair is tried with each operator, and the strings once more in a
module with `Option Compare Text`.

To measure again:

```powershell
pwsh -NoProfile -File scripts/New-OperatorProbe.ps1
python scripts/run_vb6_probe.py evidence/reference/operator-probe/OperatorProbe.vbp evidence/reference/operator-probe/operator-probe.txt
```

`scripts/run_vb6_probe.py` opens the project in Visual Basic 6, presses Start
With Full Compile, waits for the file and closes the IDE. The desktop must be
unlocked; it takes the foreground for about a minute.

WFC reproduces every line, in the interpreter and in native code
(`TC-MP0002-operator-probe-interpret` and `-native`).
