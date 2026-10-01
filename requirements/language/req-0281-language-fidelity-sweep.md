# REQ-0281 — Language fidelity sweep (single-line procedures through Variant arithmetic)

## Statement

Behaviors added by corpus-driven hardening of MP-0002, each with a corpus program
(`tests/corpus/66`–`82`) or evaluator test:

- **Single-line procedures.** `Sub F(): body: End Sub`, `Function`, and `Property`
  forms (modules and classes) are accepted. A line-start `Name:` is a label, as in VB6.
- **Numbered lines.** A leading line number is a label; `GoTo`/`GoSub`/`On ... GoTo`/`Resume`
  accept line numbers; `Erl` returns the last numbered line executed.
- **Class and bracket syntax.** `Private a As Long, b As String` declares several fields;
  `[bracketed names]` are identifiers (`[_NewEnum]` maps to `NewEnum`); `As IUnknown` and
  `As IDispatch` are plain object references; a class exposing `NewEnum` is enumerable
  with `For Each`.
- **Statements.** `Rnd -1` and `Shell cmd` work as statements; `Mid$(...) = x` works after
  `Then`; `Debug.Assert` is accepted and ignored (the condition is not evaluated).
- **Library qualifiers.** `VBA.Left$(...)`, `Strings.Len`, `Math.Sqr`, `Conversion.Int`,
  `Interaction.IIf` (and similar) resolve to the unqualified function; `ChrW$` is accepted.
- **Numeric literals.** `D` may be the exponent marker (`1.5D-1`).
- **Arrays.** `a(i)(j)`, `Array(...)(i)` and `Split(...)(i)` index the array an element or
  call produced; `v(i)(j) = x` assigns into a Variant element's array. `Byte()` and `String`
  convert on assignment (UCS-2 bytes) and `StrConv` supports `vbFromUnicode`/`vbUnicode`.
- **ByRef through fields.** `Sub(rec.Field)`, `Sub(objs(i).Field)` and `Sub(obj.Field)` pass
  the field by reference.
- **File I/O.** `Input #` into Variants yields Boolean/Date/Null for `#TRUE#`/`#date#`/`#NULL#`
  and `Integer` for small whole numbers; `Write #` writes dates as `#yyyy-mm-dd[ hh:nn:ss]#`.
- **Dates.** `Format` accepts the `AMPM` token; `CDate`/`IsDate` accept month names
  (`March 5, 2024`, `5-Mar-24`, `Mar 2024`); `DatePart("ww")` honours the first-day and
  first-week arguments; `DateDiff("w")` counts weeks.
- **Arithmetic types.** `Byte op Byte` is a `Byte`; other Byte/Integer mixes (including
  `Mod`, `\`, `And`/`Or`, unary minus) are `Integer`. Arithmetic on a Variant variable that
  overflows promotes (Integer to Long to Double) instead of raising error 6.
- **Comparison.** Boolean compares with numbers as -1/0. String versus number follows VB's
  Variant rules: a string Variant against a typed number compares numerically (error 13 when
  not numeric); a numeric Variant against a String compares as text; two Variants order
  numbers before strings. Typed String against typed number remains error 13.
- **Default members as values.** An object whose class marks a default member
  (`Attribute Name.VB_UserMemId = 0`) stands for that member where a value is expected:
  `s = obj`, `"x" & obj`, `Print obj`, arithmetic, logical and comparison operators
  (`tests/corpus/100-default-member-value`).
- **UTF-16 strings.** A String is a sequence of UTF-16 code units, stored as UTF-8.
  `Len`, `Left`, `Right`, `Mid`, `Mid` statement, `InStr`, `InStrRev`, `StrReverse`,
  `UCase`/`LCase`/`StrConv` (Latin-1, Latin Extended-A, Greek, Cyrillic case mapping),
  `Like`, text comparison, `Replace`/`Split`, fixed-length strings, `LSet`/`RSet`, `String`,
  `Asc`/`AscW`/`Chr`/`ChrW` and `Byte()` conversion all work per unit. `Chr`/`Asc` map through
  Windows-1252. Source files that are not valid UTF-8 are read as Windows-1252, and text-file
  I/O (`Print #`, `Input #`, `Line Input #`, `Input$`, string `Get`/`Put`) is ANSI on disk.
  The `*B` functions keep reporting stored bytes (REQ-0177). `tests/corpus/101-utf16-strings`,
  `102-ansi-file-text`.
- **Performance.** `Collection` and `Scripting.Dictionary` use a native ordered store with
  a hash index (keyed Add/Item/Exists are O(1)); `s = s & expr` appends in place when the
  operands are side-effect free.

## Verification

`tests/corpus/66-input-variants` through `82-variant-compare` and `100-default-member-value`, plus the evaluator tests.
