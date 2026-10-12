VERSION 5.00
Begin VB.UserControl Gauge
   ClientHeight    =   1200
   ClientWidth     =   2400
   Begin VB.TextBox Text1
      Height          =   300
      Left            =   100
      Top             =   100
      Width           =   1500
   End
End
Attribute VB_Name = "Gauge"
Option Explicit
Public Event Changed(ByVal NewValue As String)
Private mCount As Long
Public Property Get Caption() As String
    Caption = Text1.Text
End Property
Public Property Let Caption(ByVal v As String)
    Text1.Text = v
End Property
Public Sub Bump()
    mCount = mCount + 1
    RaiseEvent Changed("bump" & mCount)
End Sub
Public Function Twice(ByVal n As Long) As Long
    Twice = n * 2
End Function
Private Sub UserControl_Initialize()
    Debug.Print "uc initialize"
End Sub
Private Sub UserControl_ReadProperties(PropBag As PropertyBag)
    Text1.Text = PropBag.ReadProperty("Caption", "none")
    Debug.Print "uc read "; Text1.Text
End Sub
Private Sub UserControl_Resize()
    Debug.Print "uc resize"; Width; Height
End Sub
Private Sub Text1_Change()
    RaiseEvent Changed(Text1.Text)
End Sub
Private Sub UserControl_Terminate()
    Debug.Print "uc terminate"
End Sub
