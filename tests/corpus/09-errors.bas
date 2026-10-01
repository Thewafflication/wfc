Option Explicit
Function Safe(a As Long, b As Long) As String
    On Error GoTo bad
    Safe = CStr(a \ b)
    Exit Function
bad:
    Safe = "err" & Err.Number
    Resume Next
End Function
Sub Main()
    Print Safe(10, 3) & " " & Safe(1, 0)
    On Error Resume Next
    Dim x As Long
    x = 2147483647
    x = x + 1
    Print Err.Number & " " & Err.Description
    Err.Clear
    Dim a(1 To 2) As Long
    a(3) = 1
    Print Err.Number & " " & Err.Description
    Err.Raise vbObjectError + 5, , "custom"
    Print Err.Number - vbObjectError & " " & Err.Description
End Sub
