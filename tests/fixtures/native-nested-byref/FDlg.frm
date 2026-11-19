VERSION 5.00
Begin VB.Form FDlg
   Caption         =   "Dialog"
   ClientHeight    =   1000
   ClientWidth     =   2000
   Begin VB.Label Label1
      Caption         =   "x"
      Height          =   300
      Left            =   120
      Top             =   120
      Width           =   900
   End
End
Attribute VB_Name = "FDlg"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub Form_Load()
    Dim c As Control, s As String
    For Each c In Me.Controls
        s = s & Left(c.Name, 3) & Format(12.5, "0.00") & Trim(" x ")
        s = Replace(s, "x", "y", 1, -1, vbTextCompare) & Mid(s, 1, 2)
    Next c
    Label1.Caption = s
End Sub

' A call into this form passes the caller's variable by reference; the
' library calls made while it runs must not disturb that argument.
Public Function Owned() As Boolean
    Owned = OwnerOf(Me, Label1)
End Function

Public Function Fresh(aOwner As Form, aBoard As Board, aP1 As Person, aP2 As Person) As Long
    aBoard.Reset 7, 6
    Unload Me
    Fresh = vbOK
End Function

' A ByRef argument changed after a library call still reaches the caller.
Public Sub Bump(aCount As Long, aText As String)
    Dim s As String
    s = Left("abcdef", 3) & Format(1.5, "0.0")
    aCount = aCount + Len(s)
    aText = aText & s
End Sub
