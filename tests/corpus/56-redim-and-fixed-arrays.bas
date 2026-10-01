Sub Main()
    Dim names(1 To 3) As String * 5
    names(1) = "ab"
    names(2) = "abcdefgh"
    Print "[" & names(1) & "]", "[" & names(2) & "]", Len(names(3)), Len(names(1))
    Dim v As Variant
    ReDim v(1 To 3) As String
    v(2) = "x"
    Print UBound(v), v(2), TypeName(v(1))
    Dim d() As Double
    ReDim d(3) As Double
    d(3) = 1.5
    Print d(3), UBound(d)
    ReDim d(1 To 2, 1 To 2)
    d(2, 2) = 4
    Print d(2, 2)
    Dim s(1 To 2) As String
    s(1) = "a": s(2) = "b"
    Dim t As Variant
    t = s
    Print TypeName(t), UBound(t), t(2)
End Sub
