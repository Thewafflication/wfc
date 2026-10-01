Option Explicit
Dim counter As Long
Sub Bump(ByRef n As Long, Optional amount As Long = 1)
    n = n + amount
End Sub
Function Total(ParamArray v() As Variant) As Double
    Dim i As Long
    For i = LBound(v) To UBound(v)
        Total = Total + v(i)
    Next i
End Function
Function Counter2() As Long
    Static c As Long
    c = c + 1
    Counter2 = c
End Function
Function Describe(Optional name As String = "anon", Optional age As Long = -1) As String
    Describe = name & "/" & age
End Function
Sub Swap(ByRef a As Variant, ByRef b As Variant)
    Dim t As Variant
    t = a: a = b: b = t
End Sub
Function ByValTest(ByVal x As Long) As Long
    x = x * 2
    ByValTest = x
End Function
Sub Main()
    Dim n As Long
    n = 1
    Bump n
    Bump n, 10
    Print n
    Print Total(1, 2, 3.5) & " " & Total()
    Print Counter2() & Counter2() & Counter2()
    Print Describe() & " " & Describe("bob") & " " & Describe(, 7)
    Dim x As Variant, y As Variant
    x = "L": y = 2
    Swap x, y
    Print x & y
    Dim q As Long
    q = 5
    Print ByValTest(q) & " " & q
    counter = 3
    Print counter
End Sub
