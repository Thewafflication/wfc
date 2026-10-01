Sub Main()
    Dim b As New Bag, v As Variant, [my total] As Long
    b.Add 10: b.Add 20: b.Add 30
    Print b(2), b.Item(1), b.Count
    For Each v In b
        [my total] = [my total] + v
        Print v
    Next
    Print [my total]
    Dim u As IUnknown
    Set u = b
    Print TypeName(u)
End Sub
