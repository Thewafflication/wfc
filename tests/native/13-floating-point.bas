Dim gain As Double

Function Average(ByVal a As Double, ByVal b As Double) As Double
    Average = (a + b) / 2
End Function

Function Scale(ByVal x As Single, ByVal factor As Single) As Single
    Scale = x * factor
End Function

Sub Halve(ByRef v As Double)
    v = v / 2
End Sub

Function Hypot(ByVal a As Double, ByVal b As Double) As Double
    Hypot = Sqr(a * a + b * b)
End Function

Sub Main()
    Dim a As Single, b As Long, c As Integer, d As Double
    Dim i As Long, total As Double, flag As Boolean
    a = 1.5
    b = 3
    c = 2
    d = 0.1
    Print a + b; a + c; b / c; c / b; b \ c; 7 / 2
    Print 1 / 3; a / 3; d * 3; 1E15; 1E-5; 0.1 + 0.2
    Print CInt(2.5); CInt(3.5); CInt(-2.5); Int(-2.5); Fix(-2.5); CLng(2.5)
    Print 10 \ 3.7; 10 Mod 3.7; Sqr(16); Abs(-3); Sgn(-5); Abs(-2.5); Sgn(0.5)
    Print a; d; CSng(0.1); CDbl(CSng(0.1))
    Print Average(3, 4); Average(1.5, 2.5); Scale(1.1, 3); Hypot(3, 4)
    gain = 8
    Halve gain
    Halve gain
    Print gain
    For i = 1 To 10
        total = total + 1 / i
    Next i
    Print total
    flag = d < 0.2
    Print flag; d > a; a = 1.5; -a; -d
    Print "pi is " & 3.14159 & " and " & CStr(2.5) & Str(7.25) & Str(-1.5)
    Print Sin(0); Cos(0); Atn(1) * 4; Exp(1); Log(10)
    Print 2.5 * 4; 1E10; 123456.789; 0.000123; 100000000000000#
    Print CDbl(5) / 2; CInt(-0.5); CInt(0.5); Int(2.999); Int(-0.0001)
    If d Then Print "d is true"
    If 0.0 Then Print "no" Else Print "zero is false"
End Sub
