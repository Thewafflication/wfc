Option Explicit

Dim stamp As Date

Function Next2(ByVal d As Date) As Date
    Next2 = DateAdd("d", 2, d)
End Function

Sub Main()
    Dim d As Date, price As Currency, v As Variant, i As Long
    d = #1/5/2020#
    Print d, Next2(d), Format(d, "yyyy-mm-dd")
    stamp = #3/4/2001 1:30:00 PM#
    Print stamp, TypeName(stamp), IsNumeric(stamp)
    price = 19.99
    Print price * 3, TypeName(price)
    Print "a"; Tab(10); "b"; Spc(3); "c"; Tab; "d"
    v = Array("x", "y", "z")
    For i = LBound(v) To UBound(v)
        v(i) = v(i) & i
    Next i
    Print Join(v, "-")
    Print Replace("aaa", "a", "b", , 2), VBA.Strings.UCase$("ok"), Math.Sqr(81)
End Sub
