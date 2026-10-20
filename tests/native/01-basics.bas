Dim total As Long

Function Square(ByVal n As Long) As Long
    Square = n * n
End Function

Sub Bump(ByRef x As Long)
    x = x + 10
End Sub

Sub Main()
    Dim i As Long
    Dim flag As Boolean
    Print "hello"
    For i = 1 To 5
        total = total + Square(i)
    Next i
    Print total
    Bump total
    Print total; i
    flag = total > 50
    Print flag
    If total Mod 2 = 0 Then
        Print "even"
    Else
        Print "odd"
    End If
    Do While i > 0
        i = i - 2
    Loop
    Print i
    Print 7 \ 2; -7 \ 2; 7 Mod 3; -7 Mod 3
End Sub
