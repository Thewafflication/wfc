Sub Main()
    Print Format(0.5, "0"), Format(1.5, "0"), Format(2.5, "0"), Format(-2.5, "0")
    Print Format(0.285, "0.00"), Format(2.675, "0.00"), Format(0.125, "0.00"), Format(99.995, "0.00")
    Print Format(1234.5, "#,##0"), Format(0.999, "0.0"), Format(2.5, "Fixed"), Format(1234.565, "Standard")
    Print Format(123456789.999, "#,##0.00"), Format(0.00049, "0.000"), Format(1E15, "0")
    Print Hex(-1), Hex(-1&), Oct(-1), Hex(-32768), Hex(CByte(200)), Hex(65535&)
    Print Round(2.5), Round(3.5), Round(0.125, 2), Round(-2.5)
End Sub
