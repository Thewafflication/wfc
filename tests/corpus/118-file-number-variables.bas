Option Explicit
Sub Main()
    Dim f As Integer, path As String
    path = Environ("TEMP") & "\wfc_corpus_118.txt"
    f = FreeFile
    Open path For Output As #f
    Print #f, "abcdefghij"
    Close #f
    Open path For Input As #f
    Print EOF(#f), LOF(#f)
    Print Input$(3, #f)
    Print Input(2, f)
    Print Seek(#f)
    Close #f
    Kill path
End Sub
