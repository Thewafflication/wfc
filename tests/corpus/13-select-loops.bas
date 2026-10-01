Option Explicit
Function Grade(score As Long) As String
    Select Case score
        Case Is >= 90: Grade = "A"
        Case 80 To 89: Grade = "B"
        Case 70, 71, 72, 73, 74, 75, 76, 77, 78, 79: Grade = "C"
        Case Else: Grade = "F"
    End Select
End Function
Sub Main()
    Dim s As Long, out As String
    For Each s In Array(95, 85, 75, 50)
        out = out & Grade(s)
    Next
    Print out
    Dim n As Long, steps As Long
    n = 27
    Do While n <> 1
        If n Mod 2 = 0 Then n = n \ 2 Else n = 3 * n + 1
        steps = steps + 1
    Loop
    Print steps
    Dim i As Long, total As Long
    i = 0
    Do
        i = i + 1
        If i Mod 2 = 0 Then GoTo skip
        total = total + i
skip:
    Loop Until i >= 10
    Print total
    For i = 10 To 1 Step -3
        Print i;
        Print " ";
    Next i
    Print
    i = 0
    While i < 3
        i = i + 1
    Wend
    Print i
End Sub
