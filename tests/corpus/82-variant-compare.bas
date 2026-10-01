Option Explicit
Sub Main()
    Dim v As Variant, w As Variant, s As String, n As Long, m As Variant
    v = "10": w = 10: s = "10": n = 9: m = 5
    Print v > n, w > n, v = s, w = n + 1
    Print v > "9", w > "9", m > "9"
    Print v = w, v < w, v > w
    Print True = -1, False = 0, True = 1, n = True
    Print 10 > 9, "10" > "9", "a" < "B", 1 = 1.0
    On Error Resume Next
    Print s > n
    Print Err.Number: Err.Clear
    v = "abc"
    Print v > n
    Print Err.Number
End Sub
