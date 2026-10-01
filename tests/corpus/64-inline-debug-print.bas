Sub Main()
    Dim n As Long
    For n = 1 To 4
        If n Mod 2 = 0 Then Debug.Print "even " & n
        If n = 3 Then Debug.Print "three"
    Next
    Print "done"
End Sub
