VERSION 5.00
Begin VB.Form Form1
   Caption         =   "ImageCombo"
   Begin MSComctlLib.ImageCombo Combo1
      Height          =   300
      Left            =   120
      Top             =   120
      Width           =   2400
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Dim ci As ComboItem
    Set ci = Combo1.ComboItems.Add(, "red", "Red", 1, 2, 0)
    Combo1.ComboItems.Add , "green", "Green"
    Combo1.ComboItems.Add 1, "blue", "Blue", , , 2
    Debug.Print Combo1.ComboItems.Count; Combo1.ComboItems(1).Key; ci.Index; ci.Image; ci.SelImage; Combo1.ComboItems("blue").Indentation; TypeName(ci)
    Debug.Print (Combo1.SelectedItem Is Nothing); "[" & Combo1.Text & "]"
    Set Combo1.SelectedItem = Combo1.ComboItems("green")
    Debug.Print Combo1.Text; Combo1.SelectedItem.Key; Combo1.ComboItems("green").Selected
    Combo1.Text = "Red"
    Debug.Print Combo1.SelectedItem.Key
    Combo1.Text = "custom"
    Debug.Print (Combo1.SelectedItem Is Nothing); Combo1.Text
    Combo1.SelStart = 1
    Combo1.SelLength = 3
    Debug.Print Combo1.SelText
    Combo1.SelText = "XY"
    Debug.Print Combo1.Text; Combo1.SelLength
    Debug.Print Combo1.GetFirstVisible.Key
    Combo1.ComboItems("green").Selected = True
    Combo1.ComboItems.Remove "green"
    Debug.Print (Combo1.SelectedItem Is Nothing); Combo1.ComboItems.Count
    On Error Resume Next
    Combo1.ComboItems.Add , "red", "dup"
    Debug.Print Err.Number
    Err.Clear
    Debug.Print Combo1.ComboItems(7).Text
    Debug.Print Err.Number
    On Error GoTo 0
    Combo1.Locked = True
End Sub
Private Sub Combo1_Click()
    Debug.Print "click "; Combo1.Text
End Sub
Private Sub Combo1_Change()
    Debug.Print "change "; Combo1.Text
End Sub
Private Sub Combo1_Dropdown()
    Debug.Print "dropdown"
End Sub
