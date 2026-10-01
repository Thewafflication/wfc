Option Explicit
Sub Main()
    Dim c As New Collection, d As Object, i As Long, total As Long
    Set d = CreateObject("Scripting.Dictionary")
    For i = 1 To 3000
        c.Add i * 2, "K" & i
        d.Add "key" & i, i
    Next
    For i = 1 To c.Count
        total = total + c(i)
    Next
    Print c.Count, d.Count, total, c("k2999"), c("K3000"), d("key1500")
    For i = 1 To 3000 Step 2
        c.Remove "k" & i
        d.Remove "key" & i
    Next
    Print c.Count, d.Count, c(1), c(c.Count), d.Exists("key1"), d.Exists("key2")
    c.Add "front", , 1
    c.Add "after2", "ak", , 2
    Print c(1), c(2), c(3), c("ak")
    d("Key2") = "x"
    Print d.Count, d("key2"), d("Key2")
    Dim v As Variant, n As Long
    For Each v In d.Keys
        n = n + 1
        If n = 1 Then Print v
    Next
    On Error Resume Next
    c.Add 1, "K2"
    Print Err.Number
    Err.Clear
    Print c(5000)
    Print Err.Number
    Err.Clear
    Print c("nope")
    Print Err.Number
    d.RemoveAll
    Print d.Count
End Sub
