Option Explicit
Sub Main()
    Print Format(1234567.891, "#,##0.00") & "|" & Format(0.5, "0.0%") & "|" & Format(-5.5, "0.00")
    Print Format(42, "00000") & "|" & Format(3.14159, "0.###") & "|" & Format(1000, "$#,##0")
    Print Format(True, "Yes/No") & "|" & Format(0, "General Number") & "|" & Format(12.3456, "Fixed")
    Print Hex(255) & " " & Oct(8) & " " & &HFF & " " & (6 And 3) & " " & (6 Or 3) & " " & (6 Xor 3)
    Print Round(2.5) & " " & Round(3.5) & " " & Round(-2.5) & " " & Int(-2.5) & " " & Fix(-2.5)
    Print CInt(2.5) & " " & CInt(3.5) & " " & 7 \ 2 & " " & -7 \ 2 & " " & 7 Mod -3 & " " & -7 Mod 3
    Print 10 / 4 & " " & 2 ^ 0.5 & " " & Sqr(2)
End Sub
