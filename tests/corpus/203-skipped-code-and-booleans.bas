' Code that is skipped runs nothing: an overflow or a type error in it is not
' raised. Booleans order and compare as the numbers they are.
Sub Main()
    Dim c As Long, v As Variant, s As String
    c = 3
    On Error Resume Next
    If c = 0 Then Print ((c * 2147483647) Mod 1E10) \ -1
    Do While c < 0
        Print (((c * 2147483647) Mod 1E10) \ -1)
    Loop
    v = "abc"
    If c = 0 Then Print -v
    Print "skipped", Err.Number: Err.Clear

    Print True < False, False < True, True <= True, True > False
    Print Str$(1 > 2); Str$(2 > 1)

    Select Case c > 1
    Case 0, 1
        Print "zero or one"
    Case Else
        Print "other"
    End Select
    Select Case c > 1
    Case -1
        Print "true"
    Case Else
        Print "other"
    End Select
    v = "5"
    Select Case v
    Case 0, 1
        Print "01"
    Case 2 To 10
        Print "range"
    Case Else
        Print "else"
    End Select
    Print "errors", Err.Number
End Sub
