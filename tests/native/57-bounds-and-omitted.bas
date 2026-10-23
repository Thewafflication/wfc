Option Explicit

Sub Main()
    Const N As Long = 12
    Dim flags(2 To N) As Boolean
    Dim r As Long, c As Long, grid(0 To 2, 0 To 3) As Long
    Dim items As New Collection, d As Object, total As Long
    Dim k As Long, text As String
    For k = LBound(flags) To UBound(flags)
        flags(k) = (k Mod 3 = 0)
        If flags(k) Then text = text & k & " "
    Next k
    Print text
    For r = 0 To 2
        For c = 0 To 3
            grid(r, c) = r * 10 + c
        Next c, r
    For r = 0 To 2
        For c = 0 To 3
            total = total + grid(r, c)
        Next c
    Next r
    Print total
    items.Add "a"
    items.Add "front", , 1
    items.Add "after", "key", , 2
    Print items(1), items(2), items(3), items("key")
    Set d = CreateObject("Scripting.Dictionary")
    d("one") = 1
    d("two") = 2
    Print d.Count, d("one") + d("two")
End Sub
