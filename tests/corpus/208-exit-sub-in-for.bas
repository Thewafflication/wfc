Dim Count As Integer
Dim Grid() As Long

Sub Unreached()
    Dim j As Integer
    For j = 3 To 1
        Grid(1, j) = Grid(2, j)
    Next j
    Print "unreached loop ok"
End Sub

Sub Visit(ByVal i As Integer)
    Count = Count + 1
End Sub

Sub FindFirst(ByVal wanted As Integer)
    Dim i As Integer
    For i = 1 To 5
        Visit i
        If i = wanted Then Exit Sub
    Next i
    Print "not found"
End Sub

Function Search(ByVal wanted As Integer) As Integer
    Dim i As Integer
    For i = 1 To 5
        Visit i
        If i = wanted Then Exit Function
    Next i
    Search = -1
End Function

Sub EachTest()
    Dim v As Variant
    For Each v In Array(1, 2, 3)
        Visit v
        If v = 2 Then Exit Sub
    Next v
    Print "each fell through"
End Sub

Sub Main()
    Unreached
    FindFirst 2
    Print Count
    Count = 0
    Print Search(3), Count
    Count = 0
    Print Search(9), Count
    Count = 0
    EachTest
    Print Count
End Sub
