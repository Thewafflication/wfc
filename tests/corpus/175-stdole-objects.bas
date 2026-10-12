Option Explicit
Sub Main()
    Dim W As New Watcher
    Set W.F = New StdFont
    Dim F As StdFont
    Set F = W.F
    F.Name = "Arial"
    F.Size = 12
    F.Bold = True
    F.Bold = True
    Debug.Print F.Name; F.Size; F.Bold; F.Weight; F.Charset; F.Italic
    F.Weight = 300
    Debug.Print F.Bold; F.Weight
    Dim G As StdFont
    Set G = F.Clone
    Debug.Print G.Name; G.Size; F.IsEqual(G)
    G.Italic = True
    Debug.Print F.IsEqual(G); (F.hFont <> 0); F.hFont = F.hFont
    Debug.Print TypeName(F); TypeName(G)
    Debug.Print F
    Dim P As StdPicture
    Set P = New StdPicture
    Debug.Print P.Handle; P.Type; P.Width; P.Height; P.Attributes; P.KeepOriginalFormat
    P.KeepOriginalFormat = False
    Debug.Print P.KeepOriginalFormat; TypeName(P)
    Dim O As Object
    Set O = CreateObject("StdFont")
    O.Name = "Courier"
    Debug.Print O.Name; TypeName(O)
End Sub
