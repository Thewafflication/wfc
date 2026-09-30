VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "Greeter"
Attribute VB_GlobalNameSpace = False
Option Explicit
Public Name As String

Public Function Greet() As String
    Greet = "Hello, " & Name
End Function
