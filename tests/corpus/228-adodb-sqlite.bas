Option Explicit
Sub Main()
    Dim cn As Object, rs As Object, cmd As Object, n As Long
    Set cn = CreateObject("ADODB.Connection")
    Print cn.State
    cn.Open "Data Source=:memory:"
    Print cn.State
    cn.Execute "create table t(id integer primary key, name text, score real)"
    cn.Execute "insert into t(name, score) values ('ann', 1.5)", n
    Print "affected"; n
    Set cmd = CreateObject("ADODB.Command")
    Set cmd.ActiveConnection = cn
    cmd.CommandText = "insert into t(name, score) values (?, ?)"
    cmd.Execute , Array("bob", 2.25)
    cmd.Execute , Array("cy", Null)
    Set rs = CreateObject("ADODB.Recordset")
    rs.Open "select id, name, score from t order by id", cn
    Print rs.State; rs.RecordCount; rs.Fields.Count
    Do While Not rs.EOF
        Print rs("id"); rs!name; rs.Fields("score").Value; TypeName(rs("score").Value)
        rs.MoveNext
    Loop
    Print rs.EOF; rs.BOF; rs.AbsolutePosition
    rs.MoveFirst
    Print rs.Fields(1).Name; rs.AbsolutePosition
    rs.Close
    Print rs.State
    cn.BeginTrans
    cn.Execute "delete from t"
    cn.RollbackTrans
    Set rs = cn.Execute("select count(*) as n from t")
    Print rs("n")
    On Error Resume Next
    cn.Execute "selec bad"
    Print Err.Number
    Err.Clear
    cn.Close
    Print cn.State
End Sub
