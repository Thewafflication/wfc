Option Explicit
Sub Main()
    Dim b As New Box, c As Box, s As String
    b = 5
    Print b, b.Value, b + 1, b & "x"
    b.Value = "str"
    s = b
    Print s, Len(b), UCase(b)
    Set c = New Box
    c = b
    Print c, c.Value
    Dim col As New Collection
    col.Add b
    Print col(1), col(1).Value
    Dim v As Variant
    v = b
    Print v, TypeName(v)
    Set v = b
    Print TypeName(v)
    Print b = c, b Is c
End Sub
