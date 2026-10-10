Option Explicit
Sub Main()
    Dim f As Object, n As Long
    Print Forms.Count
    For Each f In Forms
        n = n + 1
    Next
    Print n
    On Error Resume Next
    Set f = Forms(0)
    Print Err.Number
End Sub
