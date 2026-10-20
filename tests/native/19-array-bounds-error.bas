Sub Main()
    Dim a(1 To 3) As Long
    Dim i As Long
    For i = 1 To 3
        a(i) = i
    Next i
    Print a(3)
    Print a(4)
    Print "not reached"
End Sub
