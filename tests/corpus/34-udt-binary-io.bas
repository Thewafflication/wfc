Option Explicit
Private Type Item
    Name As String * 12
    Qty As Long
    Price As Currency
End Type

Dim items() As Item
Dim count As Long

Sub AddItem(n As String, q As Long, p As Currency)
    count = count + 1
    ReDim Preserve items(1 To count)
    items(count).Name = n
    items(count).Qty = q
    items(count).Price = p
End Sub

Function Total() As Currency
    Dim i As Long
    For i = 1 To count
        Total = Total + items(i).Qty * items(i).Price
    Next i
End Function

Sub SortByName()
    Dim i As Long, j As Long, t As Item
    For i = 1 To count - 1
        For j = 1 To count - i
            If items(j).Name > items(j + 1).Name Then
                t = items(j): items(j) = items(j + 1): items(j + 1) = t
            End If
        Next j
    Next i
End Sub

Sub Main()
    AddItem "widget", 10, 2.5
    AddItem "gadget", 3, 19.99
    AddItem "bolt", 100, 0.05
    SortByName
    Dim i As Long
    For i = 1 To count
        Print RTrim$(items(i).Name); Tab(14); Format(items(i).Qty, "000"); Tab(20); Format(items(i).Price, "0.00")
    Next
    Print "Total: "; Format(Total(), "Currency")
    Dim f As Integer: f = FreeFile
    Open "inv.dat" For Binary As #f
    Put #f, , items(1)
    Close #f
    Dim chk As Item
    Open "inv.dat" For Binary As #f
    Get #f, , chk
    Close #f
    Kill "inv.dat"
    Print Len(chk), RTrim$(chk.Name), chk.Qty, chk.Price
End Sub
