Sub Main()
    Dim s As String, f As String * 5
    s = "h" & ChrW(233) & "llo " & ChrW(8364) & "x"
    Print Len(s), Left(s, 2) = "h" & ChrW(233), Right(s, 2) = ChrW(8364) & "x", Mid(s, 2, 1) = ChrW(233)
    Print Asc(Mid(s, 2, 1)), AscW(Mid(s, 2, 1)), Asc(Chr(128)), AscW(Chr(128)), Asc(ChrW(8364))
    Print InStr(s, ChrW(8364)), InStrRev(s, "l"), InStr(1, UCase(s), "H" & ChrW(201)), StrReverse("a" & ChrW(233) & "b") = "b" & ChrW(233) & "a"
    Print UCase(ChrW(233)) = ChrW(201), LCase(ChrW(201)) = ChrW(233), Len(ChrW(&H1F600 - &H10000 + 0))
    Print Len(Chr(233)), Asc(Chr(233)), "a" & ChrW(233) Like "a?", StrComp(ChrW(233), ChrW(201), 1)
    f = ChrW(233) & "ab"
    Print Len(f), "[" & f & "]" = "[" & ChrW(233) & "ab  ]", LenB("abc"), LenB(ChrW(233))
    Mid(s, 2, 1) = "e": Print s = "hello " & ChrW(8364) & "x"
    Dim t As String: t = Space(3): Mid(t, 2) = ChrW(233) & "z": Print Len(t), t = " " & ChrW(233) & "z"
    Print Len(Replace(s, ChrW(8364), "EUR")), Len(Join(Split("a" & ChrW(233) & "b,c", ","), ""))
    Print Trim(ChrW(233) & " ") = ChrW(233), String(2, ChrW(233)) = ChrW(233) & ChrW(233), Len(Format(ChrW(233), ">"))
End Sub
