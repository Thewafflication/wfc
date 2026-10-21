Function Divide(ByVal a As Long, ByVal b As Long) As Long
    Divide = a \ b
End Function

Function Safe(ByVal a As Long, ByVal b As Long) As Long
    On Error GoTo failed
    Safe = Divide(a, b)
    Exit Function
failed:
    Print "failed: "; Err.Number; Err.Description
    Safe = -1
End Function

Sub Counting()
    Dim i As Long, total As Long, a(1 To 3) As Long
    On Error Resume Next
    For i = 0 To 5
        a(i) = i
        total = total + 10 \ i
    Next i
    Print "Resume Next:"; total; Err.Number
    Err.Clear
    Print Err.Number
End Sub

Sub Retry()
    Dim n As Long, divisor As Long
    divisor = 0
    On Error GoTo fix
    n = 10 \ divisor
    Print "n ="; n
    Exit Sub
fix:
    Print "fixing "; Err.Description
    divisor = 5
    Resume
End Sub

Sub NextOne()
    Dim x As Long
    On Error GoTo oops
    x = 1 \ 0
    Print "after"
    x = 7
    Print x
    Exit Sub
oops:
    Print "oops"
    Resume Next
End Sub

Sub Raiser()
    On Error GoTo h
    Err.Raise 1001, "Raiser", "custom failure"
    Print "not here"
    Exit Sub
h:
    Print Err.Number; Err.Description; Err.Source
    Err.Raise 5
End Sub

Sub Main()
    Print Safe(10, 2); Safe(1, 0)
    Counting
    Retry
    NextOne
    On Error Resume Next
    Raiser
    Print "main continues "; Err.Number
    On Error GoTo 0
    Dim s As String
    s = Left("abc", -1)
    Print "not reached"
End Sub
