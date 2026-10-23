Option Explicit
Function Make(ByVal s As Double) As IShape
    Dim q As New Sq
    q.Init s
    Set Make = q
End Function
Sub ShowAll(ParamArray shapes() As Variant)
    Dim i As Long
    For i = LBound(shapes) To UBound(shapes)
        Dim sh As IShape
        Set sh = shapes(i)
        sh.Scale = 2
        Print sh.Name, sh.Area, sh.Describe()
    Next
End Sub
Sub Main()
    Dim a As IShape, b As IShape
    Set a = Make(2)
    Set b = Make(3)
    ShowAll a, b
    Print TypeOf a Is IShape, TypeOf a Is Sq, a Is b, TypeName(a)
End Sub
