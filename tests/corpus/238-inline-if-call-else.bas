Option Explicit
Dim n As Long
Sub Bump()
    n = n + 1
End Sub
Sub Drop()
    n = n - 1
End Sub
Sub Add(ByVal k As Long)
    n = n + k
End Sub
Sub Show(ByVal s As String)
    Print s; n
End Sub
Sub Main()
    Dim up As Boolean
    up = True
    If up Then Bump Else Drop
    Show "a"
    up = False
    If up Then Bump Else Drop
    Show "b"
    If up Then Add 5 Else Add 7
    Show "c"
    If Not up Then Drop Else Bump: Show "d"
    If up Then n = 100 Else Bump
    Show "e"
    If up Then Bump Else n = 50
    Show "f"
End Sub
