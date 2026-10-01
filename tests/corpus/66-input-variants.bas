Option Explicit
Sub Main()
    Dim path As String, f As Integer, a As Variant, b As Variant, c As Variant, d As Variant, e As Variant
    path = Environ("TEMP")
    If path = "" Then path = Environ("TMPDIR")
    If path = "" Then path = "/tmp"
    path = path & "/wfc_corpus_66.txt"
    f = FreeFile
    Open path For Output As #f
    Write #f, True, #1/2/2000#, #1/2/2000 1:30:00 PM#, "s", 4, Null
    Close #f
    Open path For Input As #f
    Dim raw As String
    Line Input #f, raw
    Close #f
    Debug.Print raw
    Open path For Input As #f
    Input #f, a, b, c, d, e
    Close #f
    Kill path
    Main2 TypeName(a), TypeName(b), TypeName(c), TypeName(d), TypeName(e)
    Print raw
End Sub
Sub Main2(ParamArray t() As Variant)
    Dim i As Long
    For i = 0 To UBound(t): Print t(i): Next
End Sub
