VERSION 5.00
Begin VB.Form Form1
   Caption         =   "End"
   Begin VB.CommandButton Command1
      Caption         =   "Quit"
   End
End
Attribute VB_Name = "Form1"
Private Sub Command1_Click()
    Print "ending"
    End
    Print "not reached"
End Sub
Private Sub Form_Unload(Cancel As Integer)
    Print "unload (should not run after End)"
End Sub
