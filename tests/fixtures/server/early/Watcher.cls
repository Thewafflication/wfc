Option Explicit
Private WithEvents g As Server2.Greeter
Public Log As String
Private Sub g_Changed(ByVal Message As String)
    Log = Log & Message & ";"
End Sub
Public Sub Run()
    Set g = New Server2.Greeter
    g.Prefix = "Early"
    Debug.Print g.Greet("peer"); g.Count
    Debug.Print Log
End Sub
