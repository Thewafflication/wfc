Option Explicit

Function MakePerson(ByVal n As String, ByVal a As Long) As Person
    Dim p As New Person
    p.Name = n
    p.Age = a
    Set MakePerson = p
End Function

Sub Rename(ByRef p As Person, ByVal n As String)
    p.Name = n
End Sub

Sub Main()
    Dim c As New Counter
    c.Bump
    c.Add 5
    Print c.Value
    c.Value = 40
    c.Inc = 3
    c.Bump
    Print c.Value; c.Twice()
    Dim a As Person, b As Person
    Set a = MakePerson("Ann", 31)
    Set b = New Person
    b.Name = "Bob"
    b.Age = 27
    Print a.Describe(); b.Describe()
    a.Greet b
    Set a.Pal = b
    Print a.HasFriend; b.HasFriend
    Print a.Pal.Name
    Rename b, "Robert"
    Print a.Pal.Describe()
    Print a Is b; a Is a; a.Self() Is a
    Print a.Self().Name
    Dim n As Person
    Print n Is Nothing
    Set n = a
    Print n Is a, n Is Nothing
    Set n = Nothing
    Print n Is Nothing
    On Error Resume Next
    b.Age = -1
    Print Err.Number; Err.Description
    Err.Clear
    Print n.Name
    Print Err.Number
    Set c = Nothing
    Print "end"
End Sub
