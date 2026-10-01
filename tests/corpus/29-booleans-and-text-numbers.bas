Sub Main()
    Print True + True, True * 5, -True, 7 \ True, 5 - False
    Print CDbl("1,000.5"), CLng("&H10"), IsNumeric("&H10"), IsNumeric("1,,2"), CInt("&HFFFF")
    Print Len(Date$), Len(Time$), TypeName(Date$), TypeName(Time$)
End Sub
