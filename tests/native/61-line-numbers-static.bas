Static Function Counter() As Long
    Dim n As Long
    n = n + 1
    Counter = n
End Function

Sub Main()
    Dim words() As String
    words = Split("the quick fox", " ")
    Print Join(words, "-")
    Dim v As Variant
    v = words
    Print UBound(v), v(1)
    Print Counter(), Counter(), Counter()
    Dim n As Long
    Do: n = n + 1: Loop Until n = 4
    Print n
    On Error GoTo 100
20  Dim x As Long
30  x = 1
40  x = x / 0
50  Print "after"
60  GoTo 200
100 Print "handler", Err.Number, Erl
110 Resume 50
200 Print "end"
    Dim i As Long
    For i = 1 To 3
        On i GoTo 300, 310, 320
300     Print "one": GoTo 330
310     Print "two": GoTo 330
320     Print "three"
330    n = 0
    Next i
End Sub
