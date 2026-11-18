' A Declare function without arguments is called when it is named without
' parentheses, and Timer counts fractions of a second, as in VB6.
Private Declare Function GetTickCount Lib "kernel32" () As Long
Private Declare Function GetCurrentProcessId Lib "kernel32" () As Long

Sub Main()
    Dim t As Long, a As Single, b As Single
    t = GetTickCount
    Print t <> 0; GetTickCount <> 0; TypeName(GetTickCount)
    Print GetCurrentProcessId = GetCurrentProcessId()
    GetTickCount
    Call GetTickCount
    ' Timer moves on in less than a second.
    a = Timer
    Do
        b = Timer
    Loop While b = a
    Print TypeName(Timer); Abs(b - a) < 0.5
End Sub
