Sub Main()
    Dim a As Variant, b As Variant, c As Currency
    a = CDec("0.1"): b = CDec("0.2")
    Print a + b, a + b = CDec("0.3"), TypeName(a + b), a * b, b / a
    a = CDec("79228162514264337593543950335")
    On Error Resume Next
    b = a + 1: Print Err.Number: Err.Clear
    Print a - 1, a / 2, TypeName(a / 2)
    a = CDec(1) / CDec(3): Print a
    a = CDec("123456789.123456789") * 1000: Print a, Int(a), Fix(-a), Round(a, 3)
    c = 922337203685477.5807@: Print c, c - 0.0001@
    c = c + 1: Print Err.Number: Err.Clear
    Print CCur(0.00005), CCur(1.99995), 10 / 4@, 5@ * 0.5@, TypeName(5@ * 2)
    Print CDec(2) ^ 10, Sqr(CDec(16)), Abs(CDec(-3.5)), Sgn(CDec(-1)), CDbl(CDec("1.5")), CInt(CDec("2.5")), CLng(CDec("3.5"))
    Print 1.1 + 2.2, CDec(1.1) + CDec(2.2), 0.1 * 3, CDec(0.1) * 3
End Sub
