Attribute VB_Name = "Mod1"
Option Explicit
Public Sub Main()
    Dim c As New Counter
    c.Inc: c.Inc
    Print "count=" & c.Value
    Print Helpers.Twice(21)
End Sub
