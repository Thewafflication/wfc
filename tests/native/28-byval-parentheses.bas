' A parenthesized argument is a value: the callee cannot change the variable.
Option Explicit
Type P
    X As Long
End Type
Sub Inc(ByRef n As Long)
    n = n + 1
End Sub
Sub Shout(ByRef s As String)
    s = UCase(s)
End Sub
Sub Main()
    Dim x As Long, a(2) As Long, t As P, s As String
    x = 1
    Inc x
    Inc (x)
    Call Inc(x)
    Inc x + 0
    Print x
    Inc a(1)
    Inc (a(1))
    Inc t.X
    Inc (t.X)
    Print a(1), t.X
    s = "abc"
    Shout (s)
    Print s
    Shout s
    Print s
End Sub
