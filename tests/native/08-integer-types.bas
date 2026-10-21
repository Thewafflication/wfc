Dim g As Integer

Function Twice(ByVal n As Integer) As Integer
    Twice = n + n
End Function

Sub Bump(x As Integer)
    x = x + 1
End Sub

Sub Main()
    Dim a As Integer, b As Long, c As Boolean
    a = 30000
    b = a
    b = b + 30000
    Print b
    Bump a
    Print a
    g = 100
    Bump g
    Print g
    Print Twice(16000)
    c = 5
    Print c; c And True
    a = c
    Print a
    c = 0
    Print c
    b = -2147483647 - 1
    Print b
    Print -32768; 32767; -1
End Sub
