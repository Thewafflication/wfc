# Format probe

**Content type:** Reference evidence

`Format`, `FormatNumber`, `FormatCurrency`, `FormatPercent` and
`FormatDateTime` were measured under Visual Basic 6 the same way as the
library functions (see `../library-probe`): what each call answers for a value
of each kind and a picture of each family. The answers are in
`vb6-reference.txt` (3,105 lines).

`scripts/New-FormatProbe.ps1` writes the project into this folder (the
generated files are not committed). Each probe sets a variable (typed, or a
Variant that holds a value) and returns what the call gives: the type name and
text of the result (`String:1,234.57`), `Null`, or `err<number>`. 27 kinds of
value (Integer, Long, Single, Double, Currency, Byte, Boolean, Date, Strings
and Variants of these, Empty and Null) meet the numeric pictures (digit
placeholders, thousands, percent, scientific, sections, literals), the named
formats, the text pictures (`@ & < > !`), the date and time pictures and the
extra arguments of the Format functions.

What Visual Basic does, as measured: a Null gives "" for every picture; a
Boolean is -1 or 0 for the numeric and date pictures and `True`/`False` for the
text ones; a Date is its serial for a numeric picture and its text for a text
picture; a String that is a number is that number (unless the picture is a text
one); a number outside the range of a Date with a date picture is error 6; a
value that rounds to zero in the places of its section takes the zero section;
`*x` shows nothing; `!` keeps the last characters of a text that is too long;
the negative amount of `Currency` is in parentheses; a style of `FormatDateTime`
outside 0 to 4 is error 5 whatever the value.

To measure again (the compile needs no elevation; the program is run with a
watchdog):

```powershell
pwsh -NoProfile -File scripts/New-FormatProbe.ps1
& 'C:\Program Files (x86)\Microsoft Visual Studio\VB98\VB6.EXE' /make evidence\reference\format-probe\FormatProbe.vbp /outdir $env:TEMP\fp /out $env:TEMP\fp\make.log
```

then run `FormatProbe.exe` from that folder and copy `format-probe.txt` to
`vb6-reference.txt`.

WFC reproduces every line, in the interpreter and in native code
(`TC-MP0002-format-probe-interpret` and `-native`).
