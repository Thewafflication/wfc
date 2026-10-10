Attribute VB_Name = "ModA"
Option Explicit
Private Declare Function GetTickCount Lib "kernel32" () As Long
Public Const Greeting As String = "hi"
Public Counter As Long
Private secret As Long
Private Const K = 1
Public Enum Level
    Low = 1
    High = 9
End Enum
Public Type Pair
    A As Long
    B As Long
End Type
Public Function Calc(x As Long) As Long
    secret = secret + 1
    Calc = x * 2 + secret
End Function
Public Sub Dup()
    Print "A.Dup", K, GetTickCount() >= 0
End Sub
Private Sub Helper()
    Print "A.Helper"
End Sub
Public Sub RunA()
    Helper
End Sub
