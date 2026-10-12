VERSION 5.00
Begin VB.Form Child1
   Caption         =   "Child"
   MDIChild        =   -1  'True
End
Attribute VB_Name = "Child1"
Option Explicit
Private Sub Form_Load()
    Debug.Print "child load"
End Sub
Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)
    Debug.Print "child query"; UnloadMode
End Sub
