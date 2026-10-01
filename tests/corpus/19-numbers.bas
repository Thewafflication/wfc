Option Explicit
Function ToBase(n As Long, b As Long) As String
    Const digits As String = "0123456789ABCDEF"
    If n = 0 Then ToBase = "0": Exit Function
    Do While n > 0
        ToBase = Mid$(digits, (n Mod b) + 1, 1) & ToBase
        n = n \ b
    Loop
End Function
Function Roman(ByVal n As Long) As String
    Dim v As Variant, r As Variant, i As Long
    v = Array(1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1)
    r = Array("M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I")
    For i = 0 To 12
        Do While n >= v(i)
            Roman = Roman & r(i)
            n = n - v(i)
        Loop
    Next i
End Function
Function IsLeap(y As Long) As Boolean
    IsLeap = (y Mod 4 = 0 And y Mod 100 <> 0) Or (y Mod 400 = 0)
End Function
Function Factors(ByVal n As Long) As String
    Dim p As Long
    p = 2
    Do While n > 1
        Do While n Mod p = 0
            Factors = Factors & p & " "
            n = n \ p
        Loop
        p = p + 1
    Loop
    Factors = RTrim$(Factors)
End Function
Sub Main()
    Print ToBase(255, 2) & " " & ToBase(255, 16) & " " & ToBase(0, 2) & " " & ToBase(1000, 8)
    Print Roman(1994) & " " & Roman(2024) & " " & Roman(4)
    Print IsLeap(1900) & " " & IsLeap(2000) & " " & IsLeap(2024) & " " & IsLeap(2023)
    Print Factors(360) & " | " & Factors(97) & " | " & Factors(1001)
    Print Hex(255) & " " & Hex(256) & " " & Oct(64) & " " & &H10 & " " & &O17
    Dim i As Long, pascal As String
    Dim row(0 To 5) As Long
    row(0) = 1
    For i = 1 To 5
        Dim j As Long
        For j = i To 1 Step -1
            row(j) = row(j) + row(j - 1)
        Next j
    Next i
    For i = 0 To 5
        pascal = pascal & row(i) & " "
    Next i
    Print RTrim$(pascal)
End Sub
