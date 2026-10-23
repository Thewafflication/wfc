Dim a As String, b As String
Sub Foo(ByVal n As Long)
    If n > 2 Then a = "": b = "big": Exit Sub
    Print "after "; n; a; b
End Sub
Sub Main()
    Foo 1
    Foo 5
    If 1 > 2 Then a = "x": Print "no"
    If 2 > 1 Then a = "y": Print "yes" Else a = "z": Print "else"
    If 1 > 2 Then a = "p": Print "p1" Else a = "q": Print "q1": Print "q2"
    Print "done "; a
End Sub
