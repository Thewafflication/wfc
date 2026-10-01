Option Explicit
Sub Main()
    Dim fso As Object, ts As Object, f As Object
    Set fso = CreateObject("Scripting.FileSystemObject")
    Set ts = fso.CreateTextFile("fso_test.txt")
    ts.WriteLine "first line"
    ts.Write "second"
    ts.WriteLine " line"
    ts.Close
    Print fso.FileExists("fso_test.txt"), fso.FileExists("nope.txt"), fso.FolderExists(".")
    Set ts = fso.OpenTextFile("fso_test.txt", 1)
    Print ts.ReadLine()
    Print ts.AtEndOfStream, ts.Line
    Print ts.ReadAll()
    ts.Close
    Set ts = fso.OpenTextFile("fso_test.txt", 8)
    ts.WriteLine "third"
    ts.Close
    Set f = fso.GetFile("fso_test.txt")
    Print f.Name, f.Size, TypeName(f), TypeName(fso), TypeName(ts)
    Print fso.GetFileName("C:\dir\sub\file.tar.gz"), fso.GetBaseName("C:\dir\file.tar.gz"), fso.GetExtensionName("file.tar.gz"), fso.GetParentFolderName("C:\dir\sub\file.txt")
    Print fso.BuildPath("C:\dir", "x.txt"), fso.BuildPath("C:\dir\", "x.txt")
    fso.CopyFile "fso_test.txt", "fso_copy.txt"
    fso.MoveFile "fso_copy.txt", "fso_moved.txt"
    Print fso.FileExists("fso_copy.txt"), fso.FileExists("fso_moved.txt")
    fso.DeleteFile "fso_moved.txt"
    fso.DeleteFile "fso_test.txt"
    On Error Resume Next
    Set ts = fso.OpenTextFile("missing.txt", 1)
    Print Err.Number, Err.Description
End Sub
