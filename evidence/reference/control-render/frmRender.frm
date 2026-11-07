VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form frmRender
   Caption         =   "Control render"
   ClientHeight    =   4800
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7200
   LinkTopic       =   "Form1"
   ScaleHeight     =   4800
   ScaleWidth      =   7200
   StartUpPosition =   2
   Begin MSComctlLib.Toolbar tbr
      Height          =   420
      Left            =   0
      TabIndex        =   0
      Top             =   0
      Width           =   7200
      _ExtentX        =   12700
      _ExtentY        =   740
      _Version        =   393216
   End
   Begin MSComctlLib.TabStrip tabMain
      Height          =   1300
      Left            =   120
      TabIndex        =   1
      Top             =   540
      Width           =   3400
      _ExtentX        =   5997
      _ExtentY        =   2293
      _Version        =   393216
   End
   Begin MSComctlLib.ProgressBar prg
      Height          =   300
      Left            =   3700
      TabIndex        =   2
      Top             =   540
      Width           =   3300
      _ExtentX        =   5821
      _ExtentY        =   529
      _Version        =   393216
   End
   Begin MSComctlLib.Slider sld
      Height          =   500
      Left            =   3700
      TabIndex        =   3
      Top             =   960
      Width           =   3300
      _ExtentX        =   5821
      _ExtentY        =   882
      _Version        =   393216
   End
   Begin MSComctlLib.TreeView trv
      Height          =   2200
      Left            =   120
      TabIndex        =   4
      Top             =   1980
      Width           =   3400
      _ExtentX        =   5997
      _ExtentY        =   3880
      _Version        =   393216
   End
   Begin MSComctlLib.ListView lvw
      Height          =   2560
      Left            =   3700
      TabIndex        =   5
      Top             =   1620
      Width           =   3300
      _ExtentX        =   5821
      _ExtentY        =   4515
      _Version        =   393216
   End
   Begin MSComctlLib.StatusBar sbr
      Height          =   360
      Left            =   0
      TabIndex        =   6
      Top             =   4400
      Width           =   7200
      _ExtentX        =   12700
      _ExtentY        =   635
      _Version        =   393216
   End
End
Attribute VB_Name = "frmRender"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub Form_Load()
    Dim n As Node, i As Integer, it As ListItem
    tbr.Buttons.Add , "a", "Open"
    tbr.Buttons.Add , "b", "Save"
    tbr.Buttons.Add , "c", "Close"
    Caption = "Control render, initial tabs=" & tabMain.Tabs.Count
    tabMain.Tabs(1).Caption = "General"
    tabMain.Tabs.Add , "k2", "Colors"
    tabMain.Tabs.Add , "k3", "Fonts"
    prg.Min = 0: prg.Max = 100: prg.Value = 60
    sld.Min = 0: sld.Max = 10: sld.Value = 4
    Set n = trv.Nodes.Add(, , "r", "Root")
    Set n = trv.Nodes.Add("r", tvwChild, "c1", "Child one")
    Set n = trv.Nodes.Add("r", tvwChild, "c2", "Child two")
    Set n = trv.Nodes.Add("c2", tvwChild, "g1", "Grandchild")
    trv.Nodes("r").Expanded = True
    trv.Nodes("c2").Expanded = True
    lvw.View = lvwReport
    lvw.ColumnHeaders.Add , , "Name", 1400
    lvw.ColumnHeaders.Add , , "Size", 900
    For i = 1 To 4
        Set it = lvw.ListItems.Add(, , "Item " & i)
        it.SubItems(1) = CStr(i * 10)
    Next
    Caption = Caption & " panels=" & sbr.Panels.Count
    sbr.Panels(1).Text = "Ready"
    sbr.Panels.Add , , "Second"
End Sub
