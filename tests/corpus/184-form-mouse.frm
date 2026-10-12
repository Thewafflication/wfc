VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Mouse"
   ScaleMode       =   3  'Pixel
   Begin VB.PictureBox Pic1
      Height          =   1500
      Left            =   1500
      ScaleMode       =   3  'Pixel
      Top             =   0
      Width           =   1500
   End
   Begin VB.Label Item
      Caption         =   "Item"
      DragMode        =   1  'Automatic
      Height          =   300
      Width           =   900
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Pic1_MouseDown(Button As Integer, Shift As Integer, X As Single, Y As Single)
    Debug.Print "pic down"; Button; Shift; X; Y
End Sub
Private Sub Pic1_MouseMove(Button As Integer, Shift As Integer, X As Single, Y As Single)
    Debug.Print "pic move"; X; Y
End Sub
Private Sub Pic1_MouseUp(Button As Integer, Shift As Integer, X As Single, Y As Single)
    Debug.Print "pic up"; Button
End Sub
Private Sub Form_MouseDown(Button As Integer, Shift As Integer, X As Single, Y As Single)
    Debug.Print "form down"; X; Y
End Sub
Private Sub Item_MouseDown(Button As Integer, Shift As Integer, X As Single, Y As Single)
    Debug.Print "item down"
End Sub
Private Sub Pic1_DragDrop(Source As Control, X As Single, Y As Single)
    Debug.Print "dropped "; Source.Caption; X; Y
End Sub
