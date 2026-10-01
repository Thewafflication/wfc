Sub T(ByVal label As String)
    Print label & ": " & Err.Number & " " & Err.Description
    Err.Clear
End Sub
Sub TwoArgs(a, b): End Sub
Sub Main()
    Dim o As Object, c As New Obj, a() As Long, f As Integer, s As String, v As Variant, l As Long, tmp As String
    tmp = Environ("TEMP")
    If tmp = "" Then tmp = Environ("TMPDIR")
    If tmp = "" Then tmp = "/tmp"
    On Error Resume Next
    o.Hello: T "nothing call"
    c.Nope: T "unknown member"
    l = a(1): T "unallocated"
    ReDim a(2): l = a(5): T "range"
    Erase a: l = UBound(a): T "erased"
    Open tmp & "/wfc_no_such_dir_85/x.txt" For Input As #1: T "path"
    Open tmp & "/wfc_no_such_file_85.txt" For Input As #1: T "file"
    Line Input #7, s: T "badnum"
    Open tmp & "/wfc_corpus_85.txt" For Output As #1: Print #1, "one line": Close #1
    Open tmp & "/wfc_corpus_85.txt" For Input As #1
    Line Input #1, s: Line Input #1, s: Line Input #1, s: T "eof"
    Close: Kill tmp & "/wfc_corpus_85.txt"
    v = Null: l = v + 1: l = Len(v): s = v: T "null"
    l = CInt("x"): T "cint"
    Set o = CreateObject("No.Such"): T "createobj"
    c.RO = 5: T "readonly"
    l = 1 \ 0: T "div"
    s = Space(-1): T "space"
    s = Mid("abc", 0): T "mid"
    l = Chr(300): T "chr"
    l = Asc(""): T "asc"
    s = String(-1, "a"): T "string"
    v = Array(1, 2)(5): T "arr"
    l = Choose(5, 1, 2): T "choose"
    l = Sqr(-4): T "sqr"
    ReDim a(-1 To -5): T "redim"
    Dim e As Variant: e = CVErr(2007): l = e: T "cverr"
End Sub
