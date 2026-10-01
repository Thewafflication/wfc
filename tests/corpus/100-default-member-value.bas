Option Explicit
Sub Main()
    Dim a As New Named, s As String, n As Long
    a.Name = "Rex"
    a.Size = 42
    s = a
    Print "[" & s & "]"
    Print "[" & a & "]"
    Print a
    Print a = "Rex", a <> "Max"
    Dim b As New Sized
    b.Size = 41
    n = b + 1
    Print n, b > 40, b & "!"
End Sub
