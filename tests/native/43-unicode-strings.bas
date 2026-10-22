Option Explicit

Sub Main()
    Dim s As String, t As String, i As Long
    s = "caf" & ChrW(233) & " " & ChrW(8364) & "uro"
    Print Len(s); Left(s, 4) = "caf" & ChrW(233); AscW(Mid$(s, 4, 1)); AscW(Right(s, 1))
    Print InStr(s, "uro"); InStr(5, s, "u"); Len(UCase(s)); UCase$(Mid(s, 4, 1)) = ChrW(201)
    t = Chr(200)
    Print Len(t); Asc(t); Len(Chr(65) & Chr(240))
    For i = 1 To Len(s)
        Print AscW(Mid(s, i, 1));
    Next i
    Print
    Print LCase(ChrW(201) & "A") = ChrW(233) & "a"
End Sub
