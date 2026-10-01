Option Explicit
Sub Main()
    Dim z As New Zoo
    Dim a As Animal
    Set a = New Animal
    a.Name = "Rex": a.Species = Dog
    z.Add a
    Set a = New Animal
    a.Name = "Tom": a.Species = Cat
    z.Add a
    Set a = Nothing
    Print z.Count
    z.Chorus
    Dim f As Animal
    Set f = z.Find("Tom")
    Print f.Describe(), f.Species = Cat
    Print z.Find("nobody") Is Nothing
    Set f = Nothing
End Sub
