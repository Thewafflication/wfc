VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Form1"
   ClientHeight    =   3000
   ClientWidth     =   4000
   Begin VB.TextBox Text1
      Height          =   285
      Left            =   120
      Text            =   "hello"
      Top             =   120
      Width           =   1500
   End
End
Attribute VB_Name = "Form1"
Private Function Id(ByVal v As Variant) As String
    Id = "ok"
End Function
Private Sub Form_Load()
    Dim s As String
    On Error Resume Next
    Print Id(Text1.Text); Err.Number
    Err.Clear
    Print Id(5); Err.Number
    s = Id(Text1.Text)
    Print s; Err.Number
    Debug.Print Id(Text1.Text); Err.Number
End Sub
