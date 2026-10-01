Sub Modify(a() As Long): a(0) = 99: End Sub
Function Sum(a() As Long) As Long
    Dim i As Long
    For i = LBound(a) To UBound(a): Sum = Sum + a(i): Next
End Function
Sub Main()
    Dim a(2) As Long, b() As Long, c() As Long, i As Long
    a(0) = 1: a(1) = 2: a(2) = 3
    b = a: b(0) = 50
    Print a(0), b(0), Sum(a), Sum(b)
    Modify a: Print a(0)
    ReDim c(1 To 2, 1 To 3)
    c(2, 3) = 7
    ReDim Preserve c(1 To 2, 1 To 5)
    Print c(2, 3), UBound(c, 2), LBound(c, 1), UBound(c)
    On Error Resume Next
    ReDim Preserve c(1 To 3, 1 To 5): Print Err.Number: Err.Clear
    Dim d(1 To 3) As Long
    Erase d: Print d(1)
    Erase b: Print UBound(b): Print Err.Number: Err.Clear
    Dim v As Variant: v = a
    v(1) = 100: Print a(1), v(1), IsArray(v), UBound(v)
    Dim e As Variant: e = Array(): Print UBound(e), LBound(e)
    Dim f(0 To 2) As String
    f(1) = "x": Print Join(f, "-") & "|" & Join(Filter(f, "x"), ",")
    Dim n As Long: n = 3
    Dim g() As Double: ReDim g(n * 2): Print UBound(g)
    Dim m(1 To 2) As Variant: m(1) = Array(1, 2): m(2) = "s"
    Print UBound(m(1)), m(2), IsArray(m(1)), IsArray(m(2))
End Sub
