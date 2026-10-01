Option Explicit
Function Rev(s As String) As String
    Dim i As Long
    For i = Len(s) To 1 Step -1
        Rev = Rev & Mid$(s, i, 1)
    Next i
End Function
Function IsPal(s As String) As Boolean
    Dim t As String
    t = LCase$(Replace(s, " ", ""))
    IsPal = (t = Rev(t))
End Function
Sub Main()
    Dim words() As String
    words = Split("the quick brown fox jumps", " ")
    Print UBound(words) + 1 & " words"
    Print Rev("Hello") & " " & UCase$(Left$("world", 3)) & " " & Len("abc")
    Print IsPal("A man a plan a canal Panama")
    Print IsPal("not one")
    Print Join(words, "-")
    Print InStr("hello world", "o") & " " & InStrRev("hello world", "o") & " " & Mid$("hello", 2, 3)
    Print Trim$("  pad  ") & "|" & String$(3, "x") & "|" & Space$(2) & "|" & Format$(7, "000")
    Print StrComp("a", "B", vbTextCompare) & " " & StrComp("a", "B", vbBinaryCompare)
End Sub
