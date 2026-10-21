Sub Main()
    Dim i As Long, j As Long, n As Long
    Dim ok As Boolean
    For i = 10 To 1 Step -3
        Print i;
    Next i
    Print
    For i = 1 To 3
        For j = 1 To 3
            If j = 2 Then Exit For
            n = n + i * j
        Next j
    Next i
    Print n
    i = 0
    Do
        i = i + 1
        If i = 5 Then Exit Do
    Loop
    Print i
    i = 10
    Do Until i <= 7
        i = i - 1
    Loop
    Print i
    Do
        i = i + 1
    Loop While i < 12
    Print i
    Do
        i = i - 1
    Loop Until i < 3
    Print i
    While i < 6
        i = i + 2
    Wend
    Print i
    ok = (i > 3) And (i < 100) Or False
    Print ok; Not ok; True Xor ok
    Print -5; Not 5; 6 And 3; 6 Or 3; 6 Xor 3
    If i = 1 Then Print "one" Else Print "not one"
    If i > 100 Then
        Print "big"
    ElseIf i > 5 Then
        Print "medium"
    Else
        Print "small"
    End If
    Print 2147483647; -2147483647
    Print &H7FFFFFFF; &HFF; &H8000
End Sub
