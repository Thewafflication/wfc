Private n As Long
Public Inc As Long

Private Sub Class_Initialize()
    Inc = 1
    Print "counter created"
End Sub

Private Sub Class_Terminate()
    Print "counter gone at"; n
End Sub

Public Sub Bump()
    n = n + Inc
End Sub

Public Sub Add(ByVal k As Long)
    n = n + k
End Sub

Public Property Get Value() As Long
    Value = n
End Property

Public Property Let Value(v As Long)
    n = v
End Property

Public Function Twice() As Long
    Twice = n * 2
End Function
