VERSION 5.00
Object = "{F5078F18-C551-11D3-89B9-0000F81FE221}#6.0#0"; "msxml6.dll"
Begin VB.Form Form1
   Caption         =   "ActiveX"
   Begin MSXML2.DOMDocument60 Doc1
      Height          =   600
      Left            =   120
      Top             =   240
      Width           =   1200
      Visible         =   0   'False
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Dim States As String
Private Sub Form_Activate()
    Dim p As String, f As Integer, n As Long
    Debug.Print Doc1.Left; Doc1.Top; Doc1.Width; Doc1.Height; Doc1.Visible; Doc1.Name
    Doc1.Left = 500
    Debug.Print Doc1.Left; TypeName(Doc1.Object) <> ""
    p = Environ("TEMP") & Chr(92) & "wfc_form_activex.xml"
    f = FreeFile
    Open p For Output As #f
    Print #f, "<root><a/></root>"
    Close #f
    Doc1.async = True
    Doc1.Load p
    Do While Doc1.readyState <> 4 And n < 200000
        DoEvents
        n = n + 1
    Loop
    Debug.Print Doc1.documentElement.nodeName; Right$(States, 1); Len(States) > 0
    Kill p
End Sub
Private Sub Doc1_onreadystatechange()
    States = States & Doc1.readyState
End Sub
