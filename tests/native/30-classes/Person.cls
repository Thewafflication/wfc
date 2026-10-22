Public Name As String
Private mAge As Long
Public Pal As Person

Public Property Get Age() As Long
    Age = mAge
End Property

Public Property Let Age(ByVal v As Long)
    If v < 0 Then Err.Raise 5, , "bad age"
    mAge = v
End Property

Public Function Describe() As String
    Describe = Name & " (" & mAge & ")"
End Function

Public Function Self() As Person
    Set Self = Me
End Function

Public Sub Greet(other As Person)
    Print Name; " greets "; other.Name
End Sub

Public Property Get HasFriend() As Boolean
    HasFriend = Not (Pal Is Nothing)
End Property
