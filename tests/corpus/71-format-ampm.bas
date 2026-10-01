Sub Main()
    Dim d As Date: d = #3/5/2001 2:07:09 PM#
    Print Format(d, "h:nn:ss AMPM")
    Print Format(d, "hh:nn am/pm")
    Print Format(d, "h:nn A/P")
    Print Format(#3/5/2001 9:05:00 AM#, "h:nn:ss AMPM")
    Print Format(d, "dddd, mmmm d, yyyy h:nn:ss AMPM")
End Sub
