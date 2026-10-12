VERSION 5.00
Object = "{EAB22AC0-30C1-11CF-A7EB-0000C05BAE0B}#1.1#0"; "ieframe.dll"
Begin VB.Form frmActiveX
   Caption         =   "ActiveX control host"
   ClientHeight    =   3000
   ClientWidth     =   4800
   Begin VB.Timer Timer1
      Interval        =   4000
   End
   Begin SHDocVw.WebBrowser Web1
      Height          =   2400
      Left            =   120
      Top             =   480
      Width           =   4500
   End
   Begin VB.Label Label1
      Caption         =   "Hosted WebBrowser control:"
      Height          =   300
      Left            =   120
      Top             =   100
      Width           =   3000
   End
End
Attribute VB_Name = "frmActiveX"
Option Explicit
Dim done As Boolean
Private Sub Form_Activate()
    Web1.Navigate "about:blank"
    Debug.Print Web1.Visible; Web1.Width; Web1.Left
End Sub
Private Sub Web1_DocumentComplete(ByVal pDisp As Object, URL As Variant)
    If done Then Exit Sub
    done = True
    Debug.Print "document complete "; URL
    Unload Me
End Sub
Private Sub Timer1_Timer()
    Debug.Print "timeout"
    Unload Me
End Sub
