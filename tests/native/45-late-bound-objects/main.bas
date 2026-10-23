Attribute VB_Name = "Main"
Option Explicit

Sub Main()
    Dim o As Object
    Dim x As Long, y As Long
    Set o = New Counter
    o.Name = "c1"
    o.Add 5
    o.Add 7
    Print o.Total
    o.Total = 100
    Print o.Total, o.Name
    Print o.Twice(21)
    x = 3: y = 9
    o.Swap x, y
    Print x, y
    Print o.Describe("<")
    Print o.Describe("<", "?")
    Print o.Self.Total
    Dim c As Counter
    Set c = o
    Print c.Total
    Print o Is c
    Dim v As Variant
    Set v = o
    v.Add 1
    Print v.Total
End Sub
