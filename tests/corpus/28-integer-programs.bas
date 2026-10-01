Option Explicit
Function Twice(ByVal n As Integer) As Integer
    Twice = n * 2
End Function
Sub Bump(ByRef n As Integer)
    n = n + 1
End Sub
Sub Swap(ByRef a As Long, ByRef b As Long)
    Dim t As Long
    t = a: a = b: b = t
End Sub
Sub Main()
    Dim i As Integer, j As Integer, a(1 To 5) As Integer, m(1 To 2) As Long
    i = 5: j = 3
    a(i) = j
    a(j) = Twice(i)
    Bump i
    Bump a(1)
    Bump a(1)
    Print i, a(5), a(3), a(1)
    m(1) = 10: m(2) = 20
    Swap m(1), m(2)
    Print m(1), m(2)
    Select Case i
        Case 6: Print "six"
        Case Else: Print "other"
    End Select
    Select Case 2.5
        Case 1 To 2: Print "a"
        Case 2 To 3: Print "b"
    End Select
    Print TypeName(i + j), TypeName(i / j)
End Sub
