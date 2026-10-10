Sub Main()
    Dim fso As New FileSystemObject, f As Folder, x As Object, p As String
    p = fso.GetSpecialFolder(2) & "\wfc_n"
    If fso.FolderExists(p) Then fso.DeleteFolder p
    Set f = fso.CreateFolder(p)
    Print f.Name, f = p, TypeName(f)
    fso.CreateFolder p & "\sub1"
    fso.CreateTextFile(p & "\a.txt").Close
    fso.CreateTextFile(p & "\b.log").Close
    Set f = fso.GetFolder(p)
    For Each x In f.Files: Print x.Name, x.Size: Next
    For Each x In f.SubFolders: Print x.Name: Next
    Print f.Files.Count, f.SubFolders.Count
    fso.DeleteFile p & "\a.txt": fso.DeleteFile p & "\b.log": fso.DeleteFolder p & "\sub1"
    fso.DeleteFolder p
    Print fso.FolderExists(p), fso.GetDriveName("C:\x")
End Sub
