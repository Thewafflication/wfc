VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Light"
   ClientHeight    =   3000
   ClientWidth     =   4000
   Begin VB.Shape Shape1
      FillColor       =   255
      FillStyle       =   0
      Height          =   500
      Left            =   100
      Shape           =   2
      Top             =   100
      Width           =   800
   End
   Begin VB.Line Line1
      X1              =   0
      X2              =   1000
      Y1              =   1000
      Y2              =   1500
   End
   Begin VB.Image Image1
      Height          =   400
      Left            =   2000
      Stretch         =   -1  'True
      Top             =   200
      Width           =   600
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Activate()
    Image1.Picture = LoadPicture(App.Path & "\163-form-lightweight.red.bmp")
    Shape1.Left = 300
    Line1.Visible = False
    Print "done"
End Sub
