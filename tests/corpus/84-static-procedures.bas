Static Function Counter() As Long
    Dim n As Long
    n = n + 1
    Counter = n
End Function
Private Static Sub Show()
    Dim calls As Long: calls = calls + 1
    Print "calls=" & calls
End Sub
Sub Main()
    Print Counter(), Counter(), Counter()
    Show : Show
End Sub
