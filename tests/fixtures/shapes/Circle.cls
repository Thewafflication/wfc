Attribute VB_Name = "Circle"
Option Explicit
Implements Shape
Private r As Double
Public Property Let Radius(v As Double)
    r = v
End Property
Public Property Get Radius() As Double
    Radius = r
End Property
Private Function Shape_Area() As Double
    Shape_Area = 3.14159265 * r * r
End Function
Private Function Shape_Describe() As String
    Shape_Describe = "Circle r=" & r
End Function
