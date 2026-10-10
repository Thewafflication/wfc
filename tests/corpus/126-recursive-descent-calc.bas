Option Explicit
Dim src As String, pos As Long, tok As String, tokType As String

Sub NextTok()
    Dim c As String
    Do While pos <= Len(src) And Mid$(src, pos, 1) = " ": pos = pos + 1: Loop
    If pos > Len(src) Then tok = "": tokType = "end": Exit Sub
    c = Mid$(src, pos, 1)
    Select Case True
        Case c Like "[0-9.]"
            tok = ""
            Do While pos <= Len(src)
                c = Mid$(src, pos, 1)
                If Not (c Like "[0-9.]") Then Exit Do
                tok = tok & c: pos = pos + 1
            Loop
            tokType = "num"
        Case c Like "[A-Za-z]"
            tok = ""
            Do While pos <= Len(src)
                c = Mid$(src, pos, 1)
                If Not (c Like "[A-Za-z0-9]") Then Exit Do
                tok = tok & c: pos = pos + 1
            Loop
            tokType = "id"
        Case Else
            tok = c: pos = pos + 1: tokType = "op"
    End Select
End Sub

Function Expr() As Double
    Dim v As Double
    v = Term()
    Do While tokType = "op" And (tok = "+" Or tok = "-")
        If tok = "+" Then
            NextTok: v = v + Term()
        Else
            NextTok: v = v - Term()
        End If
    Loop
    Expr = v
End Function

Function Term() As Double
    Dim v As Double, d As Double
    v = Power()
    Do While tokType = "op" And (tok = "*" Or tok = "/" Or tok = "%")
        Select Case tok
            Case "*": NextTok: v = v * Power()
            Case "/"
                NextTok: d = Power()
                If d = 0 Then Err.Raise 11
                v = v / d
            Case "%": NextTok: v = v - Int(v / Power()) * 1
        End Select
    Loop
    Term = v
End Function

Function Power() As Double
    Dim b As Double
    b = Unary()
    If tokType = "op" And tok = "^" Then
        NextTok
        Power = b ^ Power()
    Else
        Power = b
    End If
End Function

Function Unary() As Double
    If tokType = "op" And tok = "-" Then
        NextTok: Unary = -Unary()
    Else
        Unary = Primary()
    End If
End Function

Function Primary() As Double
    Dim name As String, a As Double
    Select Case tokType
        Case "num"
            Primary = Val(tok): NextTok
        Case "id"
            name = LCase$(tok): NextTok
            If tokType = "op" And tok = "(" Then
                NextTok: a = Expr()
                If Not (tokType = "op" And tok = ")") Then Err.Raise 5, , "expected )"
                NextTok
                Select Case name
                    Case "sqrt": Primary = Sqr(a)
                    Case "abs": Primary = Abs(a)
                    Case "sin": Primary = Sin(a)
                    Case "ln": Primary = Log(a)
                    Case Else: Err.Raise 5, , "unknown function " & name
                End Select
            ElseIf name = "pi" Then
                Primary = 4 * Atn(1)
            Else
                Err.Raise 5, , "unknown name " & name
            End If
        Case "op"
            If tok = "(" Then
                NextTok: Primary = Expr()
                If Not (tokType = "op" And tok = ")") Then Err.Raise 5, , "expected )"
                NextTok
            Else
                Err.Raise 5, , "unexpected " & tok
            End If
        Case Else
            Err.Raise 5, , "unexpected end"
    End Select
End Function

Function Evaluate(s As String) As String
    On Error GoTo bad
    src = s: pos = 1: NextTok
    Dim v As Double
    v = Expr()
    If tokType <> "end" Then Err.Raise 5, , "trailing " & tok
    Evaluate = CStr(v)
    Exit Function
bad:
    Evaluate = "error(" & Err.Number & "): " & Err.Description
End Function

Sub Main()
    Dim tests As Variant, t As Variant
    tests = Array("1 + 2 * 3", "(1 + 2) * 3", "2 ^ 3 ^ 2", "-2 ^ 2", "10 / 4", "sqrt(16) + abs(-3)", "pi * 2", "1 / 0", "2 +", "foo(1)", "3 4", "((2))", "1.5 * 4", "ln(1) + sin(0)")
    For Each t In tests
        Print t & " = " & Evaluate(CStr(t))
    Next
End Sub
