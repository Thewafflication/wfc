Option Explicit
Private Declare Sub Sleep Lib "kernel32" (ByVal ms As Long)
Private Declare Function GetTickCount Lib "kernel32" () As Long
Sub Main()
    Dim n As Long, s As String, i As Long, t As Long
    n = 3
    If n Then Print "n is true"
    n = 0
    If Not n Then Print "n is false"
    While n < 2
        n = n + 1
    Wend
    Print n
    For i = 1 To 5
        s = s & IIf(i Mod 2, "o", "e")
    Next
    Print s
    Dim v As Variant
    v = Empty
    Print Len(v), IsNull(Len(Null))
    t = GetTickCount()
    Sleep 20
    Print GetTickCount() - t >= 15
    Print App.Title = "", App.Path <> "", App.Major
End Sub
