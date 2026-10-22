' Variants: a handle to a boxed value; the interpreter's own semantics.
Option Explicit

Function Twice(v As Variant) As Variant
    Twice = v * 2
End Function

Function Describe(ByVal v As Variant) As String
    Describe = TypeName(v) & ":" & VarType(v)
End Function

Sub Bump(ByRef v As Variant)
    v = v + 1
End Sub

Function Untyped(a, b)
    Untyped = a & "-" & b
End Function

Sub Main()
    Dim v As Variant, w As Variant, n As Long, d As Double, s As String
    Dim i As Integer, e
    Print IsEmpty(v); IsNull(v); TypeName(v); VarType(v)
    v = 5
    Print v; TypeName(v); VarType(v)
    v = v + 2.5
    Print v; TypeName(v)
    v = "text"
    Print v; TypeName(v); Len(v)
    w = 7
    v = w * 3
    Print v, w
    Print Twice(21), Twice(1.5), Twice(w)
    Print Describe(1), Describe(1&), Describe(1.5), Describe(2!), Describe("x"), Describe(True)
    Print Describe(Null), Describe(Empty), Describe(v)
    n = w
    d = w
    s = w
    i = w
    Print n, d, s, i
    v = 10
    Bump v
    Bump v
    Print v
    If w > 5 Then Print "big"
    If w = 7 And v = 12 Then Print "both"
    If w Then Print "nonzero"
    Print w + 1; w - 1; w * 2; w / 2; w \ 2; w Mod 4; w ^ 2
    Print "a" & w; w & "b"; w & v
    Print -w; Not w; w And 3; w Or 8; w Xor 2
    Print Untyped("x", 3)
    Print Untyped(1, 2)
    Print w < 8, w >= 7, w <> 7, "a" < "b"
    e = Null
    Print IsNull(e); IsNull(e + 1); IsNumeric("12"); IsNumeric("x"); IsNumeric(w)
    Print CVar(5) + CVar(6)
    v = Empty
    Print IsEmpty(v); v + 1
    On Error Resume Next
    v = "abc"
    n = v
    Print Err.Number
    Err.Clear
    v = 2147483647
    v = v + 1
    Print Err.Number, TypeName(v), v
End Sub
