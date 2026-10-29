VERSION 1.0 CLASS
Attribute VB_Name = "Box"
Option Explicit
Public Item As Variant
Public Function Same() As Box
    Set Same = Me
End Function
Public Sub Copy()
    Dim Dest As Box
    Dest = Me.Same
End Sub
