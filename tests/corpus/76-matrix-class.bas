Sub Main()
    Dim a As New Matrix, b As New Matrix, p As Matrix
    a.Init 2, 3: b.Init 3, 2
    Dim i As Long, j As Long
    For i = 1 To 2: For j = 1 To 3
        a.Cell(i, j) = i + j
        b.Cell(j, i) = i * j
    Next j, i
    Print a.ToString(), b.ToString()
    Set p = a.Times(b)
    Print p.ToString(), p.Rows, p.Cols
    On Error Resume Next
    Set p = a.Times(a)
    Print Err.Number, Err.Description, Err.Source
End Sub
