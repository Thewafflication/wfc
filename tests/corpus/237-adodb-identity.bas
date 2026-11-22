Option Explicit
Sub Main()
    Dim cn As Object, rs As Object
    Set cn = CreateObject("ADODB.Connection")
    cn.Open "Data Source=:memory:"
    cn.Execute "create table t(id integer primary key, v text)"
    cn.Execute "insert into t(v) values ('a')"
    cn.Execute "insert into t(v) values ('b')"
    Set rs = cn.Execute("select @@identity")
    Print rs(0)
    cn.Execute "insert into t(v) values ('c')"
    Set rs = cn.Execute("SELECT @@IDENTITY;")
    Print rs(0); rs.RecordCount
End Sub
