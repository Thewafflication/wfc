Type Piece
    name As String
    x As Long
    y As Long
    weight As Double
End Type

Dim board(1 To 3) As Piece
Dim roster() As Piece

Sub Place(pieces() As Piece, ByVal index As Long, ByVal label As String)
    pieces(index).name = label
    pieces(index).x = index * 10
    pieces(index).y = index * 100
    pieces(index).weight = index / 4
End Sub

Function Count(pieces() As Piece) As Long
    Count = UBound(pieces) - LBound(pieces) + 1
End Function

Sub Main()
    Dim i As Long, local(0 To 2) As Piece, one As Piece
    For i = 1 To 3
        Place board, i, "piece" & i
    Next i
    For i = 1 To 3
        Print board(i).name; board(i).x; board(i).y; board(i).weight
    Next i
    ReDim roster(1 To 2)
    roster(1) = board(2)
    roster(2).name = "second"
    roster(2).x = 7
    board(2).name = "changed"
    Print roster(1).name; roster(1).x; roster(2).name; roster(2).x
    ReDim Preserve roster(1 To 4)
    roster(4).name = "fourth"
    Print Count(roster); roster(1).name; roster(2).name; roster(4).name
    local(1).name = "L1"
    one = local(1)
    one.name = one.name & "!"
    Print local(1).name; one.name; Count(local)
    board(3) = roster(2)
    Print board(3).name; board(3).x
    Erase roster
    On Error Resume Next
    Print roster(1).name
    Print Err.Number
End Sub
