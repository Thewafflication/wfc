Option Explicit
Sub Main()
    Dim rs As Object, rows As Variant, i As Long
    Set rs = CreateObject("ADODB.Recordset")
    rs.Fields.Append "id", 3
    rs.Fields.Append "name", 200, 50
    rs.Fields.Append "price", 6
    rs.Fields.Append "when", 7
    rs.Open
    For i = 1 To 3
        rs.AddNew
        rs("id") = i
        rs!name = "item" & i
        rs("price") = i * 1.25
        rs("when") = DateSerial(2024, i, 1)
        rs.Update
    Next
    rs.MoveFirst
    Do While Not rs.EOF
        Print rs!id; rs("name").Value; TypeName(rs("price").Value); Month(rs.Fields("when").Value)
        rs.MoveNext
    Loop
    rs.MoveFirst
    rows = rs.GetRows()
    Print UBound(rows, 1); UBound(rows, 2); rows(1, 2); rows(2, 0)
    rs.Filter = "id > 1"
    Print rs.RecordCount
    rs.Close
    On Error Resume Next
    rs.MoveNext
    Print Err.Number <> 0
End Sub
