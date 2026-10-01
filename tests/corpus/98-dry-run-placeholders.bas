Option Explicit
Sub Foo(a)
End Sub
Sub Main()
    Dim cur As Variant, o As Object, col As New Collection, d As Object
    Dim arr2(5) As Long, v As Variant, i As Long, s As String, z() As Long
    cur = Array(0, 1)
    If False Then
        Print cur(0) + 1, cur(0)(1), o.Prop & "a", o.Method(1) * 2, col(1) = 1, d("k") Like "a*"
        Print Left$(cur(0), 2), Mid$("abc", col(1)), Chr(d("k")), Switch(cur(0), 1), Choose(o.Prop, 1, 2)
        arr2(cur(0)) = 5
        For i = cur(0) To cur(0)(1): Next
        Select Case col(1)
            Case 1: Print "one"
        End Select
        ReDim z(d("k"))
        Do While cur(0)(1) < 3: Loop
        Mid$(s, cur(0), 1) = "a"
        With col(1): End With
        Foo cur(0)(1)
        If col.Item(1).Name Then v = 1
    End If
    With New Collection: .Add 1: Print .Count: End With
    Select Case 2: Case 2: Print "two": End Select
    Print "done"
End Sub
