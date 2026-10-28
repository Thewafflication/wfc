Sub Main()
    Dim jag(2) As Variant, i As Long, j As Long
    Dim row() As Long
    For i = 0 To 2
        ReDim row(i)
        For j = 0 To i: row(j) = i * 10 + j: Next
        jag(i) = row
    Next
    Print jag(2)(1), UBound(jag(2)), jag(0)(0), jag(1)(1)
    jag(2)(0) = -1
    Print jag(2)(0), row(0)
    Dim v As Variant
    v = Array(Array(1, 2), Array(3, 4))
    v(1)(0) = 30
    Print v(1)(0), v(0)(1), UBound(v(1))
    Print Array(5, 6, 7)(2), Split("x y z")(2)
    Debug.Assert False
    Dim l As Long, s As String
    l = "12": Print l + 1
    s = 5: Print s & "!"
    Print "5" Or 2, Not "5", "7" And "3"
    Dim c As New Collection
    c.Add "one": c.Add "two"
    Dim o As Object
    Set o = c
    Print o(2), o.Count
    Print "done"
End Sub
