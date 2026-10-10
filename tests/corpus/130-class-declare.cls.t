Option Explicit
Private Declare Function GetTickCount Lib "kernel32" () As Long
Private Declare Sub Sleep Lib "kernel32" (ByVal ms As Long)
Public Function Elapsed() As Long
    Dim t As Long
    t = GetTickCount()
    Sleep 30
    Elapsed = GetTickCount() - t
End Function
