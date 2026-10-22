Public Width As Long
Public Height As Long
Public Title As String
Public Inner As Box

Public Function Area() As Long
    Area = Width * Height
End Function

Public Sub Grow(ByVal by As Long)
    Width = Width + by
    Height = Height + by
End Sub
