Private Declare Function QueryPerformanceCounter Lib "kernel32" (lpPerformanceCount As Currency) As Long
Private Declare Function QueryPerformanceFrequency Lib "kernel32" (lpFrequency As Currency) As Long
Private Declare Function GetTickCount Lib "kernel32" () As Long
Private Declare Sub Sleep Lib "kernel32" (ByVal dwMilliseconds As Long)
Private Declare Function MessageBox Lib "user32" Alias "MessageBoxA" (ByVal hWnd As Long, ByVal lpText As String, ByVal lpCaption As String, ByVal wType As Long) As Long
Sub Main()
    Dim c1 As Currency, c2 As Currency, f As Currency, t As Long
    QueryPerformanceFrequency f
    QueryPerformanceCounter c1
    Sleep 20
    QueryPerformanceCounter c2
    Print f > 0, (c2 - c1) / f >= 0.015, (c2 - c1) / f < 5
    t = GetTickCount(): Sleep 5: Print GetTickCount() - t >= 4
    Print MessageBox(0, "hi", "cap", 4), MessageBox(0, "x", "y", 0)
End Sub
