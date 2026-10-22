Option Explicit

Enum Color
    Red
    Green = 5
    Blue
End Enum

Public Enum Flags
    None = 0
    Fast = 1
    Strong = 2
    Both = Fast + Strong
End Enum

Function Name(ByVal c As Color) As String
    Select Case c
        Case Red: Name = "red"
        Case Green: Name = "green"
        Case Blue: Name = "blue"
        Case Else: Name = "?"
    End Select
End Function

Sub Main()
    Dim c As Color, f As Flags, s As String, a As Animal, d As Dog, o As Object
    c = Blue
    Print c; Name(c); Name(Color.Green); Name(7)
    f = Both
    Print f; f And Fast; (f And Strong) <> 0; Flags.None
    s = "Hello World"
    Print s Like "Hello*"; s Like "hello*"; s Like "H?llo W*"; "abc" Like "a[b-d]c"
    Print "abc" Like "a[!b]c"; "x1" Like "x#"; "" Like "*"
    Mid$(s, 1, 5) = "JELLO"
    Print s
    Mid(s, 7) = "wo"
    Print s
    Mid$(s, 1, 2) = "ABCDEF"
    Print s
    Set a = New Animal
    Set d = New Dog
    Print TypeOf a Is Animal; TypeOf a Is Dog; TypeOf d Is Dog
    Set o = a
    Print TypeOf o Is Animal; TypeOf o Is Object
    Set o = Nothing
    Print TypeOf o Is Object
End Sub
