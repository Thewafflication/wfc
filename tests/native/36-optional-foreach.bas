Option Explicit
Attribute VB_Name = "Main"

Function Join3(ByVal a As String, Optional ByVal b As String = "-", Optional c As Long = 7) As String
    Join3 = a & b & c
End Function

Sub Describe(Optional v As Variant, Optional w As Variant = 5, Optional n As Long)
    If IsMissing(v) Then
        Print "v missing", w, n
    Else
        Print "v is "; v, w, n
    End If
End Sub

Function Total(ParamStart As Long, Optional Step2 As Long = 1) As Long
    Dim i As Long
    For i = 1 To ParamStart Step Step2
        Total = Total + i
    Next i
End Function

Sub Main()
    Dim a(3) As Long, s(1 To 3) As String, d(2) As Double, v As Variant, x As Long
    Dim t As String
    Print Join3("x"); Join3("x", "+"); Join3("x", "+", 9); Join3("x", , 4)
    Describe
    Describe 3
    Describe 3, 6, 9
    Describe , 2
    Print Total(5); Total(5, 2)
    a(0) = 10: a(1) = 20: a(2) = 30: a(3) = 40
    For Each v In a
        x = x + v
    Next v
    Print x
    s(1) = "p": s(2) = "q": s(3) = "r"
    For Each v In s
        t = t & v
    Next v
    Print t
    d(0) = 1.5: d(1) = 2.5: d(2) = 3.5
    Dim e As Variant
    For Each e In d
        Print e;
    Next
    Print
    For Each v In a
        If v = 30 Then Exit For
        x = x + 1
    Next
    Print x
    Randomize 42
    Dim r1 As Double
    r1 = Rnd
    Randomize 42
    Print r1 = Rnd
    Randomize
End Sub
