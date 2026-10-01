Sub Main()
    Dim p As String, s As String, f As String * 4, n As Integer
    p = Environ("TEMP") & "\wfc_u2.txt"
    Open p For Output As #1
    Print #1, "caf" & ChrW(233) & " " & ChrW(8364)
    Write #1, "x" & ChrW(233)
    Close #1
    Print FileLen(p)
    Open p For Input As #1
    Line Input #1, s
    Print Len(s), s = "caf" & ChrW(233) & " " & ChrW(8364)
    Input #1, s
    Print Len(s), s = "x" & ChrW(233)
    Close #1
    Open p For Binary As #1
    s = String(5, " ")
    Get #1, 1, s
    Print s = "caf" & ChrW(233) & " "
    Close #1
    Kill p
End Sub
