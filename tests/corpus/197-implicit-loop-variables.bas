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
    Dim path As String
    path = Environ$("TEMP") & "\wfc-implicit-input.txt"
    Open path For Output As #1
    Print #1, "7"
    Print #1, "tail"
    Close #1
    Open path For Input As #1
    Input #1, got
    Line Input #1, rest
    Close #1
    Kill path
    Print got; TypeName(got); rest
End Sub
