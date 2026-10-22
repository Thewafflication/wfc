Option Explicit

Sub Main()
    Dim path As String, n As Integer, a As Long, b As Double, s As String, t As String
    Dim i As Long
    path = "native_files_test.tmp"
    n = FreeFile
    Open path For Output As #n
    Print #n, "first line"
    Print #n, 42; "x"; 3.5
    Print #n, "a", "b", "c"
    Write #n, "text", 7, 2.5
    For i = 1 To 3
        Print #n, "row"; i
    Next i
    Close #n
    Print FileLen(path) > 0
    n = FreeFile
    Open path For Input As #n
    Line Input #n, s
    Print s
    Line Input #n, s
    Print "[" & s & "]"
    Line Input #n, s
    Print "[" & s & "]"
    Input #n, t, a, b
    Print t; a; b
    Do While Not EOF(n)
        Line Input #n, s
        Print s
    Loop
    Close #n
    n = FreeFile
    Open path For Binary Access Read As #n
    Print LOF(n) > 0
    s = String(5, " ")
    Get #n, 1, s
    Print s
    Close n
    Kill path
    Print Dir$(path) = ""
    On Error Resume Next
    Open "no\such\dir\file.txt" For Input As #1
    Print Err.Number
    Err.Clear
    Close
End Sub
