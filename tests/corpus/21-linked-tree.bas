Option Explicit
Sub Main()
    Dim head As Node, cur As Node, n As Node, i As Long
    For i = 3 To 1 Step -1
        Set n = New Node
        n.Value = i * 10
        Set n.NextNode = head
        Set head = n
    Next i
    Set cur = head
    Do While Not cur Is Nothing
        Print cur.Value;
        Print " ";
        Set cur = cur.NextNode
    Loop
    Print
    Dim root As TreeNode
    Set root = Insert(root, 50)
    Set root = Insert(root, 30)
    Set root = Insert(root, 70)
    Set root = Insert(root, 20)
    Set root = Insert(root, 40)
    Print InOrder(root) & "| height " & Height(root)
End Sub
Function Insert(t As TreeNode, v As Long) As TreeNode
    If t Is Nothing Then
        Set Insert = New TreeNode
        Insert.Key = v
    Else
        If v < t.Key Then
            Set t.Left = Insert(t.Left, v)
        Else
            Set t.Right = Insert(t.Right, v)
        End If
        Set Insert = t
    End If
End Function
Function InOrder(t As TreeNode) As String
    If t Is Nothing Then Exit Function
    InOrder = InOrder(t.Left) & t.Key & " " & InOrder(t.Right)
End Function
Function Height(t As TreeNode) As Long
    Dim l As Long, r As Long
    If t Is Nothing Then Exit Function
    l = Height(t.Left): r = Height(t.Right)
    Height = 1 + IIf(l > r, l, r)
End Function
