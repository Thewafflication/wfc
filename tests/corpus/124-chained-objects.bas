Option Explicit
Function Pick(a As Node, b As Node, first As Boolean) As Node
    If first Then Set Pick = a Else Set Pick = b
End Function
Sub Swap(ByRef a As Node, ByRef b As Node)
    Dim t As Node
    Set t = a: Set a = b: Set b = t
End Sub
Sub Main()
    Dim r As New Node, v As Variant, x As Node, y As Node
    r.Name = "root"
    r.Add("a").Add("b").Add "c"
    Print r.Kids.Count, r.Kids("a").Kids("b").Kids("c").Name
    Set r.Child = r.Make("m1")
    Set r.Child.Child = New Node
    r.Child.Child.Name = "m2"
    Print r.Depth, r.Child.Child.Name
    With r.Make("w")
        .Rename "w2"
        Print .Name, .Depth
    End With
    Set v = r
    v.Rename "viaVariant"
    Print r.Name, v.Name, v.Child.Name
    Set x = r.Make("X"): Set y = r.Make("Y")
    Swap x, y
    Print x.Name, y.Name, Pick(x, y, False).Name, Pick(x, y, True).Name
    Print r.Make("tmp").Name, Len(r.Make("tmp2").Name)
    Dim arr(1 To 2) As Node
    Set arr(1) = r.Make("e1")
    Print arr(1).Name, arr(2) Is Nothing, TypeName(arr(2))
    On Error Resume Next
    Print arr(2).Name
    Print Err.Number, Err.Description
End Sub
