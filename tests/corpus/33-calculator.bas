Option Explicit
Private src As String
Private pos As Long

Private Function Peek() As String
    If pos > Len(src) Then
        Peek = ""
    Else
        Peek = Mid$(src, pos, 1)
    End If
End Function

Private Sub SkipWs()
    Do While Peek() = " "
        pos = pos + 1
    Loop
End Sub

Private Function ParseNumber() As Double
    Dim start As Long
    SkipWs
    start = pos
    Do While Peek() Like "[0-9.]"
        pos = pos + 1
    Loop
    If start = pos Then Err.Raise vbObjectError + 1, "Calc", "number expected at " & pos
    ParseNumber = Val(Mid$(src, start, pos - start))
End Function

Private Function ParseFactor() As Double
    SkipWs
    Select Case Peek()
        Case "("
            pos = pos + 1
            ParseFactor = ParseExpr()
            SkipWs
            If Peek() <> ")" Then Err.Raise vbObjectError + 2, "Calc", "missing )"
            pos = pos + 1
        Case "-"
            pos = pos + 1
            ParseFactor = -ParseFactor()
        Case Else
            ParseFactor = ParseNumber()
    End Select
End Function

Private Function ParsePower() As Double
    Dim b As Double
    b = ParseFactor()
    SkipWs
    If Peek() = "^" Then
        pos = pos + 1
        ParsePower = b ^ ParsePower()
    Else
        ParsePower = b
    End If
End Function

Private Function ParseTerm() As Double
    Dim v As Double, op As String
    v = ParsePower()
    Do
        SkipWs
        op = Peek()
        If op <> "*" And op <> "/" Then Exit Do
        pos = pos + 1
        If op = "*" Then
            v = v * ParsePower()
        Else
            v = v / ParsePower()
        End If
    Loop
    ParseTerm = v
End Function

Private Function ParseExpr() As Double
    Dim v As Double, op As String
    v = ParseTerm()
    Do
        SkipWs
        op = Peek()
        If op <> "+" And op <> "-" Then Exit Do
        pos = pos + 1
        If op = "+" Then v = v + ParseTerm() Else v = v - ParseTerm()
    Loop
    ParseExpr = v
End Function

Function Evaluate(ByVal s As String) As Double
    src = s
    pos = 1
    Evaluate = ParseExpr()
    SkipWs
    If pos <= Len(src) Then Err.Raise vbObjectError + 3, "Calc", "unexpected '" & Peek() & "'"
End Function

Sub Main()
    Dim tests As Variant, t As Variant
    tests = Array("1 + 2 * 3", "(1 + 2) * 3", "2 ^ 3 ^ 2", "-4 + 10 / 4", "2 * (3 + ", "7 7", "1 / 0", "((2))")
    For Each t In tests
        On Error Resume Next
        Err.Clear
        Dim r As Double
        r = Evaluate(CStr(t))
        If Err.Number <> 0 Then
            Print t & " => error " & (Err.Number - vbObjectError) & ": " & Err.Description
        Else
            Print t & " = " & r
        End If
        On Error GoTo 0
    Next
End Sub
