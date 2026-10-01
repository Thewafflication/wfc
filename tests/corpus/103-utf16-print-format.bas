Sub Main()
    Dim e As String: e = ChrW(233)
    Print "a" & e; Tab(6); "X"
    Print e & e; Spc(2); "Y"
    Print Format(e & "b", ">"), Format("AB", "<") , Format(e, "@@@") & "|", Format(e & "x", "!@@@@") & "|"
    Print Format(5, "0 """ & e & """"), Format(12, "\" & e & "0")
    Print Format(ChrW(201), "<") = e, Format(e, ">") = ChrW(201), Format(e, "@") = e
    Print Len(Format("é", "&&&")), Format("abc", ">@@@@@") & "|"
    Print Right(Format(e, "@@@@"), 1) = e, Len(Format(e, "@@@@"))
    Print Len(Left(e & "bc" & Space(3), 3)), Len(Trim(e & " "))
    Print Len(FormatCurrency(1)), Len(Hex(255))
    Print StrComp(e, "f"), e < "f", e > "e", "é" = e
End Sub
