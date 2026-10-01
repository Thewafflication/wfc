Option Explicit
Function Depth(ByVal n As Long) As Long
    If n = 0 Then
        Depth = 0
    Else
        Depth = Depth(n - 1) + 1
    End If
End Function
Function SumTo(ByVal n As Long) As Long
    If n <= 0 Then SumTo = 0 Else SumTo = n + SumTo(n - 1)
End Function
Sub Main()
    Print Depth(100)
    Print Depth(1200)
    Print SumTo(1000)
    On Error Resume Next
    Print Depth(1000000)
    Print Err.Number, Err.Description
End Sub
