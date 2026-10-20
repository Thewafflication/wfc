Function Pad(ByVal s As String, ByVal n As Long) As String
    Do While Len(s) < n
        s = s & "."
    Loop
    Pad = s
End Function

Sub Swap(ByRef a As String, ByRef b As String)
    Dim t As String
    t = a
    a = b
    b = t
End Sub

Sub Main()
    Dim x As String, y As String, i As Long
    x = "left"
    y = "right"
    Swap x, y
    Print x; " "; y
    Swap x, x
    Print x
    Print Pad("ab", 6)
    Print Left("abc", 10); Right("abc", 10); Mid("abc", 10); "|"
    Print Mid("abcdef", 2, 100); Mid("abcdef", 6, 1)
    For i = 1 To 3
        x = x & Str(i)
    Next i
    Print x
    Print Len(Pad("", 3))
    y = Pad("z", 3) & Pad("q", 2)
    Print y
End Sub
