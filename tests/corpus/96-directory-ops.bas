Sub Main()
    Dim base As String, f As String, n As Long, names As String
    base = Environ("TEMP"): If base = "" Then base = Environ("TMPDIR"): If base = "" Then base = "/tmp"
    base = base & "/wfc_dir_test"
    On Error Resume Next
    Kill base & "/a.txt": Kill base & "/b.txt": Kill base & "/c.dat": RmDir base & "/sub": RmDir base
    On Error GoTo 0
    MkDir base: MkDir base & "/sub"
    Open base & "/a.txt" For Output As #1: Print #1, "x": Close #1
    Open base & "/b.txt" For Output As #1: Print #1, "yy": Close #1
    Open base & "/c.dat" For Output As #1: Close #1
    f = Dir(base & "/*.txt")
    Do While f <> ""
        names = names & f & "(" & FileLen(base & "/" & f) & ") "
        f = Dir()
    Loop
    Print names
    f = Dir(base & "/*", vbDirectory)
    names = ""
    Do While f <> "": If f <> "." And f <> ".." Then names = names & f & " "
        f = Dir()
    Loop
    Print names
    Print GetAttr(base) And vbDirectory, GetAttr(base & "/a.txt") And vbDirectory
    SetAttr base & "/a.txt", vbReadOnly: Print GetAttr(base & "/a.txt") And vbReadOnly
    SetAttr base & "/a.txt", vbNormal
    Name base & "/c.dat" As base & "/d.dat": Print Dir(base & "/d.dat"), Dir(base & "/c.dat") = ""
    FileCopy base & "/a.txt", base & "/e.txt": Print FileLen(base & "/e.txt")
    Print FileDateTime(base & "/a.txt") <= Now
    Kill base & "/*.txt": Kill base & "/d.dat": RmDir base & "/sub": RmDir base
    Print Dir(base, vbDirectory) = ""
End Sub
