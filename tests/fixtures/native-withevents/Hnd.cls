Attribute VB_Name = "Hnd"
Option Explicit
Public WithEvents S As Src
Public Hits As Long
Private Sub S_Ping(Obj As Src)
    Hits = Hits + 1
End Sub
