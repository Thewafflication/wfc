Option Explicit
Sub Main()
    Dim rs As Object, i As Long, d As Variant
    Set rs = CreateObject("ADODB.Recordset")
    rs.Fields.Append "id", 3
    rs.Fields.Append "d", 7
    rs.Open
    d = Array(#1/1/2020#, #6/15/2020#, #6/15/2020 6:00:00 PM#)
    For i = 0 To 2
        rs.AddNew
        rs("id") = i + 1
        rs("d") = d(i)
        rs.Update
    Next
    rs.Filter = "d > #2020-01-01#"
    Print rs.RecordCount; rs("id")
    rs.Filter = "d = #6/15/2020 6:00 PM#"
    Print rs.RecordCount; rs("id")
    rs.Filter = ""
    rs.Find "d >= #6/15/2020#"
    Print rs("id"); TypeName(rs("d").Value)
End Sub
