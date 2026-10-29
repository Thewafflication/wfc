VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Load"
   ClientHeight    =   2000
   ClientWidth     =   3000
   Begin VB.CommandButton Command1
      Caption         =   "b"
      Height          =   300
      Index           =   0
      Left            =   120
      Top             =   120
      Width           =   900
   End
End
Attribute VB_Name = "Form1"
Option Explicit

Private Sub Form_Load()
    Generate Command1, 3
    On Error Resume Next
    Unload Command1(0)
    Caption = "e" & Err.Number
End Sub

Private Sub Generate(arr As Variant, ByVal count As Integer)
    Dim i As Integer
    For i = 1 To count
        Load arr(i)
    Next i
End Sub
