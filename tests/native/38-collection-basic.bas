Option Explicit
Sub Main()
    Dim c As New Collection
    Dim v As Variant
    c.Add 5
    c.Add "text", "k"
    c.Add 3.5
    Print c.Count; c.Item(1); c.Item("k"); c.Item(3)
    Print c(1); c("k")
    c.Remove 1
    Print c.Count; c(1)
    For Each v In c
        Print v;
    Next
    Print
    Print TypeName(c)
    On Error Resume Next
    c.Add 1, "k"
    Print Err.Number
    Err.Clear
    Print c(99)
    Print Err.Number
End Sub
