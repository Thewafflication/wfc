Option Explicit
Sub InsertionSort(a() As Long)
    Dim i As Long, j As Long, key As Long
    For i = LBound(a) + 1 To UBound(a)
        key = a(i)
        j = i - 1
        Do While j >= LBound(a)
            If a(j) <= key Then Exit Do
            a(j + 1) = a(j)
            j = j - 1
        Loop
        a(j + 1) = key
    Next i
End Sub
Function Show(a() As Long) As String
    Dim i As Long
    For i = LBound(a) To UBound(a)
        Show = Show & a(i) & IIf(i < UBound(a), ",", "")
    Next i
End Function
Sub Main()
    Dim a(1 To 8) As Long
    Dim i As Long
    For i = 1 To 8
        a(i) = (i * 37) Mod 11
    Next i
    Print Show(a)
    InsertionSort a
    Print Show(a)
End Sub
