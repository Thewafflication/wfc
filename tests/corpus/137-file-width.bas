Option Explicit
Sub Main()
    Dim f As Integer, s As String, path As String
    path = Environ("TEMP") & "\wfc_corpus_137.txt"
    f = FreeFile
    Open path For Output As #f
    Width #f, 10
    Print #f, "abcdefghijklmnopqrstuvwxyz"
    Print #f, "short"
    Close #f
    Open path For Input As #f
    Do Until EOF(f)
        Line Input #f, s
        Print "[" & s & "]"
    Loop
    Close #f
    Kill path
End Sub
