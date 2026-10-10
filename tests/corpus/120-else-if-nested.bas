Option Explicit
Sub Main()
    Dim a As Long
    For a = 1 To 3
        If a = 2 Then
            Print "two"
        Else If a = 1 Then
            Print "nested one"
        Else
            Print "other"
        End If
        End If
    Next
End Sub
