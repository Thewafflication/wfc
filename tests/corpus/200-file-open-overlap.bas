Sub Main()
    Dim a, b, text As String
    text = Environ$("TEMP") & "\wfc-open-overlap.txt"
    Open text For Output As #1
    Print #1, "one"
    Print #1, "two"
    Close #1
    Open text For Input As #1
    Open text For Input As #2
    Line Input #1, a
    Line Input #2, b
    Print a; "/"; b
    On Error Resume Next
    Open text For Output As #3
    Print Err.Number
    Err.Clear
    Close
    Kill text
    x = 3
    If x = 3 Then
        Print "ending"
        End
    End If
    Print "not reached"
End Sub
