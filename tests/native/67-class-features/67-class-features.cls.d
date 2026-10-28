Private store(1 To 10) As Variant
Public Property Get Item(i As Long) As Variant
Attribute Item.VB_UserMemId = 0
    Item = store(i)
End Property
Public Property Let Item(i As Long, v As Variant)
    store(i) = v
End Property
