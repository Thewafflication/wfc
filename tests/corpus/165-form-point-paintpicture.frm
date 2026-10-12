VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Pixels"
   ClientHeight    =   3000
   ClientWidth     =   4000
   ScaleMode       =   3  'Pixel
   Begin VB.PictureBox Picture1
      Height          =   1500
      Left            =   2000
      ScaleMode       =   3  'Pixel
      Top             =   100
      Width           =   1500
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Activate()
    Print Point(5, 5); Point(500, 500)
    PSet (5, 5), vbRed
    Print Point(5, 5)
    Line (10, 10)-(30, 30), vbBlue, BF
    Print Point(20, 20); Point(31, 31)
    Circle (100, 100), 20, vbGreen
    Print Point(120, 100); Point(100, 100)
    Cls
    Print Point(20, 20)
    Picture1.PSet (3, 3), vbBlue
    Print Picture1.Point(3, 3); Picture1.Point(4, 4)
    Dim pic As StdPicture
    Set pic = LoadPicture(App.Path & "\165-form-point-paintpicture.red.bmp")
    Picture1.PaintPicture pic, 0, 0
    Picture1.PaintPicture pic, 10, 10, 50, 25, 0, 0, 16, 8
    PaintPicture pic, 100, 100
End Sub
