Dim greeting As String

Function Shout(ByVal s As String) As String
    Shout = UCase(s) & "!"
End Function

Function Repeat(ByVal s As String, ByVal n As Long) As String
    Dim result As String
    Dim i As Long
    For i = 1 To n
        result = result & s
    Next i
    Repeat = result
End Function

Sub Append(ByRef target As String, ByVal text As String)
    target = target & text
End Sub

Function Reverse(ByVal s As String) As String
    Dim i As Long, r As String
    For i = Len(s) To 1 Step -1
        r = r & Mid(s, i, 1)
    Next i
    Reverse = r
End Function

Sub Main()
    Dim a As String, b As String, c As String
    Dim n As Long
    a = "Hello"
    b = ", world"
    c = a & b
    Print c
    Print Len(c); Len(""); Len(a)
    Print Left(c, 3); "|"; Right(c, 5); "|"; Mid(c, 4, 3); "|"; Mid(c, 8)
    Print UCase(c); "|"; LCase(c)
    Print Shout("hey")
    Print Repeat("ab", 4)
    greeting = "hi"
    Append greeting, " there"
    Append greeting, "!"
    Print greeting
    Print Reverse("stressed")
    Print Chr(65); Chr(97); Asc("A"); Asc("z")
    Print Str(42); Str(-7); CStr(42); CStr(True)
    Print "[" & Trim("  padded  ") & "]"; "[" & LTrim("  x ") & "]"; "[" & RTrim(" x  ") & "]"
    Print InStr("hello world", "o"); InStr(6, "hello world", "o"); InStr("abc", "z")
    Print "[" & Space(3) & "]"
    Print a = "Hello"; a <> "Hello"; a < "Help"; a > "Hello"; "b" > "a"
    Print "n=" & 5 & " ok=" & True
    If a = "Hello" And Len(b) = 7 Then Print "match"
    c = ""
    Print Len(c); c = ""
    c = "x"
    c = c & c & c
    Print c
End Sub
