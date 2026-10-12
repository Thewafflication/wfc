VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form Form1
   Caption         =   "Persisted"
   Begin MSComctlLib.TabStrip TabStrip1 
      Height          =   1200
      Left            =   120
      Top             =   120
      Width           =   3000
      _ExtentX        =   5292
      _Version        =   393216
      BeginProperty Tabs {1EFB6598-857C-11D1-B16A-00C0F0283628} 
         NumTabs         =   2
         BeginProperty Tab1 {1EFB659A-857C-11D1-B16A-00C0F0283628} 
            Caption         =   "General"
            Key             =   "gen"
            Object.ToolTipText     =   "tip"
            ImageVarType    =   2
         EndProperty
         BeginProperty Tab2 {1EFB659A-857C-11D1-B16A-00C0F0283628} 
            Caption         =   "Advanced"
            Key             =   "adv"
            ImageVarType    =   2
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.StatusBar StatusBar1 
      Height          =   300
      Left            =   0
      Top             =   1500
      Width           =   3000
      _Version        =   393216
      BeginProperty Panels {8E3867A5-8586-11D1-B16A-00C0F0283628} 
         NumPanels       =   2
         BeginProperty Panel1 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
            AutoSize        =   1
            Object.Width    =   4000
            Key             =   "msg"
            Text            =   "Ready"
         EndProperty
         BeginProperty Panel2 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
            Style           =   6
            Key             =   "date"
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.Toolbar Toolbar1 
      Height          =   330
      Left            =   0
      Top             =   1900
      Width           =   3000
      BeginProperty Buttons {66833FE8-8583-11D1-B16A-00C0F0283628} 
         NumButtons      =   2
         BeginProperty Button1 {66833FEA-8583-11D1-B16A-00C0F0283628} 
            Caption         =   "Open"
            Key             =   "open"
            Style           =   5
            BeginProperty ButtonMenus {66833FEC-8583-11D1-B16A-00C0F0283628} 
               NumButtonMenus  =   1
               BeginProperty ButtonMenu1 {66833FEE-8583-11D1-B16A-00C0F0283628} 
                  Text            =   "Recent"
                  Key             =   "recent"
               EndProperty
            EndProperty
         EndProperty
         BeginProperty Button2 {66833FEA-8583-11D1-B16A-00C0F0283628} 
            Key             =   "sep"
            Style           =   3
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.ListView ListView1 
      Height          =   1000
      Left            =   0
      Top             =   2300
      Width           =   3000
      View            =   3
      _Version        =   393217
      BeginProperty ColumnHeader(1) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         Text            =   "Name"
         Object.Width    =   2000
      EndProperty
      BeginProperty ColumnHeader(2) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   1
         Text            =   "Size"
         Object.Width    =   1000
      EndProperty
   End
   Begin MSComctlLib.ImageList ImageList1 
      Left            =   3200
      Top             =   0
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   1
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "194-comctl-persisted.frx":0000
            Key             =   "red"
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.TreeView TreeView1 
      Height          =   500
      Left            =   3200
      Top             =   300
      Width           =   1000
      OleObjectBlob   =   "194-comctl-persisted.frx":0000
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Debug.Print TabStrip1.Tabs.Count; TabStrip1.Tabs("adv").Index; TabStrip1.Tabs(1).ToolTipText; TabStrip1.SelectedItem.Key
    Debug.Print StatusBar1.Panels.Count; StatusBar1.Panels("msg").Text; StatusBar1.Panels(1).AutoSize; StatusBar1.Panels(1).Width; StatusBar1.Panels(2).Style
    Debug.Print Toolbar1.Buttons.Count; Toolbar1.Buttons("open").Style; Toolbar1.Buttons("open").ButtonMenus("recent").Text
    Debug.Print ListView1.ColumnHeaders.Count; ListView1.ColumnHeaders(2).Text; ListView1.ColumnHeaders(2).Left
    Debug.Print ImageList1.ListImages.Count; ImageList1.ListImages("red").Index; ImageList1.ImageWidth
    Debug.Print TreeView1.Nodes.Count
End Sub
