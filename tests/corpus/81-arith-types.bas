Option Explicit
Sub Main()
    Dim b As Byte, i As Integer, l As Long, s As Single, d As Double, c As Currency
    b = 5: i = 5: l = 5: s = 5: d = 5: c = 5
    Print TypeName(b + b), TypeName(b + 1), TypeName(b + i), TypeName(i + i), TypeName(i + l), TypeName(l + l)
    Print TypeName(b * b), TypeName(b - 1), TypeName(i * 2), TypeName(l * 2), TypeName(i / 2), TypeName(b \ 2)
    Print TypeName(i + s), TypeName(l + s), TypeName(l + d), TypeName(i + c), TypeName(c * 2), TypeName(c / 2)
    Print TypeName(i Mod 2), TypeName(b Mod 2), TypeName(-b), TypeName(-i), TypeName(Not b), TypeName(Not i)
    Print TypeName(b And b), TypeName(b And i), TypeName(i Or l), TypeName(i ^ 2), TypeName(b & b)
    Dim v As Variant
    v = 32767: v = v + 1: Print v, TypeName(v)
    v = 2147483647: v = v + 1: Print v, TypeName(v)
    v = 30000: v = v * 3: Print v, TypeName(v)
    v = b: v = v * 100: Print v, TypeName(v)
    On Error Resume Next
    i = 32767: i = i + 1: Print Err.Number: Err.Clear
    b = 255: b = b + b: Print Err.Number: Err.Clear
    l = 2147483647: l = l + 1: Print Err.Number
End Sub
