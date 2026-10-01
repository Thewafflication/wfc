Option Explicit
Public Const MAXV As Long = 3
Private Const PFX = "p_"
Public Type Pair
    A As Long
    B As String
End Type
Public Enum Level
    Low = 1
    High = 10
End Enum
Dim g1 As Long, g2(2) As Long, g3 As String
Private g4%
Public g5 As Pair
Sub Main()
    Dim a(MAXV) As Long, b(2), c As Long, d As String * 3
    Dim e As Level
    e = High
    g2(1) = 5: g4% = 7: g5.A = 1: g5.B = "x"
    b(0) = "str": b(1) = 2
    Print UBound(a), UBound(b), e, PFX & g2(1), g4, g5.A & g5.B, b(0) & b(1)

    Print LBound(a), UBound(a)
    Static cnt As Long
    cnt = cnt + 1
    Print Low + High, Level.High, TypeName(e)
    Dim v As Variant
    v = g5
    v.A = 99
    Print g5.A, v.A
End Sub
