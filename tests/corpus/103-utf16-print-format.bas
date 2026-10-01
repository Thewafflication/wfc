Dim e As String

Function A(ByVal s As String) As String
    A = Replace(Replace(s, e, "e"), ChrW(201), "E")
End Function

Function Tabbed(ByVal n As Integer) As String
    Dim p As String, s As String
    p = Environ("TEMP") & "\wfc_t103.txt"
    Open p For Output As #1
    If n = 0 Then
        Print #1, "a" & e; Tab(6); "X"
    ElseIf n = 1 Then
        Print #1, e & e; Spc(2); "Y"
    Else
        Print #1, e; ","; e
    End If
    Close #1
    Open p For Input As #1
    Line Input #1, s
    Close #1
    Kill p
    Tabbed = A(s)
End Function

Sub Main()
    e = ChrW(233)
    Print Tabbed(0): Print Tabbed(1)
    Print A(Format(e & "b", ">")), A(Format("AB", "<")), A(Format(e, "@@@")) & "|", A(Format(e & "x", "!@@@@")) & "|"
    Print A(Format(5, "0 """ & e & """")), A(Format(12, "\" & e & "0"))
    Print Format(ChrW(201), "<") = e, Format(e, ">") = ChrW(201), Format(e, "@") = e
    Print Len(Format(e, "&&&")), A(Format("abc", ">@@@@@")) & "|"
    Print Right(Format(e, "@@@@"), 1) = e, Len(Format(e, "@@@@"))
    Print StrComp(e, "f"), e < "f", e > "e", "é" = e
End Sub
