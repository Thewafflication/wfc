VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Host"
   Begin Project1.Gauge Gauge1
      Caption         =   "hello"
      Height          =   900
      Left            =   120
      Top             =   240
      Width           =   3000
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Activate()
    Unload Me
End Sub
Private Sub Form_Initialize()
    Debug.Print "form initialize"
End Sub
Private Sub Form_Load()
    Debug.Print Gauge1.Caption; Gauge1.Left; Gauge1.Width; Gauge1.Twice(21)
    Gauge1.Bump
    Gauge1.Caption = "set"
    Gauge1.Width = 3600
    Debug.Print Gauge1.Object.Caption; TypeName(Gauge1)
End Sub
Private Sub Gauge1_Changed(ByVal NewValue As String)
    Debug.Print "changed "; NewValue
End Sub
Private Sub Form_Unload(Cancel As Integer)
    Debug.Print "form unload"
End Sub
