Option Explicit
Private Type Person
    Name As String
    Age As Long
End Type
Sub SortByAge(p() As Person)
    Dim i As Long, j As Long, t As Person
    For i = LBound(p) To UBound(p) - 1
        For j = i + 1 To UBound(p)
            If p(j).Age < p(i).Age Then
                t = p(i): p(i) = p(j): p(j) = t
            End If
        Next j
    Next i
End Sub
Sub Main()
    Dim ppl(1 To 3) As Person
    ppl(1).Name = "Cy": ppl(1).Age = 41
    ppl(2).Name = "Al": ppl(2).Age = 29
    ppl(3).Name = "Bo": ppl(3).Age = 35
    SortByAge ppl
    Dim i As Long
    For i = 1 To 3
        Print ppl(i).Name & ":" & ppl(i).Age
    Next i
End Sub
