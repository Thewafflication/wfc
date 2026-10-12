VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Refinements"
   Begin MSComctlLib.TabStrip Tabs1
      Height          =   1000
      Left            =   0
      Top             =   0
      Width           =   3000
   End
   Begin MSComctlLib.StatusBar Status1
      Height          =   300
      Left            =   0
      Top             =   1000
      Width           =   3000
   End
   Begin MSComctlLib.Slider Slider1
      Height          =   400
      Left            =   0
      Top             =   1400
      Width           =   3000
      Max             =   20
   End
   Begin MSComctlLib.ImageList Images1
      Left            =   3200
      Top             =   0
   End
   Begin MSComctlLib.Toolbar Tool1
      Height          =   330
      Left            =   0
      Top             =   1800
      Width           =   3000
   End
   Begin MSComctlLib.TreeView Tree1
      Height          =   1200
      Left            =   0
      Top             =   2200
      Width           =   1500
      LabelEdit       =   0
   End
   Begin MSComctlLib.ListView List1
      Height          =   1200
      Left            =   1600
      Top             =   2200
      Width           =   1500
      View            =   3
      GridLines       =   -1  'True
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Dim i As Long
    Tabs1.Tabs.Add , "a", "A"
    Tabs1.Tabs.Add , "b", "B"
    Dim p As String
    p = App.Path & "\195-comctl-refine.red.bmp"
    For i = 1 To 16
        Status1.Panels.Add , , "p" & i
    Next
    On Error Resume Next
    Status1.Panels.Add , , "too many"
    Debug.Print Status1.Panels.Count; Err.Number
    Err.Clear
    Slider1.SelectRange = False
    Slider1.Value = 6
    Debug.Print Slider1.SelStart; Slider1.SelLength
    Slider1.SelStart = 9
    Slider1.SelLength = 4
    Debug.Print Slider1.Value; Slider1.SelStart; Slider1.SelLength
    Slider1.SelectRange = True
    Slider1.SelStart = 2
    Slider1.SelLength = 5
    Debug.Print Slider1.SelStart; Slider1.SelLength
    Slider1.SelLength = 99
    Debug.Print Err.Number
    Err.Clear
    Slider1.SelLength = -1
    Debug.Print Err.Number
    Err.Clear
    Slider1.ClearSel
    Debug.Print Slider1.SelStart = Slider1.Value; Slider1.SelLength
    Images1.ListImages.Add , "one", LoadPicture(p)
    Images1.ListImages.Add , "two", LoadPicture(p)
    Debug.Print TypeName(Images1.ListImages("one").ExtractIcon); Images1.ListImages.Count
    Set Tool1.ImageList = Images1
    Images1.ListImages.Add , "three", LoadPicture(p)
    Debug.Print Images1.ListImages.Count
    Images1.ListImages.Add 1, "front", LoadPicture(p)
    Debug.Print Err.Number
    Err.Clear
    Images1.ListImages.Remove 1
    Debug.Print Err.Number
    Err.Clear
    Debug.Print Tool1.AllowCustomize
    Dim n As Node
    Set n = Tree1.Nodes.Add(, , "alpha", "Alpha")
    Tree1.Nodes.Add , , "bravo", "Bravo"
    Tree1.Nodes.Add , , "bridge", "Bridge"
    Tree1.Nodes.Add , , "delta", "Delta"
    Tool1.Buttons.Add , "k1", "One"
    Tool1.SaveToolbar "Software\WFC Tests", "Toolbar", "layout"
    Tool1.Buttons("k1").Visible = False
    Tool1.RestoreToolbar "Software\WFC Tests", "Toolbar", "layout"
    Debug.Print Tool1.Buttons("k1").Visible
    List1.ColumnHeaders.Add , , "c1", 1000
    List1.ListItems.Add , "r1", "Row1"
    List1.ListItems.Add , "r2", "Row2"
    Debug.Print n.EnsureVisible
    Tree1.Scroll = False
    Debug.Print n.EnsureVisible
End Sub
Private Sub Tabs1_BeforeClick(Cancel As Integer)
    Debug.Print "before"
    Cancel = True
End Sub
Private Sub Tabs1_Click()
    Debug.Print "click "; Tabs1.SelectedItem.Key
End Sub
Private Sub Slider1_Scroll()
    Debug.Print "scroll"
End Sub
Private Sub Slider1_Click()
    Debug.Print "sclick"
End Sub
Private Sub Slider1_Change()
    Debug.Print "schange"; Slider1.Value
End Sub
Private Sub Tool1_DblClick()
    Debug.Print "tdbl"
End Sub
Private Sub Tool1_Change()
    Debug.Print "customize"
End Sub
Private Sub Tree1_Click()
    Debug.Print "tclick "; Tree1.SelectedItem.Key
End Sub
Private Sub Tree1_BeforeLabelEdit(Cancel As Integer)
    Debug.Print "t-before"
End Sub
Private Sub Tree1_AfterLabelEdit(Cancel As Integer, NewString As String)
    Debug.Print "t-after ["; NewString; "]"
End Sub
Private Sub List1_BeforeLabelEdit(Cancel As Integer)
    Debug.Print "l-before"
    Cancel = True
End Sub
Private Sub List1_AfterLabelEdit(Cancel As Integer, NewString As String)
    Debug.Print "l-after "; NewString
End Sub
