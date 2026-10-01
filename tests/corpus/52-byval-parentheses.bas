Option Explicit
Sub Inc(ByRef n As Long)
    n = n + 1
End Sub
Function Add3(a As Long, b As Long, c As Long) As Long
    Add3 = a + b + c
End Function
Sub Main()
    Dim x As Long
    x = 1
    Inc x
    Print x
    Inc (x)
    Print x
    Call Inc(x)
    Print x
    Inc x + 0
    Print x
    Print Add3(1, 2, 3), Add3(x, (x), x + 1)
    Dim s As String
    For x = 1 To 3
        Select Case x
            Case 2: Exit For
        End Select
        s = s & x
    Next
    Print s, x
    Do
        x = x + 1
        Select Case x
            Case 5: Exit Do
        End Select
    Loop
    Print x
End Sub
