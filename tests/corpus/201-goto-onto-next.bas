Sub Main()
    Dim i As Long, n As Long
    For i = 1 To 5
        If i = 2 Then GoTo skip
        Print i;
skip: Next i
    Print
    For i = 1 To 5
        If i = 2 Then GoTo skip2
        Print i;
skip2:
    Next i
    Print
    n = 0
    Do While n < 5
        n = n + 1
        If n = 3 Then GoTo again
        Print n;
again: Loop
    Print
    n = 0
    While n < 5
        n = n + 1
        If n = 4 Then GoTo more
        Print n;
more: Wend
    Print
End Sub
