Sub Main()
    Dim d As New D
    d.Item(1) = 5
    d(2) = 7
    Print d(1), d.Item(2), d(3)
    Dim x As Variant
    x = d(2) + d(1)
    Print x
End Sub
