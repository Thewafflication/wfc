Option Explicit

' ---- CSV parsing with quoted fields ----
Function ParseCsvLine(ByVal line As String) As Collection
    Dim fields As New Collection, cur As String, inQ As Boolean, i As Long, ch As String
    For i = 1 To Len(line)
        ch = Mid$(line, i, 1)
        If inQ Then
            If ch = """" Then
                If Mid$(line, i + 1, 1) = """" Then
                    cur = cur & """": i = i + 1
                Else
                    inQ = False
                End If
            Else
                cur = cur & ch
            End If
        ElseIf ch = """" Then
            inQ = True
        ElseIf ch = "," Then
            fields.Add cur: cur = ""
        Else
            cur = cur & ch
        End If
    Next
    fields.Add cur
    Set ParseCsvLine = fields
End Function

' ---- bigint factorial using digit arrays ----
Function Factorial(ByVal n As Long) As String
    Dim d() As Long, len_ As Long, i As Long, j As Long, carry As Long, prod As Long
    ReDim d(0 To 200)
    d(0) = 1: len_ = 1
    For i = 2 To n
        carry = 0
        For j = 0 To len_ - 1
            prod = d(j) * i + carry
            d(j) = prod Mod 10
            carry = prod \ 10
        Next
        Do While carry > 0
            d(len_) = carry Mod 10
            carry = carry \ 10
            len_ = len_ + 1
        Loop
    Next
    Dim s As String
    For i = len_ - 1 To 0 Step -1: s = s & d(i): Next
    Factorial = s
End Function

' ---- BFS on a grid ----
Function ShortestPath(grid() As String, sr As Long, sc As Long, tr As Long, tc As Long) As Long
    Dim rows As Long, cols As Long, dist() As Long, q As New Collection, cur As Variant
    rows = UBound(grid) + 1: cols = Len(grid(0))
    ReDim dist(0 To rows - 1, 0 To cols - 1)
    Dim r As Long, c As Long
    For r = 0 To rows - 1: For c = 0 To cols - 1: dist(r, c) = -1: Next c, r
    dist(sr, sc) = 0: q.Add Array(sr, sc)
    Dim dr, dc, k As Long, nr As Long, nc As Long
    dr = Array(1, -1, 0, 0): dc = Array(0, 0, 1, -1)
    Do While q.Count > 0
        cur = q(1): q.Remove 1
        If cur(0) = tr And cur(1) = tc Then ShortestPath = dist(tr, tc): Exit Function
        For k = 0 To 3
            nr = cur(0) + dr(k): nc = cur(1) + dc(k)
            If nr >= 0 And nr < rows And nc >= 0 And nc < cols Then
                If Mid$(grid(nr), nc + 1, 1) <> "#" And dist(nr, nc) = -1 Then
                    dist(nr, nc) = dist(cur(0), cur(1)) + 1
                    q.Add Array(nr, nc)
                End If
            End If
        Next
    Loop
    ShortestPath = -1
End Function

Sub Main()
    Dim f As Collection, v As Variant, total As Double
    Set f = ParseCsvLine("apple,""red, ripe"",3.5,""say """"hi""""""")
    For Each v In f: Print "[" & v & "]";: Next: Print
    total = CDbl(f(3)) * 2: Print total
    Print Factorial(25)
    Print Len(Factorial(100)), Left$(Factorial(100), 12)
    Dim g(0 To 4) As String
    g(0) = "S..#...": g(1) = ".#.#.#.": g(2) = ".#...#.": g(3) = ".####.#": g(4) = "......T"
    Print ShortestPath(g, 0, 0, 4, 6)
    g(3) = ".######"
    Print ShortestPath(g, 0, 0, 4, 6)
End Sub
