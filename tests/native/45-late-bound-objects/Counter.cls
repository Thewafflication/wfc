VERSION 1.0 CLASS
Attribute VB_Name = "Counter"
Option Explicit
Private mTotal As Long
Private mName As String

Public Property Get Total() As Long
    Total = mTotal
End Property

Public Property Let Total(ByVal v As Long)
    mTotal = v
End Property

Public Property Get Name() As String
    Name = mName
End Property

Public Property Let Name(ByVal v As String)
    mName = v
End Property

Public Sub Add(ByVal n As Long)
    mTotal = mTotal + n
End Sub

Public Function Twice(ByVal n As Long) As Long
    Twice = n * 2
End Function

Public Sub Swap(ByRef a As Long, ByRef b As Long)
    Dim t As Long
    t = a
    a = b
    b = t
End Sub

Public Function Describe(ByVal prefix As String, Optional ByVal suffix As String = "!") As String
    Describe = prefix & mName & ":" & mTotal & suffix
End Function

Public Function Self() As Counter
    Set Self = Me
End Function
