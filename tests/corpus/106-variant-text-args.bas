Option Explicit
Sub Main()
    Print Left(12345, 2), Right(1.5, 2), Mid(12345, 2, 2), UCase(12), Trim(5)
    Print InStr(12345, 3), InStr(2, 12345, 3), InStrRev(12321, 2), StrComp(5, "5")
    Print Replace(12345, 3, "x"), LCase(True), StrReverse(123), Asc(5)
    Print Split(123, 2)(0), Join(Array(1, 2), 1), Left(Empty, 2) = ""
    Print Left("abc", "2"), Right("value", "1"), Mid("value", "2", "2"), Left("abc", 2.5)
    Print Chr("65"), Space("3") & "|", String("3", "*"), Choose("2", "a", "b")
    Print Sgn(False), Abs(True), TypeName(Abs(True)), Hex(True), Oct(True), Sqr(Empty)
    Print IsNull(Abs(Null)), IsNull(Int(Null)), IsNull(Hex(Null))
End Sub
