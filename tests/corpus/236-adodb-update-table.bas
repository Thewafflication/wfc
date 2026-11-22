Option Explicit
Sub Main()
    Dim cn As Object, rs As Object, ro As Object
    Set cn = CreateObject("ADODB.Connection")
    cn.Open "Data Source=:memory:"
    cn.Execute "create table t(id integer primary key, name text not null, n integer default 5)"
    cn.Execute "insert into t(name) values ('ann')"
    cn.Execute "insert into t(name) values ('bob')"
    Set rs = CreateObject("ADODB.Recordset")
    rs.Open "select id, name from t order by id", cn, 1, 3
    rs.Fields("name").Value = "ANN"
    rs.Update
    rs.MoveNext
    rs("name") = "BOB"
    rs.MoveFirst
    rs.AddNew
    rs("name") = "cy"
    rs.Update
    Print rs("id"); rs("name")
    Set ro = cn.Execute("select id, name, n from t order by id")
    Do While Not ro.EOF
        Print ro(0); ro(1); ro(2)
        ro.MoveNext
    Loop
    rs.MoveFirst
    rs.Delete
    Set ro = cn.Execute("select count(*) from t")
    Print ro(0)
    rs.AddNew
    On Error Resume Next
    rs.Update
    Print CBool(Err.Number)
    Err.Clear
    rs.Close
    rs.Open "t", cn, 1, 3
    Print rs.RecordCount
    Set ro = cn.Execute("select id from t")
    ro.MoveFirst
    On Error Resume Next
    ro.Delete
    Print Err.Number
End Sub
