' An error in the header of a block (a condition, a bound) is trapped like one
' in any other statement. With Resume Next an If goes on in its Then part and
' a Select Case runs its Case Else (measured under VB6).
Sub Main()
    Dim n As Long, i As Long
    On Error Resume Next
    If (1 \ n) Then
        Print "then"
    Else
        Print "else"
    End If
    Print "if", Err.Number: Err.Clear
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
    ' Statements inside a block still recover one by one.
    For i = 1 To 3
        Print i, 10 \ (i - 2)
    Next i
    Print "end", Err.Number
End Sub
