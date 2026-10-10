Option Explicit
Private Type T
    V As Variant
End Type
Function VarFn(Optional n As Long = 0) As Variant
    If n = 0 Then VarFn = "z" Else VarFn = n
End Function
Sub Main()
    Dim c As New Collection, d As Object, t As T
    c.Add 5: c.Add "z"
    Print c(1) < c(2), c(2) > c(1)
    Set d = CreateObject("Scripting.Dictionary")
    d("k") = "str": d("n") = 4
    Print d("n") < d("k")
    Print VarFn(3) < VarFn(), VarFn() > VarFn(3)
    t.V = "7"
    Print 5 < t.V, t.V > 5
    Print CVar(3) < CVar("a"), IIf(True, 3, "a") < IIf(False, 3, "a")
    On Error Resume Next
    Print 5 < VarFn()
    Print Err.Number
End Sub
