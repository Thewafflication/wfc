' A ByRef variable keeps what the procedure did to it before an error left the
' call.
Sub Bump(ByRef n As Long, ByVal v As Variant)
    n = n + 1
    v = v + 1
    n = n + 10
End Sub

Sub Main()
    Dim b As Long, s As Variant
    s = "x"
    On Error Resume Next
    Bump b, s
    Print b, Err.Number
    Err.Clear
    s = 4
    Bump b, s
    Print b, Err.Number
End Sub
