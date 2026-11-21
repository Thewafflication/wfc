Option Explicit
Sub Main()
    Dim rs As Object, i As Long
    Set rs = CreateObject("ADODB.Recordset")
    Print rs.State
    rs.Fields.Append "id", 3
    rs.Fields.Append "name", 200, 20
    rs.Fields.Append "ok", 11
    rs.Open
    Print rs.State; rs.Fields.Count; rs.BOF; rs.EOF
    For i = 1 To 5
        rs.AddNew
        rs("id") = i
        rs("name") = "row" & i
        rs("ok") = (i Mod 2 = 0)
        rs.Update
    Next
    Print rs.RecordCount
    rs.MoveFirst
    rs("name") = "first"
    rs.MoveNext
    rs.MoveFirst
    Print rs("name"); rs("ok"); TypeName(rs("ok").Value)
    rs.Filter = "id >= 2 AND id < 5"
    Print rs.RecordCount; rs("id")
    rs.Filter = ""
    rs.Find "name = 'row4'"
    Print rs.EOF; rs("id")
    rs.Find "name = 'nothing'"
    Print rs.EOF
    rs.MoveFirst
    rs.Delete
    Print rs.RecordCount; rs("id")
    rs.AddNew
    rs("id") = 99
    rs.CancelUpdate
    Print rs.RecordCount
    On Error Resume Next
    rs.Fields.Append "late", 3
    Print Err.Number <> 0
    Err.Clear
    Dim cn As Object
    Set cn = CreateObject("ADODB.Connection")
    cn.Open ":memory:"
    Set rs = cn.Execute("select 1 as one")
    rs.AddNew
    Print Err.Number
    rs.Close
End Sub
