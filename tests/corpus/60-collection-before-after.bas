Sub Main()
    Dim c As New Collection, v As Variant
    c.Add "b", "kb"
    c.Add "d"
    c.Add "a", , 1
    c.Add "c", , , "kb"
    c.Add "e", , "kb"
    For Each v In c: Print v;: Next: Print
    Print c.Count, c(1), c("kb"), c(5)
    On Error Resume Next
    c.Add "x", , 1, 2
    Print Err.Number
End Sub
