Option Explicit
Sub Main()
    Dim g As Object, c As Object
    Set g = CreateObject("Server1.Greeter")
    g.Prefix = "Hello"
    Debug.Print g.Greet("world"); g.Count
    Set c = g.Make()
    Debug.Print c.Greet("kid"); c.Count; g.Count
    On Error Resume Next
    Debug.Print g.Fail()
    Debug.Print Err.Number; Err.Description
End Sub
