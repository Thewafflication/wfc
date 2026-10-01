Option Explicit
Dim total As Long
Function A() As String: A = "a": End Function
Sub Bump(): total = total + 1: End Sub
Sub Greet(ByVal who As String): Print "hi " & who & ":": End Sub
Property Get Doubled() As Long: Doubled = total * 2: End Property
Sub Empty1(): End Sub
Sub Main()
    Bump: Bump
    Greet "bob"
    Empty1
    Print A(), total, Doubled
End Sub
