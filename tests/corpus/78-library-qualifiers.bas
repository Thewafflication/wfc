Sub Main()
    Print VBA.Left("abcdef", 3), VBA.Strings.UCase$("xy"), Strings.Len("four")
    Print Math.Sqr(16), VBA.Conversion.Int(-1.5), VBA.Information.IsNumeric("12")
    Print Interaction.IIf(1 > 2, "a", "b"), ChrW$(72) & VBA.ChrW(105)
    Print VBA.Constants.vbTab = vbTab, VBA.DateTime.Year(#6/7/2008#)
End Sub
