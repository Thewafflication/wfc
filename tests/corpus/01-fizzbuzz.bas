Option Explicit
Sub Main()
    Dim i As Long, s As String
    For i = 1 To 15
        If i Mod 15 = 0 Then
            s = "FizzBuzz"
        ElseIf i Mod 3 = 0 Then
            s = "Fizz"
        ElseIf i Mod 5 = 0 Then
            s = "Buzz"
        Else
            s = CStr(i)
        End If
        Print s
    Next i
End Sub
