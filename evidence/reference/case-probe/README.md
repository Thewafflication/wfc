# Case probe

**Content type:** Reference evidence

`UCase` and `LCase` of every character of the basic plane were measured under
Visual Basic 6: `vb6-reference.txt` has a line `<code> = <UCase> <LCase>`
(hexadecimal) for each of the 1,946 characters that change. The interpreter's
table of cases, `src/interpreter/vb_case_table.hpp`, is made from it by
`scripts/New-CaseTable.py` (Latin Extended, the IPA letters, Greek, Cyrillic,
Armenian, Georgian and the fullwidth letters are in it).

`scripts/New-CaseProbe.ps1` writes the project into this folder (the
generated files are not committed). To measure again, compile it with
`VB6.EXE /make` and run it as described in `../argument-probe`.

WFC reproduces every line, in the interpreter and in native code
(`TC-MP0002-case-probe-interpret` and `-native`).
