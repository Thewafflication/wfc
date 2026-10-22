Public Name As String
Private Sub Class_Initialize()
    Print "init"
    Name = "n"
End Sub
Private Sub Class_Terminate()
    Print "term"
End Sub
Public Sub Hello()
    Print "hello " & Name
End Sub
