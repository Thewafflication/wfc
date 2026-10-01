Option Compare Text
Sub Main()
    Print Replace("Hello HELLO hello", "hello", "X"), UBound(Split("aXbxc", "x")), InStr("ABC", "b"), InStrRev("ABCABC", "b")
    Print UBound(Filter(Array("Apple", "apricot", "Banana"), "AP")), StrComp("a", "A"), "a" = "A", "abc" Like "ABC"
    Dim s As String: s = "B"
    Select Case s
        Case "b": Print "sel text"
    End Select
    Print Replace("Hello HELLO", "hello", "X", , , vbBinaryCompare)
End Sub
