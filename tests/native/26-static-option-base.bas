Option Explicit
Option Base 1
Option Compare Binary

Function Counter() As Long
    Static n As Long
    n = n + 1
    Counter = n
End Function

Function Remember(ByVal s As String) As String
    Static last As String
    Remember = last
    last = s
End Function

Sub Bump()
    Static total As Double
    Static flag As Boolean
    total = total + 1.5
    flag = Not flag
    Print total; flag
End Sub

Sub Main()
    Dim a(3) As Long
    Dim b() As String
    Dim i As Long
    For i = LBound(a) To UBound(a)
        a(i) = i * i
    Next i
    Print LBound(a); UBound(a); a(1); a(3)
    ReDim b(2)
    b(1) = "one"
    b(2) = "two"
    Print LBound(b); UBound(b); b(1); b(2)
    Print Counter(); Counter(); Counter()
    Print "["; Remember("a"); "]"; "["; Remember("b"); "]"; "["; Remember("c"); "]"
    Bump
    Bump
    Dim m(2, 3) As Long
    Print LBound(m, 1); UBound(m, 1); LBound(m, 2); UBound(m, 2)
End Sub
