Option Explicit
Sub Main()
    Print Left(12345, 2), Right(1.5, 2), Mid(12345, 2, 2), UCase(12), Trim(5)
    Print InStr(12345, 3), InStr(2, 12345, 3), InStrRev(12321, 2), StrComp(5, "5")
    Print Replace(12345, 3, "x"), LCase(True), StrReverse(123), Asc(5)
    Print Split(123, 2)(0), Join(Array(1, 2), 1), Left(Empty, 2) = ""
End Sub
