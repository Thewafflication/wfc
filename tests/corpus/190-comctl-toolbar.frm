VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Toolbar"
   Begin MSComctlLib.Toolbar Tool1
      Height          =   330
      Left            =   0
      Top             =   0
      Width           =   6000
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Dim b As Button
    Tool1.Buttons.Add , "new", "New"
    Tool1.Buttons.Add , "sep", , tbrSeparator
    Tool1.Buttons.Add , "bold", "Bold", tbrCheck
    Tool1.Buttons.Add , "left", "Left", tbrButtonGroup
    Tool1.Buttons.Add , "center", "Center", tbrButtonGroup
    Set b = Tool1.Buttons.Add(, "drop", "Open", tbrDropdown)
    b.ButtonMenus.Add , "m1", "Recent 1"
    b.ButtonMenus.Add , "m2", "Recent 2"
    Debug.Print Tool1.Buttons.Count; b.ButtonMenus.Count; b.ButtonMenus("m2").Text; b.Style; TypeName(b); TypeName(b.ButtonMenus(1))
    Debug.Print Tool1.Buttons("sep").Width; Tool1.Buttons("bold").Left > Tool1.Buttons("sep").Left; Tool1.Buttons(1).Top
    Tool1.Buttons("left").Value = tbrPressed
    Tool1.Buttons("center").Value = tbrPressed
    Debug.Print Tool1.Buttons("left").Value; Tool1.Buttons("center").Value
    On Error Resume Next
    Tool1.Buttons("new").Style = 9
    Debug.Print Err.Number
    Err.Clear
    Tool1.Buttons("new").Left = 5
    Debug.Print Err.Number
    On Error GoTo 0
    Tool1.SaveToolbar "app", "tb", "main"
    Tool1.Buttons("new").Visible = False
    Tool1.RestoreToolbar "app", "tb", "main"
    Debug.Print Tool1.Buttons("new").Visible
End Sub
Private Sub Tool1_ButtonClick(ByVal Button As MSComctlLib.Button)
    Debug.Print "click "; Button.Key; Button.Value
End Sub
Private Sub Tool1_ButtonDropDown(ByVal Button As MSComctlLib.Button)
    Debug.Print "dropdown "; Button.Key
End Sub
Private Sub Tool1_ButtonMenuClick(ByVal ButtonMenu As MSComctlLib.ButtonMenu)
    Debug.Print "menu "; ButtonMenu.Key; ButtonMenu.Text
End Sub
Private Sub Tool1_Change()
    Debug.Print "changed"
End Sub
