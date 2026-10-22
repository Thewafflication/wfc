Sub Main()
    Dim a As New Acc2, v As Long
    v = 5
    a.Add (v)
    Print a.total, v
    a.Add v
    Print a.total, v
    v = 3
    a.Add(v)
    Print a.total, v
    a.Two 2, 3
    a.Two(4, 5)
    Print a.total
    With a
        .Add (7)
        .Two 1, 1
    End With
    Print a.total
End Sub
