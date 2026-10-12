VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Drag"
   Begin VB.Label Item
      Caption         =   "Item"
      DragMode        =   1  'Automatic
   End
   Begin VB.PictureBox Bin
      Height          =   500
      Width           =   500
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Bin_DragOver(Source As Control, X As Single, Y As Single, State As Integer)
    Debug.Print "over "; Source.Name; State
End Sub
Private Sub Bin_DragDrop(Source As Control, X As Single, Y As Single)
    Debug.Print "drop "; Source.Caption; X; Y
End Sub
Private Sub Form_DragDrop(Source As Control, X As Single, Y As Single)
    Debug.Print "form drop "; Source.Name; X; Y
End Sub
Private Sub Form_Load()
    Item.Drag vbBeginDrag
    Item.Drag vbEndDrag
End Sub
