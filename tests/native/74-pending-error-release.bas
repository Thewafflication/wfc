' A runtime error skips the Dim of the strings and Variants that follow it;
' leaving the procedure must not release slots that were never set.
Sub Dirty()
    Dim a As Long, b As Long, c As Long, d As Long, e As Long, f As Long
    a = &H7EADBEEF: b = a: c = a: d = a: e = a: f = a
    Print a - b + c - d + e - f
End Sub

Sub Boom()
    Dim a As Long
    a = 1 \ 0
    Dim s As String, v As Variant, t As String
    Dim r(3) As String
    s = "x": t = s & "y": v = t
    r(1) = t
    Print s; v
End Sub

Sub Main()
    Dirty
    Print "start"
    Boom
    Print "not reached"
End Sub
