Option Explicit
Sub Main()
    Dim j As Long, n As Long, flags(1 To 10) As Boolean, i As Long, cnt As Long
    If True Then n = 1: For j = 1 To 3: n = n + j: Next
    Print n
    If n > 0 Then Do While n < 20: n = n + 5: Loop
    Print n
    If n > 5 Then While n > 5: n = n - 1: Wend
    Print n
    For i = 2 To 10
        If Not flags(i) Then cnt = cnt + 1: For j = i * 2 To 10 Step i: flags(j) = True: Next
    Next
    Print cnt
End Sub
