Option Explicit
Sub Main()
    Dim text As String, words() As String, d As Object, w As Variant, i As Long, j As Long
    text = "the quick brown fox jumps over the lazy dog The DOG barks; the fox runs."
    text = LCase$(text)
    For i = 1 To Len(text)
        If Mid$(text, i, 1) Like "[!a-z ]" Then Mid$(text, i, 1) = " "
    Next
    words = Split(text, " ")
    Set d = CreateObject("Scripting.Dictionary")
    For Each w In words
        If Len(w) > 0 Then d(w) = d(w) + 1
    Next
    Dim keys As Variant: keys = d.Keys
    Dim t As Variant
    For i = LBound(keys) To UBound(keys) - 1
        For j = i + 1 To UBound(keys)
            If d(keys(j)) > d(keys(i)) Or (d(keys(j)) = d(keys(i)) And keys(j) < keys(i)) Then
                t = keys(i): keys(i) = keys(j): keys(j) = t
            End If
        Next j
    Next i
    For i = 0 To 4
        Print Format$(i + 1, "0") & ". " & keys(i) & Space(8 - Len(keys(i))) & String$(d(keys(i)), "*") & " " & d(keys(i))
    Next
    Print d.Count & " distinct, " & UBound(words) + 1 & " tokens"
End Sub
