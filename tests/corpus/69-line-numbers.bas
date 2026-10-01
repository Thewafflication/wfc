Option Explicit
Function Safe(ByVal d As Long) As String
    On Error GoTo bad
10  Dim r As Long
20  r = 100 \ d
30  Safe = "ok " & r
    Exit Function
bad:
    Safe = "err " & Err.Number & " at " & Erl
End Function
Sub Main()
    Dim i As Long
100 i = i + 1
110 If i < 3 Then GoTo 100
120 Print "i=" & i
    Print Safe(5)
    Print Safe(0)
    On i GoTo 200, 210
200 Print "first": GoTo 300
210 Print "second"
300 Print "end"
End Sub
