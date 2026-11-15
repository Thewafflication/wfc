Option Explicit
Function Depth(ByVal n As Long) As Long
    If n = 0 Then
        Depth = 0
    Else
        Depth = Depth(n - 1) + 1
    End If
End Function
Sub Main()
    Print Depth(100000)
End Sub
