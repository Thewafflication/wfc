Sub Main()
    Dim g As Object
    Set g = CreateObject("Server2.Greeter")
    g.Prefix = "Remote"
    Debug.Print g.Greet("peer"); g.Count
    Debug.Print TypeName(g.Make())
End Sub
