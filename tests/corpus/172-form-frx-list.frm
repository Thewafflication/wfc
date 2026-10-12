VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Frx list"
   Begin VB.ListBox List1
      Height          =   1000
      List            =   "172-form-frx-list.frx":0000
      Width           =   1500
   End
   Begin VB.ComboBox Combo1
      Height          =   300
      List            =   "172-form-frx-list.frx":0000
      Top             =   1200
      Width           =   1500
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Debug.Print List1.ListCount; List1.List(0); List1.List(2); Combo1.ListCount
End Sub
