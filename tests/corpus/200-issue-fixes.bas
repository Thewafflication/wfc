Type Pair
    A As Long
    B As String
End Type

Sub Bump()
    Static p As Pair
    p.A = p.A + 10
    p.B = p.B & "x"
    Print p.A; p.B
End Sub

Sub Inc(ByRef v As Variant)
    v = v + 1
End Sub

Sub Main()
    Dim i As Long, n As Long, s As String
    Bump
    Bump
    For i = 1 To 3 Step 0
        Print "once"
        Exit For
    Next
    On Error Resume Next
    s = String(3, 300)
    Print Len(s), Err.Number, s
    n = 5
    Inc n
    Print n
End Sub
