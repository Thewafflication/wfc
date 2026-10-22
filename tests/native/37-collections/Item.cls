Public Name As String
Public Qty As Long

Private Sub Class_Terminate()
    Print "bye "; Name
End Sub

Public Function Label() As String
    Label = Name & "x" & Qty
End Function
