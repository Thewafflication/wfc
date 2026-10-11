VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Main"
   ClientHeight    =   3000
   ClientWidth     =   4000
   KeyPreview      =   -1  'True
   Begin VB.Timer Timer1
      Interval        =   100
   End
   Begin VB.CheckBox Check1
      Caption         =   "Flag"
      Height          =   300
      Left            =   120
      Top             =   120
      Width           =   1200
   End
   Begin VB.OptionButton Option1
      Caption         =   "A"
      Index           =   0
      Left            =   120
      Top             =   500
      Value           =   -1  'True
   End
   Begin VB.OptionButton Option1
      Caption         =   "B"
      Index           =   1
      Left            =   120
      Top             =   800
   End
   Begin VB.CommandButton cmdOpen
      Caption         =   "Open"
      Left            =   120
      Top             =   1200
   End
   Begin VB.CommandButton cmdQuit
      Caption         =   "Quit"
      Left            =   1500
      Top             =   1200
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Dim ticks As Long
Private Sub Form_Load()
    Print "Option1 count"; Option1.Count; Option1(1).Caption
End Sub
Private Sub Timer1_Timer()
    ticks = ticks + 1
    Print "tick"; ticks
    If ticks = 3 Then Timer1.Enabled = False
End Sub
Private Sub Check1_Click()
    Print "check"; Check1.Value
End Sub
Private Sub Option1_Click(Index As Integer)
    Print "option"; Index; Option1(0).Value; Option1(1).Value
End Sub
Private Sub cmdOpen_Click()
    Form2.Show
    Print "forms"; Forms.Count
End Sub
Private Sub cmdQuit_Click()
    Unload Me
End Sub
Private Sub Form_KeyDown(KeyCode As Integer, Shift As Integer)
    Print "preview keydown"; KeyCode
End Sub
Private Sub Form_Unload(Cancel As Integer)
    Print "unload form1"
End Sub
