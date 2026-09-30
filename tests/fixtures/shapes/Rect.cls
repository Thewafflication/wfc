Attribute VB_Name = "Rect"
Option Explicit
Implements Shape
Public W As Double
Public H As Double
Private Function Shape_Area() As Double
    Shape_Area = W * H
End Function
Private Function Shape_Describe() As String
    Shape_Describe = "Rect " & W & "x" & H
End Function
Private Sub Class_Terminate()
    Print "bye rect"
End Sub
