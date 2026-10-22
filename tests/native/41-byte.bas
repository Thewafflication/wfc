Option Explicit
Const Limit As Byte = 200

Function Double2(ByVal b As Byte) As Byte
    Double2 = b * 2
End Function

Type Pixel
    r As Byte
    g As Byte
End Type

Sub Main()
    Dim a As Byte, b As Byte, c As Integer, l As Long, v As Variant
    Dim arr(3) As Byte, p As Pixel
    a = 100
    b = 50
    Print a + b; a - b; a * 2; a \ 7; a Mod 7; a / 8
    c = a + b
    l = CLng(a) * 1000
    Print c; l; TypeName(a); TypeName(a + b); TypeName(a + c); TypeName(a - 1)
    Print -a; TypeName(-a); a > b; a = 100; Limit
    arr(1) = 255
    arr(2) = a
    p.r = 7: p.g = 250
    Print arr(1); arr(2); p.r; p.g; Double2(60)
    v = a
    Print v; TypeName(v)
    a = v
    Print a
    Print CByte(77) + 1; TypeName(CByte(77))
    a = 255
    a = 3.6
    Print a
    On Error Resume Next
    a = 255
    a = a + 1
    Print Err.Number, a
    Err.Clear
    b = 100
    b = b * 3
    Print Err.Number
    Err.Clear
    a = -1
    Print Err.Number
    Err.Clear
    l = 300
    a = l
    Print Err.Number
    Err.Clear
    a = 4
    a = a - 5
    Print Err.Number
End Sub
