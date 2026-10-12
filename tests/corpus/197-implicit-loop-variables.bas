Sub Main()
    For i = 1 To 3
        total = total + i
    Next i
    Print total
    For Each item In Array(10, 20)
        sum2 = sum2 + item
    Next
    Print sum2
    For n% = 1 To 2
        count = count + 1
    Next
    Print count; TypeName(n)
End Sub
