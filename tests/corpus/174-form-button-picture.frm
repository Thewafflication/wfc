VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Buttons"
   Begin VB.CommandButton Command1
      Height          =   600
      Style           =   1  'Graphical
      Width           =   600
   End
   Begin VB.CommandButton Command2
      Caption         =   "Plain"
      Height          =   600
      Left            =   800
      Width           =   900
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Activate()
    Command1.Picture = LoadPicture(App.Path & "\174-form-button-picture.red.bmp")
    Command2.Picture = LoadPicture(App.Path & "\174-form-button-picture.red.bmp")
End Sub
