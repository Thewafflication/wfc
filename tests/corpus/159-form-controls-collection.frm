VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Controls"
   Begin VB.TextBox Text1
      Text            =   "t"
   End
   Begin VB.CommandButton Command1
      Caption         =   "Go"
      Index           =   0
      Left            =   120
      Top             =   120
   End
   Begin VB.Label Label1
      Caption         =   "lbl"
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Dim c As Control, n As Long
    For Each c In Controls
        n = n + 1
        Print n; c.Name; TypeName(c); TypeOf c Is TextBox; c.Parent.Name
    Next
    Print Controls.Count
    Print Controls("Text1").Name
    Print Controls("Text1").Text
    Print Controls(0).Name
    Load Command1(1)
    Command1(1).Caption = "Copy": Command1(1).Visible = True
    Command1(1).Left = 1500
    Print Command1.Count; Command1(1).Caption; Command1(1).Index; Command1(1).Left
    Print Controls.Count
End Sub
Private Sub Form_Activate()
    Print Screen.ActiveForm.Name; Screen.ActiveControl.Name; ActiveControl.Name
End Sub
Private Sub Command1_Click(Index As Integer)
    Print "click"; Index; Command1(Index).Caption
    If Index = 1 Then Unload Command1(1): Print "unloaded"; Command1.Count
    On Error Resume Next
    Unload Command1(0)
    Print Err.Number; Err.Description
End Sub
