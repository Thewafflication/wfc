VERSION 5.00
Begin VB.Form frmShowcase
   Caption         =   "WFC form showcase"
   ClientHeight    =   4200
   ClientWidth     =   6600
   Begin VB.Timer tmrClose
      Interval        =   600
   End
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
Attribute VB_Name = "frmShowcase"
Option Explicit
Dim ticks As Long
Private Sub Form_Load()
    Dim i As Long
    For i = 1 To 5: lstItems.AddItem "List item " & i: Next
    lstItems.ListIndex = 1
    cboChoice.AddItem "Red": cboChoice.AddItem "Green"
End Sub
Private Sub Form_Activate()
    picCanvas.Line (10, 10)-(190, 90), RGB(0, 0, 160), B
    picCanvas.Circle (100, 50), 30, RGB(200, 0, 0)
    picCanvas.Line (10, 90)-(190, 10), RGB(0, 120, 0)
    picCanvas.CurrentX = 70: picCanvas.CurrentY = 44
    picCanvas.Print "WFC"
    Debug.Print "shown"
End Sub
Private Sub Timer1_Dummy()
End Sub
Private Sub tmrClose_Timer()
    ticks = ticks + 1
    lblStatus.Caption = "Status: tick " & ticks
    If ticks >= 2 Then Unload Me
End Sub
Private Sub Form_Unload(Cancel As Integer)
    Debug.Print "closing after"; ticks
End Sub
