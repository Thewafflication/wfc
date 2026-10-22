Private marks(10) As Long
Private cells(2) As Side
Public Total As Long

Public Sub Mark(ByVal n As Long)
    marks(n) = marks(n) + 1
    Total = Total + n
    cells(1).pipe = Total
End Sub

Public Function Count(ByVal n As Long) As Long
    Count = marks(n)
End Function
