Option Explicit
Sub Main()
    Dim re As Object, m As Object, ms As Object
    Set re = CreateObject("VBScript.RegExp")
    re.Pattern = "(\d+)-(\d+)"
    re.Global = True
    Print re.Test("tel 555-1234 and 666-9999"), re.Test("none")
    Set ms = re.Execute("tel 555-1234 and 666-9999")
    Print ms.Count, TypeName(ms)
    For Each m In ms
        Print m.Value, m.FirstIndex, m.Length, m.SubMatches.Count, m.SubMatches(0), m.SubMatches(1)
    Next
    Print ms(1).Value
    Print re.Replace("555-1234", "$2/$1"), re.Replace("a 1-2 b 3-4", "<$&>")
    re.Global = False
    Print re.Replace("a 1-2 b 3-4", "X")
    re.Pattern = "hello"
    re.IgnoreCase = True
    Print re.Test("Say HELLO"), re.Replace("Hello hello", "bye")
    re.Pattern = "^\w+$"
    re.MultiLine = True
    re.Global = True
    Set ms = re.Execute("one" & vbLf & "two words" & vbLf & "three")
    Print ms.Count
    On Error Resume Next
    re.Pattern = "(unclosed"
    Print re.Test("x")
    Print Err.Number
End Sub
