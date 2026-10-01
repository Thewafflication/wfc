Option Explicit
Sub Level3()
    Err.Raise 1001, "Level3", "deep failure"
End Sub
Sub Level2()
    Level3
    Print "not reached"
End Sub
Sub Level1()
    On Error GoTo h
    Level2
    Print "not reached 2"
    Exit Sub
h:
    Print "L1 caught " & Err.Number & " " & Err.Source & " " & Err.Description
    Err.Raise Err.Number, "Level1", "rethrown: " & Err.Description
End Sub
Function Safe(ByVal n As Long) As String
    On Error GoTo bad
    Safe = "ok " & (10 \ n)
    Exit Function
bad:
    Safe = "err " & Err.Number
End Function
Sub Main()
    On Error GoTo top
    Level1
    Print "not reached 3"
    GoTo done
top:
    Print "top caught: " & Err.Description & " [" & Err.Source & "]"
    Resume done
done:
    Print Safe(2), Safe(0)
    Dim r As Long
    On Error Resume Next
    r = CLng("abc")
    Print "after", Err.Number
    Err.Clear
    Open "nonexistent.txt" For Input As #1
    Print Err.Number, Err.Description
    On Error GoTo 0
    Dim i As Integer
    On Error GoTo cnt
    For i = 1 To 3
        If i = 2 Then Error 9
        Print "i=" & i
    Next
    Exit Sub
cnt:
    Print "cnt " & Err.Number & " at i=" & i
    Resume Next
End Sub
