Sub Main()
    Dim h As Holder
    Set h = New Holder
    ' a field holding an object, then a method with and without Call
    h.Items.Add "one"
    Call h.Items.Add("two", "k2")
    h.Items.Add "three", "k3"
    ' a property holding an object
    h.More.Add "x"
    Call h.More.Add("y")
    Print h.Items.Count; h.More.Count; h.Items("k2")
End Sub
