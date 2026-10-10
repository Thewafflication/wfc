Option Explicit
Sub Main()
    Dim d As Object, p As P, k As Variant, it As Variant
    Set d = CreateObject("Scripting.Dictionary")
    Set p = New P: p.Name = "ann": d.Add "a", p
    Set p = New P: p.Name = "bob": Set d("b") = p
    Print d("a").Name, d.Item("b").Greet(), d.Count
    For Each k In d.Keys: Print k & "=" & d(k).Name;: Next: Print
    For Each it In d.Items: Print it.Greet;: Next: Print
    d("a").Name = "ANN"
    Print d("a").Greet
    Dim arr As Variant: arr = d.Items
    Print arr(0).Name, UBound(arr), TypeName(arr(1))
    d.Remove "a"
    Print d.Exists("a"), d.Count
    Dim c As New Collection
    c.Add d
    Print c(1).Count, c(1)("b").Name
End Sub
