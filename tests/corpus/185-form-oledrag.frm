VERSION 5.00
Begin VB.Form Form1
   Caption         =   "OLE drag"
   Begin VB.TextBox Src
      Height          =   300
      OLEDragMode     =   1  'Automatic
      Text            =   "payload"
      Width           =   1500
   End
   Begin VB.PictureBox Dst
      Height          =   900
      Left            =   2000
      OLEDropMode     =   1  'Manual
      Width           =   900
   End
   Begin VB.PictureBox Other
      Height          =   900
      Left            =   3000
      Width           =   900
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Src_OLEStartDrag(Data As DataObject, AllowedEffects As Long)
    Data.SetData Src.Text, vbCFText
    Debug.Print "start"; AllowedEffects
End Sub
Private Sub Dst_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
    Debug.Print "over"; Data.GetFormat(vbCFText); Effect; State
    Effect = vbDropEffectCopy
End Sub
Private Sub Dst_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
    Debug.Print "drop "; Data.GetData(vbCFText); Effect; X; Y
End Sub
Private Sub Src_OLECompleteDrag(Effect As Long)
    Debug.Print "complete"; Effect
End Sub
