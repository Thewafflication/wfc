VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Add ActiveX"
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Dim c As Object
    Set c = Controls.Add("MSXML2.DOMDocument.6.0", "Doc2")
    c.Left = 700
    c.async = False
    Debug.Print c.loadXML("<a><b/></a>"); c.documentElement.nodeName; c.Left; c.Visible
    Debug.Print Controls.Count; Controls("Doc2").Name
    Dim v As VBControlExtender
    Set v = Controls.Add("MSXML2.DOMDocument.6.0", "Doc3")
    v.Left = 5
    Debug.Print v.Left; v.Name; v.Object.async
    Controls.Remove "Doc3"
    On Error Resume Next
    Controls.Add "No.Such.Control", "X"
    Debug.Print Err.Number
    Err.Clear
    Controls.Remove "Doc2"
    Debug.Print Controls.Count
End Sub
