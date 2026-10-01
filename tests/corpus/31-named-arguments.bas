Option Explicit
Sub Show(msg As String, Optional times As Long = 1)
    Dim i As Long
    For i = 1 To times: Print msg;: Next: Print
End Sub
Sub Main()
    Dim a As New Acc
    Show "hi", times:=3
    Show times:=2, msg:="yo"
    Call Show(msg:="call")
    Print a.Add(1, c:=5), a.Add(a:=2), a.Add(c:=1, a:=1, b:=1)
    On Error Resume Next
    Show nope:=1
    Print Err.Number
End Sub
