Option Explicit
Sub Main()
    Const N As Long = 50
    Dim isPrime(2 To N) As Boolean
    Dim i As Long, j As Long, out As String
    For i = 2 To N
        isPrime(i) = True
    Next i
    For i = 2 To N
        If isPrime(i) Then
            For j = i * i To N Step i
                isPrime(j) = False
            Next j
        End If
    Next i
    For i = 2 To N
        If isPrime(i) Then out = out & i & " "
    Next i
    Print RTrim$(out)
End Sub
