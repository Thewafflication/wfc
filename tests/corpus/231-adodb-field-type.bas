Option Explicit
Sub Main()
    Dim cn As Object, rs As Object, f As Object
    Set cn = CreateObject("ADODB.Connection")
    cn.Open ":memory:"
    cn.Execute "create table t(a integer, b text, c real, d blob)"
    cn.Execute "insert into t values (1, 'x', 2.5, null)"
    Set rs = cn.Execute("select a, b, c, d from t")
    For Each f In rs.Fields
        Print f.Name; f.Type
    Next
    Set rs = CreateObject("ADODB.Recordset")
    rs.Fields.Append "n", 3
    rs.Fields.Append "s", 200, 20
    Print rs.Fields("n").Type; rs.Fields("s").Type; rs.Fields("s").DefinedSize
End Sub
