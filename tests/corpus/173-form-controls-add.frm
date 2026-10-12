VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Add"
   Begin VB.Frame Frame1
      Height          =   1000
      Width           =   2000
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Dim WithEvents Btn As CommandButton
Private Sub Form_Load()
    Dim c As Control
    Debug.Print Controls.Count
    Set c = Controls.Add("VB.CommandButton", "Cmd1", Frame1)
    Set Btn = c
    Btn.Caption = "Dyn"
    Btn.Visible = True
    Debug.Print Controls.Count; TypeName(c); Btn.Caption; Btn.Container.Name; Btn.Visible
    Controls.Add "VB.TextBox", "Text9"
    Debug.Print Controls("Text9").Visible
    On Error Resume Next
    Controls.Add "VB.CommandButton", "Cmd1"
    Debug.Print Err.Number
    Err.Clear
    Controls.Add "No.Such", "X"
    Debug.Print Err.Number
    Err.Clear
    Controls.Remove "Text9"
    Debug.Print Controls.Count
    Controls.Remove "Frame1"
    Debug.Print Err.Number
End Sub
Private Sub Btn_Click()
    Debug.Print "clicked"
End Sub
