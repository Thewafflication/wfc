VERSION 5.00
Begin VB.Form Child1
   Caption         =   "Child window"
   ClientHeight    =   1800
   ClientWidth     =   3000
   MDIChild        =   -1  'True
   Begin VB.Label Label1
      Caption         =   "Inside the MDI client"
      Height          =   300
      Left            =   120
      Top             =   120
      Width           =   2400
   End
   Begin VB.TextBox Text1
      Height          =   300
      Left            =   120
      Top             =   600
      Width           =   2400
   End
End
Attribute VB_Name = "Child1"
Option Explicit
Private Sub Form_Unload(Cancel As Integer)
    Debug.Print "child unload"
End Sub
