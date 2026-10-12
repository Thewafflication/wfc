VERSION 5.00
Begin VB.Form Form1
   Caption         =   "OLE"
   Begin VB.OLE OLE1
      Height          =   900
      Width           =   1200
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Debug.Print OLE1.OLEType; OLE1.AppIsRunning
    OLE1.CreateEmbed "", "MSXML2.DOMDocument.6.0"
    Debug.Print OLE1.OLEType; OLE1.AppIsRunning
    OLE1.Object.async = False
    Debug.Print OLE1.Object.loadXML("<r/>"); OLE1.Object.documentElement.nodeName
    OLE1.DoVerb
    OLE1.Close
    Debug.Print OLE1.OLEType; OLE1.AppIsRunning
    On Error Resume Next
    OLE1.CreateEmbed "", "No.Such.Class"
    Debug.Print Err.Number
End Sub
