Option Explicit
Sub Main()
    Dim path As String
    path = Environ("TEMP") & "\wfc_corpus_119.txt"
    Open path For Output As #1
    On Error Resume Next
    Open path For Input As #2
    Print Err.Number, Err.Description
    Err.Clear
    Close
    Open path For Input Shared As #1
    Open path For Input Shared As #2
    Print Err.Number
    Close
    Kill path
    Print Dir(Environ("TEMP") & "\wfc_no_such_dir_119\*", vbDirectory) = ""
End Sub
