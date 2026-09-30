Attribute VB_Name = "Main"
Option Explicit

Sub Main()
    Dim shapes As New Collection
    Dim c As New Circle
    c.Radius = 2
    Dim r As New Rect
    r.W = 3: r.H = 4
    shapes.Add c
    shapes.Add r
    Dim s As Shape
    Dim total As Double
    Dim i As Long
    For i = 1 To shapes.Count
        Set s = shapes.Item(i)
        Print s.Describe() & " area=" & Format(s.Area(), "0.00")
        total = total + s.Area()
    Next i
    Print "total=" & Format(total, "0.000")
    Dim o As Object
    For Each o In shapes
        If TypeOf o Is Circle Then Print "circle!"
    Next
End Sub
