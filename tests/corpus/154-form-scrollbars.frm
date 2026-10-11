VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Scroll"
   Begin VB.HScrollBar HScroll1
      Height          =   255
      LargeChange     =   10
      Max             =   100
      Min             =   10
      Top             =   120
      Value           =   20
      Width           =   2000
   End
   Begin VB.VScrollBar VScroll1
      Height          =   1000
      Left            =   2500
      Top             =   120
      Width           =   255
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Print "start"; HScroll1.Value; HScroll1.Min; HScroll1.Max; VScroll1.Max
    On Error Resume Next
    HScroll1.Value = 500
    Print Err.Number; Err.Description
    Err.Clear
    HScroll1.Value = 50
End Sub
Private Sub HScroll1_Change()
    Print "change"; HScroll1.Value
End Sub
Private Sub HScroll1_Scroll()
    Print "scroll"; HScroll1.Value
End Sub
Private Sub VScroll1_Change()
    Print "vchange"; VScroll1.Value
End Sub
