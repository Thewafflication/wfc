Sub Main()
    Dim d As Scripting.Dictionary, fso As New Scripting.FileSystemObject, re As RegExp, m As Object
    Set d = New Scripting.Dictionary
    d("a") = 1
    Set re = New RegExp
    re.Pattern = "(\d+)-(\d+)": re.Global = True
    Set m = re.Execute("10-20 and 30-40")
    Print d.Count, TypeName(d), TypeName(re), TypeName(fso), m.Count, m(1).SubMatches(1)
    Print fso.GetExtensionName("a/b/c.txt"), fso.FileExists("/nonexistent")
    Dim d2 As Object: Set d2 = CreateObject("Scripting.Dictionary"): Print TypeName(d2)
End Sub
