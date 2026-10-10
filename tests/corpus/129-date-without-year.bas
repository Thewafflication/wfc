Option Explicit
Sub Main()
    Print CDate("1/1") = DateSerial(Year(Date), 1, 1), CDate("Jan 1") = DateSerial(Year(Date), 1, 1)
    Print CDate("12/25 3:30 PM") = DateSerial(Year(Date), 12, 25) + TimeSerial(15, 30, 0)
    Print IsDate("13/45"), IsDate("2/30"), IsDate("2/28")
End Sub
