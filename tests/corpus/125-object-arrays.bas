Option Explicit
Public Sub Show(arr() As Item)
    Dim i As Long
    For i = LBound(arr) To UBound(arr): Print arr(i).Name & ":" & arr(i).Qty;: Next
    Print
End Sub
Sub Main()
    Dim inv As New Inv, it As Item, a() As Item, v As Variant
    inv.Add "b", 5: inv.Add "a", 2: inv.Add "c", 9
    inv.SortByQty
    Dim i As Long
    For i = 1 To inv.Count: Print inv.Item(i).Name;: Next: Print
    Show inv.Items
    a = inv.Items
    Print UBound(a), a(2).Name, inv.Item(3).Total(1.5)
    For Each v In inv.Items: Print v.Name;: Next: Print
    For Each it In inv.Items: Print it.Qty;: Next: Print
    inv.Fill
    Print inv.Fixed(1).Name, inv.Fixed(2).Name, UBound(inv.Fixed)
    inv.Fixed(2).Qty = 77
    Print inv.Fixed(2).Qty
End Sub
