VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Keys"
   Begin VB.TextBox Text1
      TabIndex        =   0
   End
   Begin VB.TextBox Text2
      TabIndex        =   1
   End
   Begin VB.CommandButton cmdOK
      Caption         =   "OK"
      Default         =   -1  'True
      TabIndex        =   2
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Activate()
    SendKeys "abc{TAB}xy{BS}z~"
End Sub
Private Sub Text1_Change(): Print "t1="; Text1.Text: End Sub
Private Sub Text2_Change(): Print "t2="; Text2.Text: End Sub
Private Sub Text2_KeyDown(KeyCode As Integer, Shift As Integer)
    If KeyCode = vbKeyA Then Print "keydown A shift="; Shift
End Sub
Private Sub cmdOK_Click(): Print "ok clicked": End Sub
