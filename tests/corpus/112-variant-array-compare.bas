Option Explicit
Sub BubbleSort(a() As Variant)
    Dim i As Long, j As Long, t As Variant
    For i = LBound(a) To UBound(a) - 1
        For j = i + 1 To UBound(a)
            If a(j) < a(i) Then t = a(i): a(i) = a(j): a(j) = t
        Next
    Next
End Sub
Sub Main()
    Dim data As Variant, v As Variant, s(1 To 2) As Variant
    data = Array(5, 3, "b", 1, "a", 2.5)
    BubbleSort data
    For Each v In data: Print v;: Next
    Print
    s(1) = 3: s(2) = "3"
    Print s(1) < s(2), s(1) = s(2), s(2) > s(1)
End Sub
