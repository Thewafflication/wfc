Sub Main()
    Dim p As New Par, c As New Collection, it As New Itm, v As Variant
    p.AddKid "a": p.AddKid "b"
    p.Kids(1).Name = "A!"
    p.Kid(2).Name = "B!"
    p.Kid(2).Bump 5
    p.Kids(2).Bump
    p.Rename 1, "A!!"
    Print p.Kid(1).Name, p.Kid(2).Name, p.Kid(2).N, p.Kids.Count
    c.Add p
    c(1).Kids(1).N = 42
    c(1).Kid(1).Bump 8
    Print p.Kid(1).N
    c(1).Name = "root": Print p.Name
    c.Add it
    c(2).Name = "item": c(2).N = c(2).N + 5
    Print it.Name, it.N
    v = Array(it)
    v(0).N = 77: Print it.N
End Sub
