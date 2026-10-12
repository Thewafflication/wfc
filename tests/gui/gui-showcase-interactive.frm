VERSION 5.00
Begin VB.Form frmPlay
   Caption         =   "WFC form showcase (interactive)"
   ClientHeight    =   4200
   ClientWidth     =   6600
   Begin VB.Frame fraOptions
      Caption         =   "Options"
      Height          =   1215
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   2415
      Begin VB.OptionButton optA
         Caption         =   "Option A"
         Height          =   255
         Left            =   120
         TabIndex        =   1
         Top             =   300
         Value           =   -1  'True
         Width           =   1815
      End
      Begin VB.OptionButton optB
         Caption         =   "Option B"
         Height          =   255
         Left            =   120
         TabIndex        =   2
         Top             =   660
         Width           =   1815
      End
      Begin VB.CheckBox chkFlag
         Caption         =   "A check box"
         Height          =   255
         Left            =   120
         TabIndex        =   3
         Top             =   960
         Value           =   1  'Checked
         Width           =   1815
      End
   End
   Begin VB.TextBox txtName
      Height          =   315
      Left            =   2760
      TabIndex        =   4
      Text            =   "Type here"
      Top             =   240
      Width           =   2055
   End
   Begin VB.CommandButton cmdGo
      Caption         =   "&Go"
      Default         =   -1  'True
      Height          =   375
      Left            =   5040
      TabIndex        =   5
      Top             =   200
      Width           =   1215
   End
   Begin VB.ListBox lstItems
      Height          =   1230
      Left            =   2760
      TabIndex        =   6
      Top             =   720
      Width           =   2055
   End
   Begin VB.ComboBox cboChoice
      Height          =   315
      Left            =   5040
      TabIndex        =   7
      Text            =   "Pick one"
      Top             =   720
      Width           =   1455
   End
   Begin VB.HScrollBar hsbLevel
      Height          =   255
      Left            =   120
      Max             =   100
      TabIndex        =   8
      Top             =   1560
      Value           =   40
      Width           =   2415
   End
   Begin VB.Label lblStatus
      Caption         =   "Status: ready"
      Height          =   255
      Left            =   2760
      TabIndex        =   9
      Top             =   2100
      Width           =   3495
   End
   Begin VB.PictureBox picCanvas
      BackColor       =   &H00FFFFFF&
      Height          =   1700
      Left            =   120
      ScaleHeight     =   100
      ScaleWidth      =   200
      TabIndex        =   10
      Top             =   2300
      Width           =   3200
   End
   Begin VB.Shape shpBadge
      FillColor       =   &H0000C000&
      FillStyle       =   0  'Solid
      Height          =   735
      Left            =   3600
      Shape           =   3  'Circle
      Top             =   2700
      Width           =   735
   End
   Begin VB.Menu mnuFile
      Caption         =   "&File"
      Begin VB.Menu mnuExit
         Caption         =   "E&xit"
      End
   End
End
Attribute VB_Name = "frmPlay"
Option Explicit
Private Sub Form_Load()
    Dim i As Long
    For i = 1 To 5: lstItems.AddItem "List item " & i: Next
    lstItems.ListIndex = 1
    cboChoice.AddItem "Red": cboChoice.AddItem "Green": cboChoice.AddItem "Blue"
End Sub
Private Sub Form_Activate()
    picCanvas.Line (10, 10)-(190, 90), RGB(0, 0, 160), B
    picCanvas.Circle (100, 50), 30, RGB(200, 0, 0)
    picCanvas.Line (10, 90)-(190, 10), RGB(0, 120, 0)
    picCanvas.CurrentX = 70: picCanvas.CurrentY = 44
    picCanvas.Print "WFC"
End Sub
Private Sub cmdGo_Click()
    lblStatus.Caption = "Hello, " & txtName.Text & "!"
End Sub
Private Sub optA_Click()
    lblStatus.Caption = "Option A"
End Sub
Private Sub optB_Click()
    lblStatus.Caption = "Option B"
End Sub
Private Sub chkFlag_Click()
    lblStatus.Caption = "Check box = " & chkFlag.Value
End Sub
Private Sub lstItems_Click()
    lblStatus.Caption = "Chose " & lstItems.Text
End Sub
Private Sub cboChoice_Click()
    lblStatus.Caption = "Combo: " & cboChoice.Text
End Sub
Private Sub hsbLevel_Change()
    lblStatus.Caption = "Level " & hsbLevel.Value
    shpBadge.FillColor = RGB(hsbLevel.Value * 2, 200 - hsbLevel.Value, 60)
End Sub
Private Sub mnuExit_Click()
    Unload Me
End Sub
