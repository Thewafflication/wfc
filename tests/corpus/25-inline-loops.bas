Option Explicit
Sub Main()
    Dim i As Long, v As Variant, n As Long
    For i = 1 To 3: Print i;: Next: Print
    For Each v In Array(1, 2): Print v;: Next: Print
    Do While n < 3: n = n + 1: Loop
    Print n
    While n > 0: n = n - 1: Wend
    Print n
    For i = 1 To 2: For n = 1 To 2: Print i * n;: Next n: Next i: Print
    Do: n = n + 1: Loop Until n = 4
    Print n
    On Error Resume Next
    Print Left$("abc", -1)
    Print Err.Number
    Err.Clear
    Print Mid$("abc", 0)
    Print Err.Number
    Err.Clear
    Print Sqr(-1)
    Print Err.Number
End Sub
