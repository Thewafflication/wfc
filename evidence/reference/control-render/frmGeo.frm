VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form frmGeo
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
Attribute VB_Name = "frmGeo"
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
    Dump
End Sub

Private Sub Rec(ByVal label As String, ByVal v As String)
    Dim n As Integer
    n = FreeFile
    Open App.Path & "\geometry.txt" For Append As #n
    Print #n, label & " = " & v
    Close #n
End Sub

Private Sub Dump()
    Dim i As Long
    On Error Resume Next
    Rec "form | ScaleWidth", CStr(ScaleWidth)
    Rec "form | ScaleHeight", CStr(ScaleHeight)
    Rec "tbr | Left", CStr(tbr.Left)
    Rec "tbr | Top", CStr(tbr.Top)
    Rec "tbr | Width", CStr(tbr.Width)
    Rec "tbr | Height", CStr(tbr.Height)
    Rec "tbr | ButtonHeight", CStr(tbr.ButtonHeight)
    Rec "tbr | ButtonWidth", CStr(tbr.ButtonWidth)
    Rec "tbr | Style", CStr(tbr.Style)
    Rec "tbr | TextAlignment", CStr(tbr.TextAlignment)
    Rec "tbr | Appearance", CStr(tbr.Appearance)
    Rec "tbr | BorderStyle", CStr(tbr.BorderStyle)
    Rec "tbr | Wrappable", CStr(tbr.Wrappable)
    Rec "tab | Left", CStr(tabMain.Left)
    Rec "tab | Top", CStr(tabMain.Top)
    Rec "tab | Width", CStr(tabMain.Width)
    Rec "tab | Height", CStr(tabMain.Height)
    Rec "tab | ClientLeft", CStr(tabMain.ClientLeft)
    Rec "tab | ClientTop", CStr(tabMain.ClientTop)
    Rec "tab | ClientWidth", CStr(tabMain.ClientWidth)
    Rec "tab | ClientHeight", CStr(tabMain.ClientHeight)
    Rec "tab | Style", CStr(tabMain.Style)
    Rec "tab | TabStyle", CStr(tabMain.TabStyle)
    Rec "tab | Style", CStr(tabMain.Style)
    Rec "prg | Height", CStr(prg.Height)
    Rec "prg | BorderStyle", CStr(prg.BorderStyle)
    Rec "prg | Scrolling", CStr(prg.Scrolling)
    Rec "prg | Appearance", CStr(prg.Appearance)
    Rec "sld | Height", CStr(sld.Height)
    Rec "sld | BorderStyle", CStr(sld.BorderStyle)
    Rec "sld | TickStyle", CStr(sld.TickStyle)
    Rec "trv | Style", CStr(trv.Style)
    Rec "trv | Appearance", CStr(trv.Appearance)
    Rec "trv | Height", CStr(trv.Height)
    Rec "trv | BorderStyle", CStr(trv.BorderStyle)
    Rec "trv | Indentation", CStr(trv.Indentation)
    Rec "trv | LineStyle", CStr(trv.LineStyle)
    Rec "lvw | View", CStr(lvw.View)
    Rec "lvw | Height", CStr(lvw.Height)
    Rec "lvw | Appearance", CStr(lvw.Appearance)
    Rec "lvw | BorderStyle", CStr(lvw.BorderStyle)
    Rec "sbr | Top", CStr(sbr.Top)
    Rec "sbr | Height", CStr(sbr.Height)
    Rec "sbr | Style", CStr(sbr.Style)
    For i = 1 To tbr.Buttons.Count
        Rec "tbr | Button" & i & " Left", CStr(tbr.Buttons(i).Left)
        Rec "tbr | Button" & i & " Top", CStr(tbr.Buttons(i).Top)
        Rec "tbr | Button" & i & " Width", CStr(tbr.Buttons(i).Width)
        Rec "tbr | Button" & i & " Height", CStr(tbr.Buttons(i).Height)
    Next
    For i = 1 To tabMain.Tabs.Count
        Rec "tab | Tab" & i & " Left", CStr(tabMain.Tabs(i).Left)
        Rec "tab | Tab" & i & " Top", CStr(tabMain.Tabs(i).Top)
        Rec "tab | Tab" & i & " Width", CStr(tabMain.Tabs(i).Width)
        Rec "tab | Tab" & i & " Height", CStr(tabMain.Tabs(i).Height)
    Next
    For i = 1 To sbr.Panels.Count
        Rec "sbr | Panel" & i & " Width", CStr(sbr.Panels(i).Width)
        Rec "sbr | Panel" & i & " Style", CStr(sbr.Panels(i).Style)
        Rec "sbr | Panel" & i & " Bevel", CStr(sbr.Panels(i).Bevel)
        Rec "sbr | Panel" & i & " Alignment", CStr(sbr.Panels(i).Alignment)
        Rec "sbr | Panel" & i & " AutoSize", CStr(sbr.Panels(i).AutoSize)
    Next
End Sub
