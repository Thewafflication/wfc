Sub Main()
    Dim n As Long, d As Double, s As String, text As String
    text = Environ$("TEMP") & "\wfc-input-number.txt"
    Open text For Output As #1
    Print #1, "Christoph"
    Print #1, "12abc"
    Print #1, "3.5x"
    Print #1, "word"
    Close #1
    Open text For Input As #1
    Input #1, n
    Print n
    Input #1, n
    Print n
    Input #1, d
    Print d
    Input #1, s
    Print s
    Close #1
    Kill text
End Sub
