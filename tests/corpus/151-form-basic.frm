VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Demo"
   ClientHeight    =   3000
   ClientWidth     =   4000
   Begin VB.TextBox Text1
      Height          =   300
      Left            =   120
      Text            =   "start"
      Top             =   120
      Width           =   2000
   End
   Begin VB.CommandButton Command1
      Caption         =   "Go"
      Height          =   400
      Left            =   120
      Top             =   600
      Width           =   1200
   End
   Begin VB.Label Label1
      Caption         =   "idle"
      Height          =   200
      Left            =   120
      Top             =   1200
      Width           =   1200
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Dim clicks As Long
Private Sub Form_Load()
    Print "load"; Caption; Text1.Text
    Label1.Caption = "loaded"
End Sub
Private Sub Form_Activate()
    Print "activate"
End Sub
Private Sub Command1_Click()
    clicks = clicks + 1
    Label1.Caption = "clicked " & clicks
    Print "click"; Label1.Caption
End Sub
Private Sub Text1_Change()
    Print "change"; Text1.Text
End Sub
Private Sub Text1_GotFocus()
    Print "gotfocus text1"
End Sub
Private Sub Label1_Change()
    Print "label now "; Label1.Caption
End Sub
Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)
    Print "queryunload"; UnloadMode
End Sub
Private Sub Form_Unload(Cancel As Integer)
    Print "unload"
End Sub
