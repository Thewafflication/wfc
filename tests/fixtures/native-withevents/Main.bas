Attribute VB_Name = "Main"
Option Explicit
Sub Main()
    Dim a As New Src
    Dim h As New Hnd
    Set h.S = a
    a.Fire
    Print "hits"; h.Hits
    Dim h2 As New Hnd
    Set h2.S = a
    a.Fire
    Print "hits"; h.Hits; h2.Hits
End Sub
