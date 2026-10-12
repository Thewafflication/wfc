Private Type POINTAPI
    X As Long
    Y As Long
End Type
Private Declare Function GetCursorPos Lib "user32" (lpPoint As POINTAPI) As Long
Private Declare Function GetSystemMetrics Lib "user32" (ByVal nIndex As Long) As Long
Private Declare Function lstrlenA Lib "kernel32" (ByVal lpString As String) As Long
Private Declare Function GetWindowsDirectoryA Lib "kernel32" (ByVal lpBuffer As String, ByVal nSize As Long) As Long
Private Declare Function MulDiv Lib "kernel32" (ByVal nNumber As Long, ByVal nNumerator As Long, ByVal nDenominator As Long) As Long
Private Declare Function GetModuleHandleA Lib "kernel32" (ByVal lpModuleName As String) As Long
Private Declare Function Nonexistent Lib "kernel32" (ByVal x As Long) As Long
Private Declare Function NoLib Lib "nosuchdll.dll" (ByVal x As Long) As Long
Sub Main()
    Dim pt As POINTAPI
    Dim r As Long, buf As String
    r = GetCursorPos(pt)
    Print r; (pt.X >= -32768)
    Print GetSystemMetrics(0) > 0
    Print lstrlenA("hello")
    buf = Space$(260)
    r = GetWindowsDirectoryA(buf, 260)
    Print r > 0; Left$(buf, 3) <> "   "
    Print MulDiv(10, 3, 2)
    Print GetModuleHandleA("kernel32") <> 0
    On Error Resume Next
    Print Nonexistent(1)
    Print Err.Number
    Err.Clear
    Print NoLib(1)
    Print Err.Number
End Sub
