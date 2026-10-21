Function Twice(ByVal s As String) As String
    Twice = s & s
End Function

Function Label(ByVal n As Long) As String
    Label = "item" & n & ":" & Str(n)
End Function

Sub Main()
    Dim i As Long, total As Long, s As String, t As String
    For i = 1 To 3000
        s = Label(i)
        t = Twice(Left(s, 4)) & Mid(s, 5)
        total = total + Len(t)
        If InStr(t, "99") > 0 Then total = total + 1
    Next i
    Print total
    s = ""
    For i = 1 To 300
        s = s & Chr(65 + i Mod 26)
    Next i
    Print Len(s); Left(s, 10); Right(s, 10)
End Sub
