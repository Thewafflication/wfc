Option Explicit
Dim g As New Foo
Sub Main()
    Dim a As New Foo
    Print "before"
    a.Hello
    a.Hello
    Set a = Nothing
    Print "cleared"
    Print a Is Nothing
    a.Hello
    Dim b As New Foo
    Set b = Nothing
    Print "end"
    g.Hello
End Sub
