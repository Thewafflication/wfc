VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Draw"
   ClientHeight    =   3000
   ClientWidth     =   4000
   ForeColor       =   255
   Begin VB.PictureBox Picture1
      Height          =   1000
      Left            =   100
      ScaleHeight     =   500
      ScaleWidth      =   500
      Top             =   100
      Width           =   1000
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Debug.Print "loaded"
End Sub
Private Sub Form_Activate()
    Print ScaleWidth; ScaleHeight; ScaleMode
    Scale (0, 0)-(100, 50)
    Print ScaleWidth; ScaleHeight; ScaleMode
    PSet (10, 10)
    PSet Step(5, 5), vbBlue
    Line (0, 0)-(100, 50), vbGreen
    Line (10, 10)-(20, 20), , B
    Line (10, 10)-Step(5, 5), vbRed, BF
    Circle (50, 25), 10
    Circle (50, 25), 10, vbBlue, 0, 3.14159
    CurrentX = 5: CurrentY = 5
    Print "hi"; 
    Print "there"
    Print CurrentX; CurrentY
    Picture1.Line (0, 0)-(500, 500)
    Picture1.Circle (250, 250), 100
    Picture1.Print "pic"
    ScaleMode = vbPixels
    Print ScaleWidth; TextWidth("abc") > 0; TextHeight("x") > 0
    Cls
End Sub
