VERSION 5.00
Begin VB.Form frmCommon
   Caption         =   "Common controls"
   ClientHeight    =   5400
   ClientWidth     =   8400
   Begin VB.Timer Timer1
      Interval        =   2500
   End
   Begin MSComctlLib.ImageList Images1
      Left            =   8000
      Top             =   0
   End
   Begin MSComctlLib.Toolbar Toolbar1
      Height          =   420
      Left            =   0
      Top             =   0
      Width           =   8400
   End
   Begin MSComctlLib.TabStrip TabStrip1
      Height          =   1500
      Left            =   120
      Top             =   480
      Width           =   3000
   End
   Begin MSComctlLib.TreeView TreeView1
      Height          =   2200
      Left            =   120
      Top             =   2100
      Width           =   2400
   End
   Begin MSComctlLib.ListView ListView1
      Height          =   2200
      Left            =   2700
      Top             =   2100
      Width           =   3600
      View            =   3
   End
   Begin MSComctlLib.ProgressBar ProgressBar1
      Height          =   255
      Left            =   3300
      Top             =   600
      Width           =   2400
      Value           =   65
   End
   Begin MSComctlLib.Slider Slider1
      Height          =   500
      Left            =   3300
      Top             =   1000
      Width           =   2400
      Max             =   10
      Value           =   4
   End
   Begin MSComctlLib.ImageCombo ImageCombo1
      Height          =   300
      Left            =   6000
      Top             =   600
      Width           =   2100
   End
   Begin MSComctlLib.StatusBar StatusBar1
      Height          =   300
      Left            =   0
      Top             =   5100
      Width           =   8400
   End
End
Attribute VB_Name = "frmCommon"
Option Explicit
Private Sub Form_Load()
    Dim n As Node
    Images1.ListImages.Add , "red", LoadPicture(App.Path & "\gui-comctl.red.bmp")
    Set Toolbar1.ImageList = Images1
    Set TabStrip1.ImageList = Images1
    Set TreeView1.ImageList = Images1
    Set ListView1.SmallIcons = Images1
    Set ListView1.Icons = Images1
    Set ImageCombo1.ImageList = Images1
    Toolbar1.Buttons.Add , "new", "New", , "red"
    Toolbar1.Buttons.Add , "open", "Open", tbrDropdown
    Toolbar1.Buttons.Add , "sep", , tbrSeparator
    Toolbar1.Buttons.Add , "bold", "Bold", tbrCheck
    TabStrip1.Tabs.Add , "gen", "General", 1
    TabStrip1.Tabs.Add , "adv", "Advanced"
    TabStrip1.Tabs.Add , "log", "Log"
    Set n = TreeView1.Nodes.Add(, , "root", "Root")
    TreeView1.Nodes.Add n, tvwChild, "a", "Alpha", "red"
    TreeView1.Nodes.Add n, tvwChild, "b", "Beta"
    n.Expanded = True
    ListView1.ColumnHeaders.Add , , "Name", 1500
    ListView1.ColumnHeaders.Add , , "Size", 1000
    ListView1.ListItems.Add(, "x", "One", , "red").SubItems(1) = "1 KB"
    ListView1.ListItems.Add(, "y", "Two").SubItems(1) = "2 KB"
    ImageCombo1.ComboItems.Add , "r", "Red", "red"
    ImageCombo1.ComboItems.Add , "g", "Green"
    Set ImageCombo1.SelectedItem = ImageCombo1.ComboItems("g")
    StatusBar1.Panels.Add , "msg", "Ready"
    StatusBar1.Panels.Add , "n", "3 items"
End Sub
Private Sub Timer1_Timer()
    Debug.Print "closing "; TabStrip1.Tabs.Count; TreeView1.Nodes.Count; ListView1.ListItems.Count; ProgressBar1.Value
    Unload Me
End Sub
