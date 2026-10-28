Option Base 1
Sub Main()
    Dim a, b(3) As Long
    a = Array(10, 20, 30)
    Print LBound(a), UBound(a), LBound(b), UBound(b)
    Dim v As Variant
    ReDim v(1 To 3) As String
    v(2) = "x"
    Print UBound(v), v(2), TypeName(v(1))
    ReDim d(4)
    Print LBound(d), UBound(d)
    d(4) = 7
    ReDim Preserve d(6)
    Print d(4), UBound(d)
End Sub
