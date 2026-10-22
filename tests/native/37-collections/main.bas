Option Explicit

Function Make(ByVal n As String, ByVal q As Long) As Item
    Dim it As New Item
    it.Name = n
    it.Qty = q
    Set Make = it
End Function

Sub Main()
    Dim c As New Collection
    Dim it As Item, other As Item
    Dim v As Variant, total As Long
    c.Add Make("apple", 3), "a"
    c.Add Make("pear", 5), "p"
    c.Add Make("fig", 7)
    Print c.Count
    Set it = c("p")
    Print it.Label()
    Set other = c.Item(1)
    Print other.Name; it Is c("p"); it Is other
    For Each it In c
        total = total + it.Qty
        Print it.Name;
    Next it
    Print
    Print total
    Set it = Nothing
    Set other = Nothing
    c.Remove "a"
    Print "removed"
    c.Remove 1
    Print "removed again"
    Print c.Count
    Set c = Nothing
    Print "end"
End Sub
