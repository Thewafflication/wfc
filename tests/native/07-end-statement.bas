Sub Stop1(ByVal n As Long)
    If n > 2 Then
        Print "ending"
        End
    End If
End Sub

Sub Main()
    Dim i As Long
    For i = 1 To 10
        Print i
        Stop1 i
    Next i
    Print "not reached"
End Sub
