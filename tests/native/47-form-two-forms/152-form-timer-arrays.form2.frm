VERSION 5.00
Begin VB.Form Form2
   Caption         =   "Second"
   Begin VB.CommandButton cmdClose
      Caption         =   "Close"
   End
End
Attribute VB_Name = "Form2"
Private Sub Form_Load()
    Print "form2 load"
End Sub
Private Sub cmdClose_Click()
    Me.Hide
    Print "form2 hidden"
End Sub
Private Sub Form_Unload(Cancel As Integer)
    Print "unload form2"
End Sub
