Const MaxItems = 5
Const Greeting As String = "Hi, " & "there"
Const Pi As Double = 3.14159
Const Mask As Long = &HFF And 7
Public Const Limit = 10 * 2 + 1

Function Classify(ByVal n As Long) As String
    Select Case n
        Case Is < 0
            Classify = "negative"
        Case 0
            Classify = "zero"
        Case 1, 2, 3
            Classify = "small"
        Case 4 To 9
            Classify = "medium"
        Case Is >= 100, 50
            Classify = "big"
        Case Else
            Classify = "other"
    End Select
End Function

Function Weekday3(ByVal s As String) As Long
    Select Case s
        Case "mon", "monday"
            Weekday3 = 1
        Case "tue"
            Weekday3 = 2
        Case Else
            Weekday3 = 0
    End Select
End Function

Sub Main()
    Const Local As Long = 3
    Dim i As Long, n As Long
    For i = -1 To 11
        Print Classify(i); " ";
    Next i
    Print
    Print Classify(100); Classify(50); Classify(77)
    Print Weekday3("mon"); Weekday3("tue"); Weekday3("x"); Weekday3("monday")
    Print MaxItems; Greeting; Pi; Mask; Limit; Local
    Print MaxItems * 2 + Local; Pi * 2
    i = 0
top:
    i = i + 1
    If i < 5 Then GoTo top
    Print i
    GoTo skip
    Print "skipped"
skip:
    Select Case 2.5
        Case 1 To 2
            Print "low"
        Case 2 To 3
            Print "mid"
    End Select
    n = 7
    Select Case n Mod 3
        Case 0: Print "div"
        Case Else: Print "rem"; n Mod 3
    End Select
End Sub
