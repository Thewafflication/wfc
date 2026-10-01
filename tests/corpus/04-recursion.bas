Option Explicit
Function Gcd(a As Long, b As Long) As Long
    If b = 0 Then Gcd = a Else Gcd = Gcd(b, a Mod b)
End Function
Function Hanoi(n As Long) As Long
    If n = 0 Then
        Hanoi = 0
    Else
        Hanoi = 2 * Hanoi(n - 1) + 1
    End If
End Function
Function Fib(n As Long) As Long
    If n < 2 Then
        Fib = n
    Else
        Fib = Fib(n - 1) + Fib(n - 2)
    End If
End Function
Function Power(b As Long, e As Long) As Long
    If e = 0 Then
        Power = 1
    Else
        Power = b * Power(b, e - 1)
    End If
End Function
Sub Main()
    Print Gcd(48, 18) & " " & Hanoi(10) & " " & Fib(15) & " " & Power(2, 10)
End Sub
