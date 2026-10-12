VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Fill"
   ScaleMode       =   3  'Pixel
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Activate()
    FillStyle = vbFSSolid
    FillColor = vbRed
    Circle (50, 50), 20, vbBlack
    Line (100, 100)-(140, 140), vbBlack, B
    FillStyle = vbFSTransparent
    Line (200, 100)-(240, 140), vbBlack, B
    Print Point(50, 50); Point(120, 120); Point(220, 120)
End Sub
