' Functions of the VBA library reached through the shared runtime.
Option Explicit

Sub Main()
    Dim s As String, n As Long, d As Double, v As Variant
    Print Format(1234.5, "0.00"); Format(0.256, "0%"); Format(7, "000")
    Print IIf(3 > 2, "yes", "no"); IIf(1 > 2, 1, 2)
    Print Choose(2, "a", "b", "c"); Switch(False, 1, True, 2)
    Print Round(2.567, 2); Round(3.5); Round(2.5)
    Print Replace("a-b-c", "-", "+"); InStrRev("abcabc", "b"); StrReverse("xyz")
    Print Hex(255); Oct(8); Val("42abc"); Val("3.5")
    Print Left$("hello", 2); Right$("hello", 3); Mid$("hello", 2, 3)
    Print UCase$("abc"); LCase$("ABC"); Trim$("  x  "); String(3, "z"); Space(2) & "|"
    Print Asc("A"); Chr$(66); Len("four"); StrComp("a", "b"); StrComp("b", "a")
    Print DateSerial(2024, 3, 15); Year(DateSerial(2024, 3, 15)); Month(DateSerial(2024, 3, 15))
    Print Format(DateSerial(2024, 3, 5), "yyyy-mm-dd"); DateAdd("d", 10, DateSerial(2024, 3, 5))
    Print Weekday(DateSerial(2024, 3, 15)); DateDiff("d", DateSerial(2024, 1, 1), DateSerial(2024, 3, 1))
    n = Len(Format(123456, "#,##0"))
    s = Format(0.5, "Percent")
    d = Val("2.25") * 2
    Print n; s; d
    v = IIf(n > 3, "big", 0)
    Print v; TypeName(v)
    Print Abs(-3); Sgn(-2); Int(-2.5); Fix(-2.5); Sqr(16)
    Print CInt(2.5); CLng(3.5); CDbl("1.5"); CStr(12); CBool(0)
    Print IsNumeric("12"); IsDate("2024-01-01"); IsEmpty(v)
End Sub
