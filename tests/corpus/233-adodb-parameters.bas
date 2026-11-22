Option Explicit
Sub Main()
    Dim cn As Object, cmd As Object, rs As Object, p As Object
    Set cn = CreateObject("ADODB.Connection")
    cn.Open "Data Source=:memory:"
    cn.Execute "create table t(id integer, name text)"
    Set cmd = CreateObject("ADODB.Command")
    Set cmd.ActiveConnection = cn
    cmd.CommandText = "insert into t values (?, ?)"
    cmd.Parameters.Append cmd.CreateParameter("id", 3, 1, 0, 7)
    cmd.Parameters.Append cmd.CreateParameter("name", 200, 1, 20, "ann")
    Print cmd.Parameters.Count
    cmd.Execute
    cmd.Parameters("id").Value = 8
    cmd.Parameters(1).Value = "bob"
    cmd.Execute
    cmd.CommandText = "select id, name from t where id >= ? order by id"
    cmd.Parameters.Delete 1
    cmd.Parameters.Delete "id"
    Set p = cmd.CreateParameter("min", 3, 1, 0, 8)
    cmd.Parameters.Append p
    Set rs = cmd.Execute
    Print rs.RecordCount; rs("name"); p.Type; p.Direction
    For Each p In cmd.Parameters
        Print p.Name; p.Value
    Next
End Sub
