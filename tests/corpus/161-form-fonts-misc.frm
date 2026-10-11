VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Misc"
   Begin VB.TextBox Text1
      BeginProperty Font
         Name            =   "Courier New"
         Size            =   12
         Weight          =   700
      EndProperty
   End
   Begin VB.TextBox Text2
      Text            =   "x"
   End
   Begin VB.Menu mnuPop
      Caption         =   "Pop"
      Visible         =   0   'False
      Begin VB.Menu mnuItem
         Caption         =   "Item"
      End
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Activate()
    Print Text1.Font.Name; Text1.Font.Size; Text1.Font.Bold; Text2.Font.Bold
    Text2.Font.Bold = True: Text2.Font.Size = 14
    Print Text2.Font.Bold; Text2.Font.Size; Text2.FontBold
    Set Text2.Font = Text1.Font
    Print Text2.Font.Name
    Print ScaleWidth; Width; ScaleHeight
    On Error Resume Next
    Text1.LinkRequest
    Print Err.Number; Err.Description
    Err.Clear
    PopupMenu mnuPop
    ValidateControls
    Print Err.Number
End Sub
Private Sub mnuItem_Click()
    Print "popup chose item"
End Sub
