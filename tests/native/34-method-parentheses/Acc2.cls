Public total As Long
Public Sub Add(ByRef n As Long)
    total = total + n
    n = 0
End Sub
Public Sub Two(a As Long, b As Long)
    total = total + a * b
End Sub
