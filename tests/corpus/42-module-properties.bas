Option Explicit
Private mCount As Long
Private mName As String
Private mObj As Object
Public Property Get Count() As Long
    Count = mCount
End Property
Public Property Let Count(ByVal v As Long)
    If v < 0 Then Exit Property
    mCount = v
End Property
Property Get Name() As String
    Name = mName
End Property
Property Let Name(s As String)
    mName = UCase$(s)
End Property
Property Get Item(i As Long) As String
    Item = "item" & i
End Property
Sub Main()
    Count = 5
    Print Count, Count + 1
    Count = -3
    Print Count
    Name = "bob"
    Print Name, Item(4)
End Sub
