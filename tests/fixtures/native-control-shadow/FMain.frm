VERSION 5.00
Begin VB.Form FMain
   Caption         =   "Main"
   ClientHeight    =   1000
   ClientWidth     =   2000
   Begin VB.Label vbRed
      Caption         =   "control"
      Height          =   300
      Left            =   120
      Top             =   120
      Width           =   900
   End
End
Attribute VB_Name = "FMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

' A control of the form shadows a constant of the language of the same name.
Public Sub Run()
    Print "caption " & vbRed.Caption
End Sub
