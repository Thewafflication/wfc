Option Explicit
Sub Main()
    Dim rs As Object, i As Long, n As Variant, s As Variant
    Set rs = CreateObject("ADODB.Recordset")
    rs.Fields.Append "id", 3
    rs.Fields.Append "name", 200, 20
    rs.Open
    n = Array("mia", "Ann", "bob", "ann", "zed")
    For i = 0 To 4
        rs.AddNew
        rs("id") = i + 1
        rs("name") = n(i)
        rs.Update
    Next
    rs.Sort = "name ASC, id DESC"
    Print "[" & rs.Sort & "]"
    Do While Not rs.EOF
        Print rs("id"); rs("name")
        rs.MoveNext
    Loop
    rs.Sort = "id DESC"
    rs.MoveFirst
    Print rs("id")
    rs.Filter = "id < 4"
    Print rs.RecordCount; rs("id")
    rs.Sort = ""
    rs.MoveFirst
    Print rs("id")
    On Error Resume Next
    rs.Sort = "nosuch"
    Print Err.Number
End Sub
