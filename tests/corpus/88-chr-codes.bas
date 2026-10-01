Sub Main()
    Dim s As String
    s = "abc" & vbNullChar & "def"
    Print Len(s), InStr(s, vbNullChar), Left$(s, InStr(s, vbNullChar) - 1), Asc(Mid$(s, 4, 1))
    s = String$(5, 0): Print Len(s), Asc(s)
    Print Asc(Chr(200)), Asc(Chr$(255)), Len(Chr(0)), Chr(65) & Chr$(66)
    Print AscW(ChrW(8364)), AscW(ChrW(233)), AscW(ChrW(65)), Len(ChrW(8364)) > 1
    Print Len(vbCrLf), Len(vbCr), Len(vbLf), vbTab = Chr(9), vbBack = Chr(8), vbFormFeed = Chr(12), vbVerticalTab = Chr(11)
End Sub
