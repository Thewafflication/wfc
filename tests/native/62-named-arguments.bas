Sub Show(msg As String, Optional times As Long = 1)
    Dim i As Long
    For i = 1 To times: Print msg;: Next: Print
End Sub

Function Mix(a As Long, Optional b As Long = 10, Optional c As Long = 100) As Long
    Mix = a + b * 2 + c * 3
End Function

Sub Main()
    Show "hi", times:=3
    Show times:=2, msg:="yo"
    Call Show(msg:="call")
    Print Mix(1), Mix(1, c:=5), Mix(c:=1, a:=1, b:=1)
    On Error Resume Next
    Show nope:=1
    Print Err.Number
End Sub
