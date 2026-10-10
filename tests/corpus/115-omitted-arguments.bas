Option Explicit
Sub Main()
    Print Join(Split("a1b1c", "1", , 1), "-"), Join(Split("a b", , 1), "+")
    Print UBound(Filter(Array("a", "b", "A"), "a", , 1)), StrComp("a", "A", 1)
    Print DatePart("ww", #1/5/2020#, , 2), WeekdayName(1, , 1)
    Print FormatNumber(2.5, 0), FormatPercent(0.125, 0), FormatNumber(1.005, 2)
End Sub
