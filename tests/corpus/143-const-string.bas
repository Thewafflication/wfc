#Const Edition = "pro"
#Const Greeting = "it's ""on"""
#Const Level = 3
Sub Main()
#If Edition = "pro" Then
    Print "pro edition"
#ElseIf Edition = "lite" Then
    Print "lite edition"
#Else
    Print "other"
#End If
#If Edition <> "pro" Or Level > 5 Then
    Print "wrong"
#Else
    Print "ok"
#End If
#If Greeting = "it's ""on""" Then
    Print "quoted match"
#End If
#If "abc" < "abd" And Level = 3 Then
    Print "ordered"
#End If
End Sub
