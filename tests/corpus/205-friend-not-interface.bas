Sub Main()
    Dim a As Square, b As Square, s As IShape
    Set a = New Square
    a.Side = 4
    Set b = New Square
    b.Copy a
    Set s = b
    Print s.Area
End Sub
