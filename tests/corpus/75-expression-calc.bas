Option Explicit
Private src As String
Private pos As Long

Private Function Peek() As String
    If pos > Len(src) Then Peek = "": Else Peek = Mid$(src, pos, 1)
End Function

Private Sub SkipWs()
    Do While Peek() = " ": pos = pos + 1: Loop
End Sub

Private Function ParseNumber() As Double
    Dim start As Long
    start = pos
    Do While InStr("0123456789.", Peek()) > 0 And Peek() <> ""
        pos = pos + 1
    Loop
    If start = pos Then Err.Raise 1001, "Calc", "Number expected at " & pos
    ParseNumber = Val(Mid$(src, start, pos - start))
End Function

Private Function ParseFactor() As Double
    SkipWs
    Dim c As String: c = Peek()
    Select Case c
        Case "("
            pos = pos + 1
            ParseFactor = ParseExpr()
            SkipWs
            If Peek() <> ")" Then Err.Raise 1002, "Calc", "Missing )"
            pos = pos + 1
        Case "-"
            pos = pos + 1
            ParseFactor = -ParseFactor()
        Case Else
            ParseFactor = ParseNumber()
    End Select
End Function

Private Function ParsePower() As Double
    Dim b As Double: b = ParseFactor()
    SkipWs
    If Peek() = "^" Then
        pos = pos + 1
        ParsePower = b ^ ParsePower()
    Else
        ParsePower = b
    End If
End Function

Private Function ParseTerm() As Double
    Dim v As Double: v = ParsePower()
    Do
        SkipWs
        Select Case Peek()
            Case "*": pos = pos + 1: v = v * ParsePower()
            Case "/"
                pos = pos + 1
                Dim d As Double: d = ParsePower()
                If d = 0 Then Err.Raise 11
                v = v / d
            Case Else: Exit Do
        End Select
    Loop
    ParseTerm = v
End Function

Private Function ParseExpr() As Double
    Dim v As Double: v = ParseTerm()
    Do
        SkipWs
        Select Case Peek()
            Case "+": pos = pos + 1: v = v + ParseTerm()
            Case "-": pos = pos + 1: v = v - ParseTerm()
            Case Else: Exit Do
        End Select
    Loop
    ParseExpr = v
End Function

Function Calc(ByVal expr As String) As String
    On Error GoTo fail
    src = expr: pos = 1
    Dim r As Double: r = ParseExpr()
    SkipWs
    If pos <= Len(src) Then Err.Raise 1003, "Calc", "Unexpected '" & Peek() & "' at " & pos
    Calc = expr & " = " & r
    Exit Function
fail:
    Calc = expr & " -> error " & Err.Number & ": " & Err.Description
End Function

Sub Main()
    Dim e As Variant
    For Each e In Array("1 + 2 * 3", "(1+2)*3", "2^3^2", "-4 + 10 / 4", "1/0", "2 * (3", "7 $ 2", "1.5*4", "")
        Print Calc(CStr(e))
    Next
End Sub
