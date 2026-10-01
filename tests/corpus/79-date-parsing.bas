Sub Main()
    Dim d As Date: d = #3/15/2024#
    Print CDate("March 5, 2024"), CDate("5 Mar 2024"), CDate("2024-03-05"), CDate("3/5/24 4:30 PM")
    Print CDate("Friday, March 15, 2024"), CDate("15-Mar-24"), CDate("Mar 2024"), CDate("December 25, 1999 6:15 AM")
    Print IsDate("Feb 30, 2024"), IsDate("Smarch 3, 2024"), IsDate("5 Mar 2024"), IsDate("Jan 1")
    Print DatePart("ww", d), DatePart("ww", d, vbMonday), DatePart("ww", #1/1/2024#, vbMonday, vbFirstFourDays)
    Print DatePart("ww", #12/31/2024#, vbSunday, vbFirstFullWeek), DatePart("ww", #12/30/2024#, vbMonday, vbFirstFourDays)
    Print DateDiff("w", #1/1/2024#, #3/15/2024#), DateDiff("ww", #1/1/2024#, #3/15/2024#), DateDiff("ww", #1/1/2024#, #1/8/2024#, vbMonday)
End Sub
