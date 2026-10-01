Option Explicit
Sub Main()
    Print Replace("aXbXc", "X", "-", , 1)
    Print Replace("aXbXc", "X", "-", 3)
    Print Replace("AxBx", "x", "_", , , 1)
    Print InStr(, "hello", "l")
    Print InStr(, "HELLO", "l", 1)
    Print InStrRev("abcabc", "b", , 1)
    Print InStrRev("abcabc", "b", 4)
End Sub
