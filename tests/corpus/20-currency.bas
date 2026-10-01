Option Explicit
Sub Main()
    Dim price As Currency, qty As Long, total As Currency, tax As Currency
    price = 19.99
    qty = 3
    total = price * qty
    tax = total * 0.0825
    Print total & " " & tax & " " & (total + tax)
    Print CCur(0.1) + CCur(0.2) = CCur(0.3)
    Dim d As Double
    d = 0.1
    Print d + 0.2 = 0.3
    Print Format(total, "0.00") & " " & Format(1234567.5, "#,##0.00") & " " & Format(0.075, "0.0%")
    Print Round(2.5) & " " & Round(3.5) & " " & Round(-2.5) & " " & Round(0.5) & " " & Round(1.5)
    Print FormatCurrency(1234.5) & " " & FormatNumber(1234.5678, 2) & " " & FormatPercent(0.256, 0)
    Print Pmt(0.05 / 12, 12, -1000) > 85 And Pmt(0.05 / 12, 12, -1000) < 86
    Print CCur(7) / 2 & " " & TypeName(price * 2) & " " & TypeName(CCur(1) + 1.5)
End Sub
