Option Explicit
Function Caesar(s As String, shift As Long) As String
    Dim i As Long, c As Long
    For i = 1 To Len(s)
        c = Asc(Mid$(s, i, 1))
        If c >= 65 And c <= 90 Then
            c = (c - 65 + shift + 26) Mod 26 + 65
        ElseIf c >= 97 And c <= 122 Then
            c = (c - 97 + shift + 26) Mod 26 + 97
        End If
        Caesar = Caesar & Chr$(c)
    Next i
End Function
Function Rle(s As String) As String
    Dim i As Long, n As Long
    For i = 1 To Len(s)
        n = n + 1
        If i = Len(s) Then
            Rle = Rle & n & Mid$(s, i, 1)
        ElseIf Mid$(s, i, 1) <> Mid$(s, i + 1, 1) Then
            Rle = Rle & n & Mid$(s, i, 1)
            n = 0
        End If
    Next i
End Function
Sub Main()
    Dim e As String
    e = Caesar("Hello, World!", 3)
    Print e
    Print Caesar(e, -3)
    Print Rle("aaabccdddd")
End Sub
