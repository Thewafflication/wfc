Sub Main()
    Dim i As Integer, s As String, a(1 To 5) As Long, d As Double
    i = 3: s = "hello world"
    Print Mid$(s, i, i), Left$(s, i), Right$(s, i), Chr(i + 62), String(i, "x"), Space(i) & "|", InStr(i, s, "o"), Hex(i), Oct(i)
    Print Format(i, "00"), Round(d, i), Int(i / 2), Abs(-i), Sgn(i), Sqr(i * i), i ^ 2, i Mod 2, i \ 2
    a(i) = i * 2: Print a(i), UBound(a, i - 2), Choose(i, "a", "b", "c"), Weekday(i), MonthName(i), DateSerial(i, i, i)
    Dim v As Variant
    v = i: Print TypeName(v), VarType(v), v + 1
    ReDim b(i) As Integer
    Print UBound(b), TypeName(i + i)
    Print Timer > 0, RGB(i, i, i), QBColor(i), Asc(Chr(i + 64)), StrReverse(CStr(i)), CStr(i) & i
End Sub
