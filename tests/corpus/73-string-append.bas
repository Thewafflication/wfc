Option Explicit
Dim g As String
Function Grow() As String
    g = g & "!"
    Grow = "+"
End Function
Sub Main()
    Dim s As String, i As Long, v As Variant, f As String * 4, b As Boolean
    For i = 1 To 5
        s = s & i & ","
    Next
    Print s
    s = s & "x" & 1.5 & True & Null & Empty
    Print s
    s = "ab"
    s = s & s & s
    Print s
    s = ""
    s = s & Len(s) & Len("abc")
    Print s
    b = (s & "z" = "03z")
    s = s & "z" = "03z"
    Print b
    g = "g"
    g = g & Grow()
    Print g
    v = "v"
    v = v & 1 & "x"
    Print v, TypeName(v)
    f = "ab"
    f = f & "cdef"
    Print f
    If i > 3 Then s = s & "T" Else s = s & "F"
    Print s
    s = s & "a": s = s & "b"
    Print s
    On Error Resume Next
    Dim o As Object
    s = s & o
    Print Err.Number
End Sub
