Const Size = 5
Dim table(1 To Size) As Long
Dim names(0 To 2) As String
Dim grid(2, 3) As Integer
Dim scores() As Double

Sub Fill(values() As Long, ByVal n As Long)
    Dim i As Long
    For i = LBound(values) To UBound(values)
        values(i) = i * n
    Next i
End Sub

Function Total(values() As Long) As Long
    Dim i As Long, t As Long
    For i = LBound(values) To UBound(values)
        t = t + values(i)
    Next i
    Total = t
End Function

Sub Main()
    Dim i As Long, j As Long
    Dim local(10) As Long
    Dim words() As String
    Fill table, 3
    Print Total(table); table(1); table(5); LBound(table); UBound(table)
    names(0) = "zero"
    names(1) = "one"
    names(2) = names(0) & "+" & names(1)
    Print names(0); names(1); names(2); Len(names(2))
    For i = 0 To 2
        For j = 0 To 3
            grid(i, j) = i * 10 + j
        Next j
    Next i
    Print grid(0, 0); grid(1, 2); grid(2, 3); UBound(grid); UBound(grid, 2)
    ReDim scores(1 To 3)
    scores(1) = 1.5
    scores(2) = scores(1) * 2
    scores(3) = scores(2) + 0.25
    Print scores(1); scores(2); scores(3)
    ReDim Preserve scores(1 To 5)
    scores(5) = 9
    Print scores(1); scores(3); scores(4); scores(5); UBound(scores)
    ReDim words(0 To 2)
    words(0) = "a"
    words(1) = "b"
    words(2) = "c"
    ReDim Preserve words(0 To 4)
    words(4) = "e"
    For i = 0 To 4
        Print "[" & words(i) & "]";
    Next i
    Print
    For i = 0 To 10
        local(i) = i * i
    Next i
    Print local(10); local(3)
    Erase table
    Print table(2)
    Erase scores
    Print "done"
End Sub
