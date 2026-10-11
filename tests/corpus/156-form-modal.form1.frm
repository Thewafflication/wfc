VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Main"
   Begin VB.CommandButton cmdDlg
      Caption         =   "Dialog"
   End
End
Attribute VB_Name = "Form1"
Private Sub cmdDlg_Click()
    Print "before modal"
    Form2.Show vbModal
    Print "after modal"; Form2.Tag
    Form2.Show vbModal
    Print "second modal returned"
End Sub
