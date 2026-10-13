Public arr(0 To 5) As Variant
Public names(0 To 5) As String
Sub Main()
    Dim n As Integer, text As String
    text = Environ$("TEMP") & "\wfc-input-array.txt"
    Open text For Output As #1
    Print #1, """a b"""
    Print #1, """c"""
    Print #1, "3.5"
    Print #1, ""
    Print #1, "last"
    Close #1
    Open text For Input As #1
    Input #1, arr(n)
    n = 1
    Input #1, names(n)
    n = n + 1
    Input #1, arr(n)
    Input #1, names(3)
    Input #1, names(4)
    Close #1
    Kill text
    Print arr(0); "|"; names(1); "|"; arr(2); "|"; names(3); "|"; names(4)
    Print TypeName(arr(0)); " "; TypeName(arr(2))
End Sub
