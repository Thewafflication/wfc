Option Explicit
Sub Main()
    Dim text As String, words() As String, d As Object, w As Variant, i As Long, j As Long
    text = "the quick brown fox jumps over the lazy dog the fox"
    words = Split(text, " ")
    Set d = CreateObject("Scripting.Dictionary")
    For i = LBound(words) To UBound(words)
        If d.Exists(words(i)) Then
            d(words(i)) = d(words(i)) + 1
        Else
            d.Add words(i), 1
        End If
    Next
    Dim keys As Variant, tmp As Variant
    keys = d.Keys
    For i = LBound(keys) To UBound(keys) - 1
        For j = i + 1 To UBound(keys)
            If d(keys(j)) > d(keys(i)) Or (d(keys(j)) = d(keys(i)) And keys(j) < keys(i)) Then
                tmp = keys(i): keys(i) = keys(j): keys(j) = tmp
            End If
        Next j
    Next i
    For i = 0 To 3
        Print keys(i) & ": " & d(keys(i))
    Next
    Print "distinct=" & d.Count & " total=" & (UBound(words) + 1)
    Dim csv As String, rows() As String, f() As String, sum As Double
    csv = "a,1.5,x" & vbCrLf & "b,2.25,y" & vbCrLf & "c,3,z"
    rows = Split(csv, vbCrLf)
    For i = 0 To UBound(rows)
        f = Split(rows(i), ",")
        sum = sum + Val(f(1))
        Print f(0) & "=" & Format(Val(f(1)), "0.00") & UCase$(f(2))
    Next
    Print "sum=" & sum
End Sub
