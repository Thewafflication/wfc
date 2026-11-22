Option Explicit
Sub Main()
    Dim rs As Object, i As Long, b2 As Variant, b4 As Variant
    Set rs = CreateObject("ADODB.Recordset")
    rs.Fields.Append "id", 3
    rs.Open
    For i = 1 To 4
        rs.AddNew
        rs("id") = i * 10
        rs.Update
        If i = 2 Then b2 = rs.Bookmark
        If i = 4 Then b4 = rs.Bookmark
    Next
    rs.MoveFirst
    rs.Bookmark = b4
    Print rs("id")
    rs.Sort = "id DESC"
    rs.Bookmark = b2
    Print rs("id"); rs.AbsolutePosition
    rs.MoveFirst
    rs.Delete
    rs.Bookmark = b2
    Print rs("id"); rs.RecordCount
    rs.Filter = "id > 20"
    On Error Resume Next
    rs.Bookmark = b2
    Print Err.Number
End Sub
