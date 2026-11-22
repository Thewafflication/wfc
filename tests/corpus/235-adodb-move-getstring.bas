Option Explicit
Sub Main()
    Dim rs As Object, i As Long
    Set rs = CreateObject("ADODB.Recordset")
    rs.Fields.Append "id", 3
    rs.Fields.Append "name", 200, 10
    rs.Open
    For i = 1 To 4
        rs.AddNew
        rs("id") = i
        If i <> 3 Then rs("name") = "n" & i
        rs.Update
    Next
    rs.MoveFirst
    rs.Move 2
    Print rs("id")
    rs.Move -1
    Print rs("id")
    rs.Move 0, 2
    Print rs("id")
    rs.AbsolutePosition = 3
    Print rs("id"); rs.AbsolutePosition
    rs.MoveFirst
    Print Replace(rs.GetString(2, 3, ",", ";", "?"), ";", "|")
    Print rs.EOF; rs("id")
    Print rs.Supports(&H400000)
End Sub
