' A comma in Print moves to the next zone of 14 columns.
Sub Main()
    Dim s As String
    s = "abcdefghijklmnopq"
    Print 1, 2, 3
    Print "a", "b", "c"
    Print "x"; "y", "z"
    Print 1.5, -2.25, 3
    Print s, "after"
    Print "left",
    Print "right"
    Print True, False, 12345678
    Print "ab"; 7; "cd", 9
    Print
    Print , "second"
End Sub
