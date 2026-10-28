Private Type Rec
    Name As String * 6
    V As Variant
    When As Date
End Type
Private mCount As Long
Public Property Get Count() As Long
    Count = mCount
End Property
Public Property Let Count(ByVal v As Long)
    If v < 0 Then Exit Property
    mCount = v
End Property
Sub BubbleSort(a() As Variant)
    Dim i As Long, j As Long, t As Variant
    For i = LBound(a) To UBound(a) - 1
        For j = i + 1 To UBound(a)
            If a(j) < a(i) Then t = a(i): a(i) = a(j): a(j) = t
        Next
    Next
End Sub
Sub Main()
    Dim r As Rec, q As Rec
    r.Name = "ab"
    r.V = 12
    r.When = #1/5/2020#
    q = r
    r.V = "changed"
    Print "[" & q.Name & "]", q.V, r.V, Format(q.When, "yyyy-mm-dd")
    Dim recs(1 To 2) As Rec
    recs(2).V = "second"
    Print recs(2).V, IsEmpty(recs(1).V)
    Count = 5
    Count = -1
    Print Count
    Dim data As Variant
    data = Array(5, 3, "b", 1, "a", 2.5)
    BubbleSort data
    Dim v As Variant
    For Each v In data: Print v;: Next
    Print
    Dim b() As Byte, s As String
    b = "AB"
    Print UBound(b), b(0), b(1), b(2), b(3)
    s = b
    Print s, Len(s)
    Print Left(12345, 2), IsNull(Abs(Null))
End Sub
