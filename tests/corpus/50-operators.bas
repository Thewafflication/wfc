Dim calls As Long
Function T() As Boolean
    calls = calls + 1
    T = True
End Function
Function F() As Boolean
    calls = calls + 1
    F = False
End Function
Sub Main()
    Dim r As Boolean
    r = F() And T()
    Print calls
    r = T() Or T()
    Print calls
    Print 7 Mod 2.5, -7 Mod 2.5, 7.7 Mod 2, 2 + 3 & "x", "a" & 1 + 2, 1 + 2 & 3 + 4
    Print Not 1 = 2, Not (1 = 2), -2 ^ 2, 2 ^ -1, 10 - 2 - 3, 2 ^ 3 ^ 2, 100 / 10 / 2
    Print 1 < 2 = True, (1 < 2) = (2 < 3), 5 > 3 And 2 > 1, 1 + 1 = 2 And 2 + 2 = 4
    Print "a" & Null, Null & "b", IsNull("a" + Null), 1 + Null
End Sub
