VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Tabs and status"
   Begin MSComctlLib.TabStrip Tabs1
      Height          =   2000
      Left            =   120
      Top             =   120
      Width           =   4000
   End
   Begin MSComctlLib.StatusBar Status1
      Height          =   300
      Left            =   0
      Top             =   2700
      Width           =   4800
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Dim t As Tab, p As Panel
    Set t = Tabs1.Tabs.Add(, "one", "First")
    Tabs1.Tabs.Add , "two", "Second"
    Tabs1.Tabs.Add 1, "zero", "Zero"
    Debug.Print Tabs1.Tabs.Count; Tabs1.Tabs(1).Key; Tabs1.Tabs("one").Index; t.Caption; TypeName(t)
    Debug.Print Tabs1.SelectedItem.Key; Tabs1.Tabs(1).Selected
    Debug.Print Tabs1.Tabs(1).Left; Tabs1.Tabs(2).Left > Tabs1.Tabs(1).Left; Tabs1.ClientTop; Tabs1.ClientLeft
    Debug.Print Tabs1.HitTest(10, 10).Key; (Tabs1.HitTest(3999, 1900) Is Nothing)
    Set Tabs1.SelectedItem = Tabs1.Tabs("two")
    Debug.Print Tabs1.SelectedItem.Index; Tabs1.Tabs("two").Selected
    Tabs1.Tabs.Remove "two"
    Debug.Print Tabs1.Tabs.Count; Tabs1.SelectedItem.Key
    On Error Resume Next
    Tabs1.Tabs.Add , "one", "Dup"
    Debug.Print Err.Number
    Err.Clear
    Debug.Print Tabs1.Tabs(9).Key
    Debug.Print Err.Number
    On Error GoTo 0
    Tabs1.Tabs(2).Selected = True
    Tabs1.DeselectAll
    Debug.Print (Tabs1.SelectedItem Is Nothing)
    Set p = Status1.Panels.Add(, "msg", "Ready")
    Status1.Panels.Add , "clock", "00:00", sbrTime
    Debug.Print Status1.Panels.Count; p.Text; p.Style; Status1.Panels(2).Left > 0; Status1.Style
    Status1.SimpleText = "simple"
    Debug.Print Status1.SimpleText
End Sub
Private Sub Tabs1_BeforeClick(Cancel As Integer)
    Debug.Print "before"
End Sub
Private Sub Tabs1_Click()
    Debug.Print "click "; Tabs1.SelectedItem.Caption
End Sub
Private Sub Status1_PanelClick(ByVal Panel As MSComctlLib.Panel)
    Debug.Print "panel "; Panel.Key
End Sub
Private Sub Status1_PanelDblClick(ByVal Panel As MSComctlLib.Panel)
    Debug.Print "dbl "; Panel.Key
End Sub
