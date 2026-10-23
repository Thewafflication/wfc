Option Explicit
Private Declare Function GetPrivateProfileString Lib "kernel32" Alias "GetPrivateProfileStringA" (ByVal lpApplicationName As String, ByVal lpKeyName As Any, ByVal lpDefault As String, ByVal lpReturnedString As String, ByVal nSize As Long, ByVal lpFileName As String) As Long
Private Declare Function WritePrivateProfileString Lib "kernel32" Alias "WritePrivateProfileStringA" (ByVal lpApplicationName As String, ByVal lpKeyName As Any, ByVal lpString As Any, ByVal lpFileName As String) As Long
Private Declare Function GetPrivateProfileInt Lib "kernel32" Alias "GetPrivateProfileIntA" (ByVal lpApplicationName As String, ByVal lpKeyName As String, ByVal nDefault As Long, ByVal lpFileName As String) As Long
Private Declare Function GetUserName Lib "advapi32.dll" Alias "GetUserNameA" (ByVal lpBuffer As String, nSize As Long) As Long
Private Declare Function GetTempPath Lib "kernel32" Alias "GetTempPathA" (ByVal nBufferLength As Long, ByVal lpBuffer As String) As Long
Private Declare Function GetWindowsDirectory Lib "kernel32" Alias "GetWindowsDirectoryA" (ByVal lpBuffer As String, ByVal nSize As Long) As Long

Function ReadIni(sec As String, key As String, dflt As String, file As String) As String
    Dim buf As String, n As Long
    buf = Space$(255)
    n = GetPrivateProfileString(sec, key, dflt, buf, Len(buf), file)
    ReadIni = Left$(buf, n)
End Function

Sub Main()
    Dim f As String, n As Long, buf As String
    f = Environ("TEMP") & "\wfc_test.ini"
    On Error Resume Next: Kill f: On Error GoTo 0
    Print WritePrivateProfileString("Window", "Width", "800", f)
    WritePrivateProfileString "Window", "Height", "600", f
    WritePrivateProfileString "User", "Name", "Zed", f
    WritePrivateProfileString "Window", "Width", "1024", f
    Print ReadIni("Window", "Width", "?", f), ReadIni("window", "HEIGHT", "?", f), ReadIni("User", "Name", "?", f), ReadIni("User", "Nope", "dflt", f)
    Print GetPrivateProfileInt("Window", "Height", 1, f), GetPrivateProfileInt("Window", "Zip", 42, f)
    buf = Space$(100): n = GetUserName(buf, 100)
    Print n, Len(Trim$(Left$(buf, InStr(buf, vbNullChar) - 1))) > 0
    buf = Space$(260): n = GetTempPath(260, buf): Print n > 3, Left$(buf, n) = Environ("TEMP") & "\"
    buf = Space$(260): n = GetWindowsDirectory(buf, 260): Print n > 2
    Dim t As Integer: t = FreeFile
    Open f For Input As #t
    Dim line As String
    Do Until EOF(t): Line Input #t, line: Print "|" & line & "|": Loop
    Close #t
    Kill f
End Sub
