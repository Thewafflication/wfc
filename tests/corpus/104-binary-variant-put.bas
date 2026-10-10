Option Explicit
Sub Main()
    Dim f As Integer, fn As String, a As Long, s As String, d As Double
    Dim v As Variant
    fn = Environ("TEMP") & "\wfc_corpus_104.dat"
    f = FreeFile
    Open fn For Binary As #f
    Put #f, , 123456&
    Put #f, , "hello"
    Put #f, , 2.5#
    v = 7
    Put #f, , v
    v = "str"
    Put #f, , v
    Print LOF(f), Loc(f), Seek(f)
    Seek #f, 1
    Get #f, , a
    s = Space(5)
    Get #f, , s
    Get #f, , d
    Print a, s, d
    Get #f, , v
    Print v, VarType(v)
    Get #f, , v
    Print v, VarType(v)
    Close #f
    Kill fn
End Sub
