Option Explicit
Sub Main()
    Dim a(1 To 2, 1 To 3) As Long, b(1 To 3, 1 To 2) As Long, c(1 To 2, 1 To 2) As Long
    Dim i As Long, j As Long, k As Long
    For i = 1 To 2
        For j = 1 To 3
            a(i, j) = i + j
            b(j, i) = i * j
        Next j
    Next i
    For i = 1 To 2
        For j = 1 To 2
            For k = 1 To 3
                c(i, j) = c(i, j) + a(i, k) * b(k, j)
            Next k
        Next j
    Next i
    Print c(1, 1) & " " & c(1, 2) & " " & c(2, 1) & " " & c(2, 2)
    Print UBound(a, 1) & UBound(a, 2) & LBound(c, 1)
End Sub
