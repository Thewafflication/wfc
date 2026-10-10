Sub Main()
10  On Error GoTo 100
20  Dim x As Long
30  x = 1
40  x = x / 0
50  Print "after"
60  GoTo 200
100 Print "handler", Err.Number, Erl
110 Resume 50
200 Print "end"
End Sub
