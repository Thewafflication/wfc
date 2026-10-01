Option Explicit
Sub Main()
    Dim v As Variant
    v = CInt(5)
    Print TypeName(v) & " " & VarType(v)
    v = "text"
    Print TypeName(v) & " " & Len(v)
    v = 2.5
    Print TypeName(v) & " " & v * 2
    v = Null
    Print IsNull(v) & " " & TypeName(v)
    Print IsNull(v + 1) & " " & IsNull(v & "x") & " " & (v & "x")
    v = Empty
    Print IsEmpty(v) & " " & TypeName(v) & " " & (v + 3) & " [" & v & "]"
    v = Array(1, "two", 3.5)
    Print IsArray(v) & " " & UBound(v) & " " & TypeName(v(1))
    v = #1/1/2000#
    Print TypeName(v) & " " & IsDate(v) & " " & IsNumeric(v)
    Dim a As Variant, b As Variant
    a = 10: b = "5"
    Print a + b & " " & a & b
    Print IsNumeric("12") & IsNumeric("1e3") & IsNumeric("abc") & IsNumeric("")
    Print CInt("7") + CLng("8") & " " & CDbl("1.5") * 2 & " " & CStr(99) & "!" & CBool(0)
End Sub
