Sub Main()
    Dim n As Long
    n = 2
    On n GoTo a, b
    Print "none"
    GoTo done
a: Print "a": GoTo done
b: Print "b"
done:
    Print "end"
    Open "41.tmp" For Output As #1
    Width #1, 80
    Print #1, "x"
    Lock #1
    Unlock #1
    Close #1
    Kill "41.tmp"
    Dim width As Long
    width = 3
    Print width
End Sub
