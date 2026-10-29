Attribute VB_Name = "Src"
Option Explicit
Public Event Ping(Obj As Src)
Public Sub Fire()
    RaiseEvent Ping(Me)
End Sub
