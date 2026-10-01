Option Explicit
Sub Main()
    Dim path As String, f As Integer, line As String, n As Long
    path = Environ("TEMP")
    If path = "" Then path = Environ("TMPDIR")
    If path = "" Then path = "/tmp"
    path = path & "/wfc_corpus_17.txt"
    f = FreeFile
    Open path For Output As #f
    Print #f, "alpha"
    Print #f, "beta"; "gamma"
    Write #f, "q", 42, True
    Close #f
    Open path For Input As #1
    Do While Not EOF(1)
        Line Input #1, line
        n = n + 1
        Print n & ": " & line
    Loop
    Close #1
    Open path For Append As #1
    Print #1, "delta"
    Close #1
    Print FileLen(path)
    Open path For Input As #1
    Dim s As String, i As Long, b As Boolean
    Line Input #1, line
    Line Input #1, line
    Input #1, s, i, b
    Print s & "|" & i & "|" & b
    Close #1
    Kill path
    Print Dir(path) = ""
End Sub
