Function Factorial(ByVal n As Long) As Long
    If n <= 1 Then
        Factorial = 1
    Else
        Factorial = n * Factorial(n - 1)
    End If
End Function

Function Fib(ByVal n As Long) As Long
    If n < 2 Then
        Fib = n
        Exit Function
    End If
    Fib = Fib(n - 1) + Fib(n - 2)
End Function

Sub Swap(a As Long, b As Long)
    Dim t As Long
    t = a
    a = b
    b = t
End Sub

Function Gcd(ByVal a As Long, ByVal b As Long) As Long
    Do While b <> 0
        Dim r As Long
        r = a Mod b
        a = b
        b = r
    Loop
    Gcd = a
End Function

Sub Many(ByVal a As Long, ByVal b As Long, ByVal c As Long, ByVal d As Long, ByVal e As Long, ByVal f As Long)
    Print a; b; c; d; e; f
End Sub

Sub Main()
    Dim x As Long, y As Long, i As Integer
    Print Factorial(10)
    For i = 0 To 15
        Print Fib(i);
    Next i
    Print
    x = 3
    y = 4
    Swap x, y
    Print x; y
    Swap x, x
    Print Gcd(84, 36); Gcd(17, 5)
    Many 1, 2, 3, 4, 5, 6
    Call Many(10, 20, 30, 40, 50, 60)
End Sub
