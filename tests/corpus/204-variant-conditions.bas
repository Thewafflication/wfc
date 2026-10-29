' Conditions and selectors that hold Variants: a text that is no number is a
' Type mismatch (trappable), Empty is 0 or "", Null selects no Case, and the
' skipped branches run nothing.
Sub Main()
    Dim v As Variant, w As Variant
    On Error Resume Next
    v = "A,B"
    If v Then
        Print "then"
    Else
        Print "else"
    End If
    Print "if", Err.Number: Err.Clear
    w = 3
    If w Then Print "num"
    v = "7"
    If v Then Print "numeric text"
    v = Empty
    Select Case v
    Case 0, 1
        Print "empty is 0"
    Case Else
        Print "else"
    End Select
    v = Null
    Select Case v
    Case 0, 1
        Print "01"
    Case 2 To 10
        Print "range"
    Case Else
        Print "null selects Else"
    End Select
    Print "select", Err.Number: Err.Clear
    v = "x"
    Select Case 5
    Case 1
        Print (v ^ 2)
    End Select
    Print "end", Err.Number
End Sub
