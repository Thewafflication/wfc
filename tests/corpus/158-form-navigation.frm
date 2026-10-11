VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Nav"
   Begin VB.TextBox Text1
      Text            =   "a"
   End
   Begin VB.TextBox Text2
      TabIndex        =   2
   End
   Begin VB.CheckBox Check1
      Caption         =   "&Flag"
      TabIndex        =   1
   End
   Begin VB.CommandButton cmdOK
      Caption         =   "OK"
      Default         =   -1  'True
      TabIndex        =   3
   End
   Begin VB.CommandButton cmdCancel
      Cancel          =   -1  'True
      Caption         =   "&Cancel"
      TabIndex        =   4
   End
   Begin VB.TextBox Text3
      CausesValidation=   0   'False
      TabIndex        =   5
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Print "start"
End Sub
Private Sub Text1_GotFocus(): Print "got1": End Sub
Private Sub Text1_LostFocus(): Print "lost1": End Sub
Private Sub Text1_Validate(Cancel As Boolean)
    Print "validate1"; Text1.Text
    If Text1.Text = "bad" Then Cancel = True
    Print "cancel now"; Cancel
End Sub
Private Sub Check1_GotFocus(): Print "got check": End Sub
Private Sub Text2_GotFocus(): Print "got2": End Sub
Private Sub cmdOK_Click(): Print "OK clicked": End Sub
Private Sub cmdCancel_Click(): Print "cancel clicked": End Sub
