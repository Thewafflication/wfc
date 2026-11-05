# Select Case probe

**Content type:** Reference evidence

WFC must make `Select Case` behave exactly as Visual Basic 6 does. The
documentation does not settle every case, for example how a Variant that holds a
number compares with a String constant, how `Null` and `Empty` compare, or when
the evaluation of a `Case` list stops. The probe measures it.

## What it is

`scripts/New-SelectCaseProbe.ps1` writes a Visual Basic 6 project into this
folder (the generated files are not committed):

- 20 selectors (every numeric type, Boolean, Date, String, and Variants that hold
  Empty, Null, numbers, strings, a Boolean and a Date);
- 20 `Case` values (the same kinds, literal and Variant);
- four clause forms (`Case v`, `Case Is < v`, `Case Is > v`, `Case v To v`);
- the same for strings under `Option Compare Text`; and
- the order of evaluation: the selector is evaluated once, a list stops at the
  first match, a later `Case` is not evaluated after a match, an error in a
  value that is reached, ranges written backwards, `Select Case True`.

Each probe is one `Select Case`; it records `match`, `nomatch` or `err<number>`.
`Main` writes one line per probe to `select-case-probe.txt` next to the project.

## How to use it

```powershell
pwsh -NoProfile -File scripts/New-SelectCaseProbe.ps1
```

1. Open `SelectCaseProbe.vbp` in Visual Basic 6 and run it (F5). If the compiler
   refuses a probe, delete that function and its `Rec` line and run again, and
   say which one it was: that is also an answer.
2. Keep the `select-case-probe.txt` that it writes.
3. Run the same project under WFC (`wfc --interpret SelectCaseProbe.vbp`, and
   `wfc SelectCaseProbe.vbp` for native code) and compare the files.

The Visual Basic 6 file is the reference. Every line where WFC differs is a
defect of WFC.

## Result

`vb6-reference.txt` is the answer of Visual Basic 6.00.8176 to the whole probe
(six thousand seven hundred and sixty-seven lines, no probe refused by the
compiler), run on 2026-11-04. It is the reference. The probe has five waves:
the basic kinds, other numbers and overflows, fractions, typed variables as the
Case values, and Variants that hold Currency and fractions. The rules that came
out of it are REQ-0313.

WFC reproduces the reference line for line, in the interpreter and in native
code. The tests `TC-MP0002-select-case-probe-interpret` and `...-native` run the
probe and compare it with this file.

To measure again, generate the project, open `SelectCaseProbe.vbp` in Visual
Basic 6, choose Run > Start With Full Compile, and keep the
`select-case-probe.txt` that it writes.
