VERSION 5.00
Begin VB.MDIForm Main
   Caption         =   "MDI Parent"
   ClientHeight    =   4000
   ClientWidth     =   6000
   Begin VB.Timer Timer1
      Interval        =   300
   End
End
Attribute VB_Name = "Main"
Option Explicit
Dim n As Long
Private Sub MDIForm_Load()
    Child1.Show
End Sub
Private Sub Timer1_Timer()
    n = n + 1
    If n = 1 Then
        Debug.Print "active "; ActiveForm.Caption
        Arrange vbCascade
    End If
    If n = 3 Then
        Debug.Print "closing mdi"
        Unload Me
    End If
End Sub
