Attribute VB_Name = "Main"
Option Explicit

Sub Main()
    Dim i As Long
    Print Form1.Caption
    Print Form1.Text1.Text
    Form1.Text1.Text = "changed"
    Print Form1.Text1.Text; Len(Form1.Text1.Text)
    Form1.Label1.Caption = "n=" & 42
    Print Form1.Label1.Caption
    With Form1.Text1
        .Left = 500
        Print .Left, .Width
    End With
    For i = 1 To 3
        Form1.Caption = "step " & i
    Next i
    Print Form1.Caption
    Dim f As Object
    Set f = Form1
    Print f.Text1.Text
    Print f Is Form1
End Sub
