Function Half(ByVal n As Long, ByVal d As Long) As Long
    Half = n \ d
End Function

Sub Main()
    Print Half(10, 3)
    Print Half(-10, 3)
    Print 10 Mod 4; -10 Mod 4
    Print Half(5, 0)
    Print "not reached"
End Sub
