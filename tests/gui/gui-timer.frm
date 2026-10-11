VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Real window"
   ClientHeight    =   2000
   ClientWidth     =   3000
   Begin VB.Timer Timer1
      Interval        =   200
   End
   Begin VB.CommandButton Command1
      Caption         =   "Go"
      Height          =   400
      Left            =   120
      Top             =   120
      Width           =   1200
   End
   Begin VB.TextBox Text1
      Height          =   300
      Left            =   120
      Top             =   700
      Width           =   2000
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Dim n As Long
Private Sub Form_Load()
    Print "load"; Caption; Command1.Caption
End Sub
Private Sub Form_Activate()
    Print "activate"
End Sub
Private Sub Timer1_Timer()
    n = n + 1
    Print "tick"; n
    If n = 1 Then Text1.Text = "set from code"
    If n = 3 Then Unload Me
End Sub
Private Sub Text1_Change()
    Print "change "; Text1.Text
End Sub
Private Sub Form_Unload(Cancel As Integer)
    Print "unload"
End Sub
