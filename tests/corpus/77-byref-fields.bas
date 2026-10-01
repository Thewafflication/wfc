Option Explicit
Type P
    X As Long
    Name As String
End Type
Sub Inc(ByRef n As Long): n = n + 1: End Sub
Sub Upper(ByRef s As String): s = UCase(s): End Sub
Sub Main()
    Dim arr(3) As Long, p As P, ps(1) As P, i As Long
    Inc arr(2): Inc arr(2)
    Inc p.X
    Inc ps(1).X: Inc ps(1).X: Inc ps(1).X
    p.Name = "abc": Upper p.Name
    ps(0).Name = "def": Upper ps(0).Name
    Print arr(2), p.X, ps(1).X, p.Name, ps(0).Name
    For i = 0 To 1: Inc ps(i).X: Next
    Print ps(0).X, ps(1).X
    Inc (p.X)
    Print p.X
End Sub
