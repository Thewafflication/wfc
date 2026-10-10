Attribute VB_Name = "ModB"
Option Explicit
Private Declare Function GetTickCount Lib "kernel32" () As Long
Private secret As String
Private Const K = 2
Public Sub Dup()
    Print "B.Dup", K, GetTickCount() >= 0
End Sub
Private Sub Helper()
    Print "B.Helper"
End Sub
Public Sub Main()
    Dim p As Pair
    p.A = 3
    ModA.Counter = 5
    Counter = Counter + 1
    Print ModA.Greeting, Greeting, ModA.Calc(10), Calc(10), Counter, ModA.High
    ModA.Dup
    ModB.Dup
    Dup
    Helper
    RunA
    secret = "bs"
    Print secret, p.A, Level.Low
End Sub
