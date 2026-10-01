Option Explicit
Dim board(1 To 8) As Long
Dim solutions As Long

Function Safe(ByVal row As Long, ByVal col As Long) As Boolean
    Dim r As Long
    For r = 1 To row - 1
        If board(r) = col Or Abs(board(r) - col) = row - r Then
            Safe = False
            Exit Function
        End If
    Next r
    Safe = True
End Function

Sub Place(ByVal row As Long)
    Dim c As Long
    If row > 8 Then
        solutions = solutions + 1
        Exit Sub
    End If
    For c = 1 To 8
        If Safe(row, c) Then
            board(row) = c
            Place row + 1
        End If
    Next c
End Sub

Sub Hanoi(ByVal n As Long, f As String, t As String, v As String, ByRef moves As Long)
    If n = 0 Then Exit Sub
    Hanoi n - 1, f, v, t, moves
    moves = moves + 1
    Hanoi n - 1, v, t, f, moves
End Sub

Function ToBase(ByVal n As Long, ByVal b As Long) As String
    Const digits As String = "0123456789ABCDEF"
    If n = 0 Then ToBase = "0": Exit Function
    Do While n > 0
        ToBase = Mid$(digits, n Mod b + 1, 1) & ToBase
        n = n \ b
    Loop
End Function

Function Life(g() As Boolean, ByVal n As Long) As Long
    Dim ng() As Boolean, i As Long, j As Long, di As Long, dj As Long, c As Long, alive As Long
    ReDim ng(1 To n, 1 To n)
    For i = 1 To n
        For j = 1 To n
            c = 0
            For di = -1 To 1
                For dj = -1 To 1
                    If di <> 0 Or dj <> 0 Then
                        If i + di >= 1 And i + di <= n And j + dj >= 1 And j + dj <= n Then
                            If g(i + di, j + dj) Then c = c + 1
                        End If
                    End If
                Next dj
            Next di
            ng(i, j) = (c = 3) Or (g(i, j) And c = 2)
            If ng(i, j) Then alive = alive + 1
        Next j
    Next i
    g = ng
    Life = alive
End Function

Sub Main()
    Place 1
    Print "queens", solutions
    Dim m As Long
    Hanoi 10, "A", "C", "B", m
    Print "hanoi", m
    Print ToBase(255, 2), ToBase(255, 16), ToBase(0, 8), ToBase(1000, 7)
    Dim g() As Boolean
    ReDim g(1 To 5, 1 To 5)
    g(2, 3) = True: g(3, 3) = True: g(4, 3) = True
    Dim gen As Long
    For gen = 1 To 3
        Print "gen" & gen, Life(g, 5), g(3, 2), g(2, 3)
    Next
End Sub
