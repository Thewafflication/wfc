Option Explicit

Function Join2(ByVal sep As String, ParamArray Args() As Variant) As String
    Dim i As Long, s As String
    For i = LBound(Args) To UBound(Args)
        If i > LBound(Args) Then s = s & sep
        s = s & CStr(Args(i))
    Next i
    Join2 = s
End Function

Sub Main()
    Dim v() As Variant
    ReDim v(1 To 3)
    Dim i As Long
    v(1) = 10
    v(2) = "two"
    v(3) = 3.5
    For i = 1 To 3
        Print v(i)
    Next i
    ReDim Preserve v(1 To 5)
    v(5) = "five"
    Print UBound(v); v(5); IsEmpty(v(4))
    Print Join2("-", 1, "b", 2.5)
    Print Join2("+")
    Print Join2(",", "x")
    Dim o() As Object
    ReDim o(2)
End Sub
