DefInt A-C
DefStr S
#Const Edition = "pro"
Function Half(n)
    Half = n / 2
End Function
Sub Main()
    Dim alpha, sx
    Print TypeName(alpha), TypeName(sx)
    b = 4000
    Print TypeName(b)
    n% = 5
    Print TypeName(n)
#If Edition = "pro" Then
    Print "pro edition"
#Else
    Print "other"
#End If
    Print TypeName(5.), TypeName(1D3), 1D3
    Dim cur As Currency
    cur = 922337203685477.5807@
    Print cur
    Print InStr(, "hello", "l")
    On Error Resume Next
    Err.Raise 7, "src", "boom", "help.hlp", 99
    With Err
        Print .Source, .Description, .HelpFile, .HelpContext
        .Clear
        Print .Number
    End With
    Dim f As Integer
    f = FreeFile
    Open "65.tmp" For Output As #f
    Width #f, 20
    Print #f, "x"
    Print LOF(f) > 0, EOF(f)
    Close #f
    Kill "65.tmp"
End Sub
