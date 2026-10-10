Option Explicit
Sub Main()
    Print CCur("$1,234.5678"), CDbl("$5"), CDbl("-$5"), CDbl("(5)"), CDbl("5-"), CDbl("1d2")
    Print Val("1d2"), Val("1.5D-1x"), Val("12abc"), Val(" 1 2 3 ")
    Print IsNumeric("$1,000"), IsNumeric("(5)"), IsNumeric("5-"), IsNumeric("1d2")
    Print IsNumeric("1e"), IsNumeric("$"), IsNumeric("-"), IsNumeric("()"), IsNumeric("5%"), IsNumeric("1 000")
End Sub
