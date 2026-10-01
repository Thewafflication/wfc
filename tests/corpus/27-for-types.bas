Option Explicit
Sub Show(ByVal n As Long)
    Print "n="; n
End Sub
Sub Main()
    Dim i As Integer, b As Byte, d As Double, s As Single, v As Variant, L As Long
    For i = 1 To 3: Print i;: Next: Print
    For b = 250 To 252: Print b;: Next: Print
    For d = 0 To 1 Step 0.25: Print d;: Next: Print
    For s = 1 To 2 Step 0.5: Print s;: Next: Print
    For v = 1 To 3: Print v;: Next: Print
    For v = 0.5 To 1.5: Print v;: Next: Print
    For i = 10 To 1 Step -3: Print i;: Next: Print
    For L = 1 To 10
        If L Mod 2 = 0 Then L = L + 1
        Print L;
    Next
    Print
    If L > 5 Then Show L
    For d = 1 To 0: Print "never": Next
    Print d
End Sub
