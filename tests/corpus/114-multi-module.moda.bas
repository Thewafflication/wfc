Attribute VB_Name = "ModA"
Option Explicit
Private Declare Function GetTickCount Lib "kernel32" () As Long
Public Const Greeting As String = "hi"
Public Counter As Long
Public GlobalTag As String
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
Public Function Shared1() As String
    Shared1 = "A.Shared1"
End Function
Public Function Dup2() As String
    Dup2 = "a2"
End Function
