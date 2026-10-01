Sub Main()
    Dim k As New Kw
    k.Name = "n": k.Date = #1/2/2000#: k.Left = 3: k.Right = 4
    Set k.NextNode = New Kw: k.NextNode.Name = "child"
    Print k.Name, k.Date, k.Left + k.Right, k.NextNode.Name, k.Count
    k.Print "hi"
    Print k.Format(5), k.Len(), k.Input()
    Dim s As String: s = "abc"
    Print Len(s), Left(s, 1), Format(5, "00")
    With k
        .Name = "w"
        Print .Name, .Len, .Count
    End With
End Sub
