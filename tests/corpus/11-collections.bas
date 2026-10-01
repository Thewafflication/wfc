Option Explicit
Sub Main()
    Dim c As New Collection
    Dim v As Variant
    c.Add "one"
    c.Add "two", "k2"
    c.Add "three"
    Print c.Count & " " & c("k2") & " " & c(3)
    For Each v In c
        Print v;
        Print ",";
    Next
    Print
    c.Remove 1
    Print c.Count & " " & c(1)
    On Error Resume Next
    Print c.Item("nope")
    Print Err.Number
End Sub
