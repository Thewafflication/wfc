' The ^ operator: left to right, tighter than unary minus, always Double.
Const Cube As Double = 3 ^ 3
Sub Main()
    Dim a As Long
    Dim x As Double
    a = 2
    x = 0.5
    Print 2 ^ 10
    Print a ^ 3
    Print a ^ -1
    Print 2 ^ 3 ^ 2
    Print -2 ^ 2
    Print x ^ 2, a ^ x
    Print Cube
    Print (a + 1) ^ 2 * 2
    Print 4 ^ 0.5
    Print 10 ^ 15
    On Error Resume Next
    Print 0 ^ -1
    Print Err.Number
    Err.Clear
    Print 10 ^ 400
    Print Err.Number
    Print (-8) ^ 0.5
    Print Err.Number
End Sub
