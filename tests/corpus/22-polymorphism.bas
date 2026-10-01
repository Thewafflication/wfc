Option Explicit
Sub Main()
    Dim shapes(1 To 3) As IShape
    Set shapes(1) = New Square
    Set shapes(2) = New Circle2
    Set shapes(3) = New Square
    shapes(1).SetSize 2
    shapes(2).SetSize 1
    shapes(3).SetSize 3
    Dim i As Long, total As Double
    For i = 1 To 3
        Print shapes(i).Name & ": " & Format(shapes(i).Area, "0.00")
        total = total + shapes(i).Area
    Next i
    Print "total " & Format(total, "0.00")
    Dim s As IShape
    Set s = shapes(2)
    Print TypeName(s) & " " & (TypeOf s Is Circle2) & " " & (TypeOf s Is Square)
End Sub
