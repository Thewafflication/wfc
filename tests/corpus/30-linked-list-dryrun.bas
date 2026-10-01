Option Explicit
Sub Main()
    Dim x As Nd, s As String, n As Long
    ' none of these branches run; none may raise
    If Not x Is Nothing Then s = Left(x.Name, 1) & Mid(x.Name, 2) & Trim(x.Name) & CStr(x.V)
    If Not x Is Nothing Then s = Format(x.V, "0") & Replace(x.Name, "a", "b") & String(x.V, "a")
    If Not x Is Nothing Then n = Len(x.Name) + InStr(x.Name, "a") + Asc(x.Name) + Val(x.Name) + Abs(x.V)
    If Not x Is Nothing Then s = Join(Split(x.Name, ","), "-") & Hex(x.V) & LCase(x.Nxt.Name)
    If Not x Is Nothing Then If x.Name = "q" And x.Nxt.V > 3 Then s = "z"
    If Not x Is Nothing Then If x.Name Like "a*" Then s = "z"
    Set x = New Nd
    x.Name = "abc"
    If Not x Is Nothing Then If x.Name = "abc" Then Print "nested if ok"
    If x.V = 0 Then Print "zero" Else Print "nonzero"
    Dim f As Integer: f = FreeFile
    Open "30.tmp" For Output As #f
    If x.V = 0 Then Print #f, "line"
    If x.V = 0 Then Close #f
    Open "30.tmp" For Input As #f
    Dim t As String
    If Not EOF(f) Then Line Input #f, t
    Close #f
    Kill "30.tmp"
    Print t
    Print Format(#1/5/2020#, "w"), Format(#2/5/2020#, "y"), Format(#5/5/2020#, "q"), Format(12, "@@@@") & "|"
End Sub
