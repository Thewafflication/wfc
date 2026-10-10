Option Explicit
Sub Main()
    Dim l As New Loader, n As Long, p As String, f As Integer
    p = Environ("TEMP") & Chr(92) & "wfc_com_events.xml"
    f = FreeFile
    Open p For Output As #f
    Print #f, "<root><a/></root>"
    Close #f
    l.Start p
    Do While Not l.Done And n < 200000
        DoEvents
        n = n + 1
    Loop
    Print l.Done; Right$(l.States, 1); Len(l.States) > 0; l.RootName
    Kill p
End Sub
