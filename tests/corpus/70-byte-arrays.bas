Option Explicit
Sub Main()
    Dim b() As Byte, s As String
    b = "AB"
    Print UBound(b), b(0), b(1), b(2), b(3)
    s = "xyz"
    b = StrConv(s, vbFromUnicode)
    Print UBound(b), b(0), b(2)
    s = StrConv(b, vbUnicode)
    Print s
    ReDim b(2): b(0) = 72: b(1) = 105: b(2) = 33
    Print StrConv(b, vbUnicode)
    b = "Hi"
    s = b
    Print s, Len(s)
End Sub
