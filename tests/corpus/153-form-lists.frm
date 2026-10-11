VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Lists"
   Begin VB.ListBox List1
      Height          =   1000
      Left            =   120
      Top             =   120
      Width           =   1500
   End
   Begin VB.ComboBox Combo1
      Left            =   2000
      Top             =   120
      Width           =   1500
   End
   Begin VB.ListBox Sorted1
      Height          =   600
      Left            =   120
      Sorted          =   -1  'True
      Top             =   1500
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Dim i As Long
    For i = 1 To 3: List1.AddItem "item" & i: Next
    List1.ItemData(List1.NewIndex) = 99
    List1.AddItem "first", 0
    Combo1.AddItem "red": Combo1.AddItem "green"
    Sorted1.AddItem "pear": Sorted1.AddItem "apple": Sorted1.AddItem "mango"
    Print List1.ListCount; List1.List(0); List1.List(3); List1.ItemData(3); List1.ListIndex
    Print Sorted1.List(0); Sorted1.List(1); Sorted1.List(2)
End Sub
Private Sub List1_Click()
    Print "list click"; List1.ListIndex; List1.Text
End Sub
Private Sub Combo1_Click()
    Print "combo click"; Combo1.ListIndex; Combo1.Text
End Sub
Private Sub Combo1_Change()
    Print "combo change"; Combo1.Text
End Sub
Private Sub Form_Activate()
    List1.RemoveItem 0
    Print List1.ListCount; List1.List(0)
    List1.ListIndex = 1
    Combo1.Text = "typed"
    Print Combo1.ListIndex
End Sub
