Option Explicit
Sub Main()
    Dim s As New Stack
    s.Push 10
    s.Push 20
    s.Push 30
    Print s.Count & " " & s.Peek()
    Print s.Pop() & " " & s.Pop() & " " & s.Count
    Dim q As New Account
    q.Deposit 100
    q.Withdraw 30
    On Error Resume Next
    q.Withdraw 500
    Print q.Balance & " " & Err.Number & " " & Err.Description
End Sub
