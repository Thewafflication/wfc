Option Explicit
Sub Main()
    Print Format(1234567, "0,"), Format(1234567, "0.0,,"), Format(1234567, "#,##0,"), Format(1234567, "#,##0")
    Print Format(1, "#,##0.00;;Zero"), Format(0, "#,##0.00;;Zero"), "[" & Format(-1, "#,##0.00;;Zero") & "]"
    Print Format(5, "0;;;"), "[" & Format(-5, "0;;;") & "]", Format(1234567890, "#,##0,,")
End Sub
