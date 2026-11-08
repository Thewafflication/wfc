VERSION 5.00
Object = "{86CF1D34-0C5F-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCT2.OCX"
Begin VB.Form frmRender2
   Caption         =   "Control render 2"
   ClientHeight    =   4200
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7200
   LinkTopic       =   "Form1"
   ScaleHeight     =   4200
   ScaleWidth      =   7200
   StartUpPosition =   3  'Windows Default
   Begin MSComCtl2.UpDown upd
      Height          =   600
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   240
      _ExtentX        =   423
      _ExtentY        =   1058
      _Version        =   393216
      Enabled         =   -1  'True
   End
   Begin MSComCtl2.MonthView mvw
      Height          =   2370
      Left            =   600
      TabIndex        =   1
      Top             =   120
      Width           =   2700
      _ExtentX        =   4763
      _ExtentY        =   4180
      _Version        =   393216
      ForeColor       =   -2147483630
      BackColor       =   -2147483633
      Appearance      =   1
      StartOfWeek     =   65863682
      CurrentDate     =   46333
   End
   Begin MSComCtl2.DTPicker dtp
      Height          =   300
      Left            =   3600
      TabIndex        =   2
      Top             =   120
      Width           =   2400
      _ExtentX        =   4233
      _ExtentY        =   529
      _Version        =   393216
      Format          =   65863681
      CurrentDate     =   46333
   End
   Begin MSComCtl2.FlatScrollBar fsh
      Height          =   255
      Left            =   3600
      TabIndex        =   3
      Top             =   720
      Width           =   2400
      _ExtentX        =   4233
      _ExtentY        =   450
      _Version        =   393216
      Orientation     =   1
      Max             =   100
      Value           =   30
   End
   Begin MSComCtl2.FlatScrollBar fsv
      Height          =   1800
      Left            =   6300
      TabIndex        =   4
      Top             =   120
      Width           =   255
      _ExtentX        =   450
      _ExtentY        =   3175
      _Version        =   393216
      Orientation     =   0
      Max             =   100
      Value           =   60
   End
   Begin MSComCtl2.Animation ani
      Height          =   900
      Left            =   3600
      TabIndex        =   5
      Top             =   1200
      Width           =   2400
      _ExtentX        =   4233
      _ExtentY        =   1588
      _Version        =   393216
      FullWidth       =   160
      FullHeight      =   60
   End
End
Attribute VB_Name = "frmRender2"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub Rec(ByVal label As String, ByVal v As String)
    Dim n As Integer
    n = FreeFile
    Open App.Path & "\geometry2.txt" For Append As #n
    Print #n, label & " = " & v
    Close #n
End Sub

Private Sub Form_Load()
    upd.Min = 0: upd.Max = 10: upd.Value = 3
    fsh.Value = 30
    Caption = "Control render 2"
    Dump
End Sub

Private Sub Dump()
    On Error Resume Next
    Rec "upd | Left", CStr(upd.Left)
    Rec "upd | Top", CStr(upd.Top)
    Rec "upd | Width", CStr(upd.Width)
    Rec "upd | Height", CStr(upd.Height)
    Rec "upd | Value", CStr(upd.Value)
    Rec "mvw | Left", CStr(mvw.Left)
    Rec "mvw | Top", CStr(mvw.Top)
    Rec "mvw | Width", CStr(mvw.Width)
    Rec "mvw | Height", CStr(mvw.Height)
    Rec "mvw | StartOfWeek", CStr(mvw.StartOfWeek)
    Rec "mvw | MonthRows", CStr(mvw.MonthRows)
    Rec "mvw | MonthColumns", CStr(mvw.MonthColumns)
    Rec "mvw | VisibleDay1", CStr(Int(CDbl(mvw.VisibleDays(1))))
    Rec "mvw | VisibleDay42", CStr(Int(CDbl(mvw.VisibleDays(42))))
    Rec "dtp | Left", CStr(dtp.Left)
    Rec "dtp | Top", CStr(dtp.Top)
    Rec "dtp | Width", CStr(dtp.Width)
    Rec "dtp | Height", CStr(dtp.Height)
    Rec "dtp | Format", CStr(dtp.Format)
    Rec "fsh | Left", CStr(fsh.Left)
    Rec "fsh | Top", CStr(fsh.Top)
    Rec "fsh | Width", CStr(fsh.Width)
    Rec "fsh | Height", CStr(fsh.Height)
    Rec "fsh | Value", CStr(fsh.Value)
    Rec "fsv | Left", CStr(fsv.Left)
    Rec "fsv | Width", CStr(fsv.Width)
    Rec "fsv | Height", CStr(fsv.Height)
    Rec "fsv | Value", CStr(fsv.Value)
    Rec "ani | Width", CStr(ani.Width)
    Rec "ani | Height", CStr(ani.Height)
End Sub
