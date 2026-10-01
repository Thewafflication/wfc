Option Explicit
Sub Main()
    Dim d As Date
    d = DateSerial(2024, 2, 28)
    Print Format(d, "yyyy-mm-dd") & " " & Format(DateAdd("d", 2, d), "yyyy-mm-dd")
    Print DateDiff("d", #1/1/2024#, #12/31/2024#) & " " & DateDiff("m", #1/31/2024#, #3/1/2024#)
    Print Weekday(#1/1/2000#) & " " & Format(#1/1/2000#, "dddd") & " " & MonthName(2)
    Print Year(d) & "/" & Month(d) & "/" & Day(d)
    Print DatePart("q", #8/15/2023#) & " " & DatePart("y", #3/1/2024#)
    Print Format(TimeSerial(13, 5, 9), "hh:nn:ss AM/PM")
    Print DateAdd("m", 1, #1/31/2023#)
End Sub
