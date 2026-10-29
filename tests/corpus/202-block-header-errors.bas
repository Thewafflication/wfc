' An error in the header of a block (a condition, a bound) is trapped like one
' in any other statement: with Resume Next the block is skipped.
Sub Main()
    Dim n As Long, i As Long
    On Error Resume Next
    If (1 \ n) Then
        Print "then"
    Else
        Print "else"
    End If
    Print "if", Err.Number: Err.Clear
    While 1 \ n
        Print "loop"
    Wend
    Print "while", Err.Number: Err.Clear
    For i = 1 To 10 \ n
        Print "for"
    Next i
    Print "for", Err.Number: Err.Clear
    Select Case 1 \ n
        Case 0
            Print "zero"
        Case Else
            Print "other"
    End Select
    Print "select", Err.Number: Err.Clear
    Do While 5 \ n
        Print "do"
    Loop
    Print "do", Err.Number: Err.Clear
    ' Statements inside a block still recover one by one.
    For i = 1 To 3
        Print i, 10 \ (i - 2)
    Next i
    Print "end", Err.Number
End Sub
