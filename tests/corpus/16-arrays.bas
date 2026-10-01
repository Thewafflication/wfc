Option Explicit
Option Base 1
Sub Main()
    Dim a(3) As Long
    Print LBound(a) & UBound(a)
    Dim m() As Long
    ReDim m(2)
    m(1) = 10: m(2) = 20
    ReDim Preserve m(4)
    m(4) = 40
    Dim i As Long, s As Long
    For i = LBound(m) To UBound(m)
        s = s + m(i)
    Next i
    Print s & " " & UBound(m)
    Erase m
    Print UBound(a)
    Dim g(1 To 2, 0 To 2) As Integer
    g(2, 0) = 7
    g(1, 2) = 3
    Print g(2, 0) + g(1, 2) & " " & UBound(g, 2) & LBound(g, 2)
    Dim words(1 To 3) As String
    words(1) = "b": words(2) = "c": words(3) = "a"
    Dim j As Long, t As String
    For i = 1 To 2
        For j = i + 1 To 3
            If words(j) < words(i) Then
                t = words(i): words(i) = words(j): words(j) = t
            End If
        Next j
    Next i
    Print Join(words, "")
    Dim v As Variant
    v = Split("a,b,c", ",")
    Print LBound(v) & UBound(v)
End Sub
