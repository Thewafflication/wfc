VERSION 5.00
Begin VB.Form Form2
   Caption         =   "Dialog"
   Begin VB.CommandButton cmdOK
      Caption         =   "OK"
   End
End
Attribute VB_Name = "Form2"
Private Sub cmdOK_Click()
    Tag = "ok"
    Me.Hide
End Sub
