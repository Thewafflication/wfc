Option Explicit
Public Const Greeting As String = "hi"
Sub Main()
    Dim r As New Rect, s As IShape, o As New Owner
    r.W = 3: r.H = 4
    o.Name = "bob"
    Set s = r
    Set s.Owner = o
    Print s.Area, s.Describe, s.Describe(True)
    Print Rect.Tag, r.Tag, Rect.Meters, r.Counter, r.Counter
    Dim r2 As New Rect
    Print r2.Counter
    Dim u As Rect.Unit: u = Feet
    Print u, TypeName(s), Greeting
    Dim arr(1 To 3) As Rect, i As Long
    For i = 1 To 3
        Set arr(i) = New Rect
        arr(i).W = i: arr(i).H = i
    Next
    Dim tot As Double
    For i = 1 To 3: Set s = arr(i): tot = tot + s.Area: Next
    Print tot
End Sub
