Option Explicit
Type Point
    x As Long
    y As Long
    label As String
End Type
Type Shape
    corner As Point
    sides(2) As Long
End Type

Function MakeBox() As Box
    Dim b As New Box
    b.Width = 2
    b.Height = 3
    Set MakeBox = b
End Function

Sub Main()
    Dim b As New Box, p As Point, s As Shape
    With b
        .Width = 4
        .Height = 5
        .Title = "main"
        .Grow 1
        Print .Width; .Height; .Area(); .Title
        Set .Inner = New Box
        With .Inner
            .Width = 7
            .Height = .Width + 1
            Print .Area()
        End With
        Print .Inner.Width
    End With
    With p
        .x = 3
        .y = .x * 2
        .label = "pt" & .y
    End With
    Print p.x; p.y; p.label
    With s
        .corner.x = 9
        .sides(2) = 4
        Print .corner.x; .sides(2)
        With .corner
            .y = 11
            Print .x + .y
        End With
    End With
    Print s.corner.y
    With MakeBox()
        Print .Area()
        .Grow 2
        Print .Width
    End With
End Sub
