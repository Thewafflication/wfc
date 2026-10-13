VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Geometry"
   ClientHeight    =   3000
   ClientWidth     =   3000
   ScaleMode       =   3  'Pixel
   Begin VB.Label Label1
      Caption         =   "x"
      Height          =   300
      Left            =   600
      Top             =   450
      Width           =   1500
   End
   Begin VB.Line Line1
      X1              =   150
      X2              =   1500
      Y1              =   300
      Y2              =   900
   End
   Begin VB.Image Image1
      BorderStyle     =   1  'Fixed Single
      Height          =   480
      Left            =   0
      Top             =   0
      Width           =   480
   End
End
Attribute VB_Name = "Form1"
Private Sub Form_Load()
    Dim shown As VB.Image
    Print Label1.Left; Label1.Top; Label1.Width; Label1.Height
    Label1.Left = 100
    Label1.Width = 5
    Print Label1.Left; Label1.Width; ScaleWidth; ScaleHeight
    Print Line1.X1; Line1.X2; Line1.Y1; Line1.Y2
    Line1.X2 = 20
    Print Line1.X2
    Label1.ForeColor = vbgray
    Print Label1.ForeColor
    Set shown = Image1
    Print TypeName(shown)
End Sub
