VERSION 5.00
Object = "{F5078F18-C551-11D3-89B9-0000F81FE221}#6.0#0"; "msxml6.dll"
Begin VB.Form Form1
   Caption         =   "ActiveX array"
   Begin MSXML2.DOMDocument60 Doc1
      Index           =   0
      Left            =   10
   End
   Begin MSXML2.DOMDocument60 Doc1
      Index           =   1
      Left            =   20
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Load Doc1(2)
    Doc1(2).Left = 30
    Dim i As Long
    For i = 0 To 2
        Doc1(i).async = False
        Doc1(i).loadXML "<n" & i & "/>"
        Debug.Print i; Doc1(i).Left; Doc1(i).documentElement.nodeName
    Next
    Debug.Print Doc1.Count
    Unload Doc1(2)
    Debug.Print Doc1.Count
End Sub
