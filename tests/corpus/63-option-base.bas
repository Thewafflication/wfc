Option Base 1
Function F(ParamArray p() As Variant) As String
    F = LBound(p) & "-" & UBound(p)
End Function
Sub Main()
    Dim a, b(3) As Long, c() As String
    a = Array(10, 20, 30)
    Print LBound(a), UBound(a), LBound(b), UBound(b), F(7, 8)
    c = Split("x y z", " ")
    Print LBound(c), UBound(c)
    ReDim d(4)
    Print LBound(d), UBound(d)
    Dim e(2, 3)
    Print LBound(e, 1), UBound(e, 2)
End Sub
