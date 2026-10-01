Sub Main()
    Print "[" & Format(0, "#") & "]", "[" & Format(0, "#.##") & "]", "[" & Format(0.5, "#.##") & "]", "[" & Format(5, "#.##") & "]"
    Print Format(1234.5, "#,##0"), Format(-1234.5, "#,##0.00"), Format(0.1234, "0.0%"), Format(1, "0.00%")
    Print Format(12, "0000.00"), Format(3.14159, "00.000"), Format(1000000, "#,##0"), Format(123, "#,##0.0")
    Print Format(-5, "0;-0;Zero"), Format(0, "0;-0;Zero"), Format(5, "+0;-0")
    Print Format(1234.5678, "Fixed"), Format(1234.5678, "Standard"), Format(0.5, "Percent"), Format(1234.5, "Currency")
    Print Format(#1/2/2003 3:04:05 PM#, "Long Date"), Format(#1/2/2003 3:04:05 PM#, "Short Time")
    Print FormatNumber(1234.567, 1), FormatNumber(-0.5, 2, vbFalse), FormatPercent(0.256, 1), FormatNumber(1234567, 0, , , vbTrue)
    Print IsNull(Format(Null, "x"))
End Sub
