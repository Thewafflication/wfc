Option Explicit
Type Side
    edge As Integer
    pipe As Integer
End Type
Type Cell
    kind As Integer
    sides(4) As Side
    names(1 To 3) As String
End Type
Const Size As Long = 3
Dim grid(Size) As Cell

Sub Fill(ByRef c As Cell, ByVal k As Integer)
    Dim i As Long
    c.kind = k
    For i = 1 To 4
        c.sides(i).pipe = k * 10 + i
        c.sides(i).edge = i
    Next i
    c.names(1) = "a" & k
    c.names(2) = "b" & k
    c.names(3) = "c" & k
End Sub

Sub Main()
    Dim i As Long, j As Long, total As Long
    Dim copy As Cell
    For i = 0 To Size
        Fill grid(i), i
    Next i
    For i = 0 To Size
        For j = 1 To 4
            total = total + grid(i).sides(j).pipe
        Next j
    Next i
    Print total
    Print grid(2).names(1); grid(2).names(2); grid(2).names(3)
    copy = grid(1)
    grid(1).sides(2).pipe = 99
    Print copy.sides(2).pipe; grid(1).sides(2).pipe
    Dim b As New Board
    b.Mark 3
    b.Mark 7
    b.Mark 3
    Print b.Count(3); b.Count(7); b.Count(9)
    Print b.Total
    On Error Resume Next
    Print grid(0).sides(5).pipe
    Print Err.Number
End Sub
