VERSION 1.0 CLASS
Attribute VB_Name = "Greeter"
Attribute VB_Creatable = True
Attribute VB_Exposed = True
Option Explicit
Public Event Changed(ByVal Message As String)
Public Prefix As String
Private mCount As Long
Public Property Get Count() As Long
    Count = mCount
End Property
Public Function Greet(ByVal who As String) As String
    mCount = mCount + 1
    Greet = Prefix & " " & who
    RaiseEvent Changed("greeted " & who)
End Function
Public Function Fail() As Long
    Err.Raise 1234, , "boom"
End Function
Public Function Make() As Object
    Set Make = New Greeter
    Make.Prefix = "child"
End Function
