Option Explicit
Sub Main()
    Dim d As Object, k As Variant
    Set d = CreateObject("Scripting.Dictionary")
    d.Add "one", 1
    d.Add "two", 2
    d("three") = 3
    d.Item("two") = 22
    Print d.Count, d("one"), d.Item("two"), d.Exists("three"), d.Exists("THREE")
    For Each k In d
        Print k & "=" & d(k);
    Next
    Print
    Dim ks As Variant, i As Long
    ks = d.Keys
    For i = LBound(ks) To UBound(ks): Print ks(i);: Next: Print
    d.Remove "one"
    Print d.Count, TypeName(d), d.Exists("one")
    Print d("missing") = "", d.Count
    d.RemoveAll
    d.CompareMode = 1
    d.Add "Two", 5
    On Error Resume Next
    d.Add "TWO", 6
    Print Err.Number, d.Count
End Sub
