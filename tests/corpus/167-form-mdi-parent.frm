VERSION 5.00
Begin VB.MDIForm Main
   Caption         =   "Main"
End
Attribute VB_Name = "Main"
Option Explicit
Private Sub MDIForm_Load()
    Debug.Print "mdi load"
    Child1.Show
    Debug.Print TypeName(ActiveForm)
    Arrange vbCascade
End Sub
Private Sub MDIForm_Unload(Cancel As Integer)
    Debug.Print "mdi unload"
End Sub
