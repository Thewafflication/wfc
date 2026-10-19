VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   3000
   ClientWidth     =   4000
   LinkTopic       =   "Form1"
   ScaleHeight     =   3000
   ScaleWidth      =   4000
   Begin VB.Data Data1 
      Caption         =   "Data1"
      Connect         =   "Access"
      DatabaseName    =   ""
      Height          =   345
      Left            =   240
      RecordSource    =   ""
      Top             =   240
      Width           =   1500
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Command1"
      Height          =   375
      Left            =   240
      Top             =   1200
      Width           =   1200
   End
End
Attribute VB_Name = "Form1"
Private Sub Form_Load()
    On Error Resume Next
    Form1.LinkExecute "x"
    Print "dde", Err.Number, Err.Description
    Err.Clear
    Data1.Refresh
    Print "data", Err.Number, Err.Description
    Err.Clear
    Form1.LinkMode = 1
    Print "linkmode", Err.Number, Err.Description
End Sub
