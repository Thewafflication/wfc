Option Compare Text
Option Base 1
Sub Main()
    Dim c As New Cfg, e As New Cfg
    Print c.Bump, c.Bump, e.Bump
    Print Cfg.ModeA, c.Version, Cfg.Version
    Greeting = "hi"
    Print Twice(21); Greeting
    Say Twice(4)
    Dim d As New D
    d(2) = 7
    Print d(2), d.Item(2)
    Print "ABC" = "abc", InStr("Hello", "L"), "abc" Like "ABC"
    Dim s As String: s = "B"
    Select Case s
        Case "b": Print "matched"
    End Select
End Sub
