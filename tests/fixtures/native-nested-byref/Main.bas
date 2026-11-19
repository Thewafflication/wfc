Attribute VB_Name = "Main"
Option Explicit

Sub Main()
    FMain.Run
End Sub

' True when the control sits directly on the form (the theme code of the
' games centers a dialog this way).
Public Function OwnerOf(frm As Form, c As Object) As Boolean
    OwnerOf = (c.Container Is frm)
End Function
