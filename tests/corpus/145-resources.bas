Option Explicit
Sub Main()
    Dim b() As Byte, i As Long
    Print LoadResString(1); " "; LoadResString(2); " "; Len(LoadResString(16)); AscW(Mid(LoadResString(16), 3, 1))
    b = LoadResData(101, "DATA")
    For i = LBound(b) To UBound(b): Print b(i);: Next
    Print
    On Error Resume Next
    Print LoadResString(99)
    Print Err.Number; Err.Description
    Err.Clear
    b = LoadResData(7, "DATA")
    Print Err.Number
End Sub
