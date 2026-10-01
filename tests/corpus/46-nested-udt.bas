Option Explicit
Type Inner
    V(1 To 3) As Long
    Tag As String
End Type
Type Outer
    I As Inner
    Items() As Inner
    N As Long
End Type
Sub Main()
    Dim pa(1 To 2) As Inner, pb() As Inner
    pa(1).Tag = "orig"
    pb = pa
    pb(1).Tag = "copy"
    Print pa(1).Tag, pb(1).Tag
    Dim o As Outer, o2 As Outer
    o.I.V(2) = 5
    o.I.Tag = "t"
    ReDim o.Items(1 To 2)
    o.Items(2).V(3) = 9
    o.Items(2).Tag = "deep"
    o.N = 2
    o2 = o
    o2.I.V(2) = 6
    o2.Items(2).Tag = "changed"
    Print o.I.V(2), o2.I.V(2), o.Items(2).V(3), o.Items(2).Tag, o2.Items(2).Tag
    Dim arr() As Outer
    ReDim arr(1 To 2)
    arr(1).N = 1: arr(2).I.Tag = "x"
    ReDim Preserve arr(1 To 3)
    Print UBound(arr), arr(1).N, arr(2).I.Tag, arr(3).N
    With o.I
        .V(1) = 11
        .Tag = .Tag & "!"
    End With
    Print o.I.V(1), o.I.Tag, Len(o.I)
End Sub
