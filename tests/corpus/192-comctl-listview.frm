VERSION 5.00
Begin VB.Form Form1
   Caption         =   "ListView"
   Begin MSComctlLib.ListView List1
      Height          =   2400
      Left            =   0
      Top             =   0
      Width           =   4800
      View            =   3
      Checkboxes      =   -1  'True
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Dim li As ListItem, ch As ColumnHeader
    Set ch = List1.ColumnHeaders.Add(, "name", "Name", 1500)
    List1.ColumnHeaders.Add , "size", "Size", 900, lvwColumnRight
    List1.ColumnHeaders.Add , "kind", "Kind", 1200
    Debug.Print List1.ColumnHeaders.Count; ch.Left; List1.ColumnHeaders("kind").Left; List1.ColumnHeaders(2).Position; List1.ColumnHeaders(2).SubItemIndex; List1.ColumnHeaders(2).Alignment
    Set li = List1.ListItems.Add(, "b", "Banana")
    li.SubItems(1) = "12"
    li.SubItems(2) = "fruit"
    Set li = List1.ListItems.Add(, "a", "Apple")
    li.ListSubItems.Add , "sz", "7"
    li.ListSubItems.Add , , "fruit", , "tip"
    List1.ListItems.Add , "c", "Carrot", , 1
    Debug.Print List1.ListItems.Count; List1.ListItems(2).SubItems(1); List1.ListItems(1).ListSubItems.Count; List1.ListItems("a").ListSubItems("sz").Text; li.ListSubItems(2).ToolTipText; TypeName(li.ListSubItems(1))
    Debug.Print li.Top; List1.ListItems(1).Top; List1.ListItems(3).Top
    List1.SortKey = 0
    List1.Sorted = True
    Debug.Print List1.ListItems(1).Text; List1.ListItems(2).Text; List1.ListItems(3).Text
    List1.SortOrder = lvwDescending
    Debug.Print List1.ListItems(1).Text; List1.ListItems(3).Text
    List1.SortKey = 1
    List1.SortOrder = lvwAscending
    Debug.Print List1.ListItems(1).Text; List1.ListItems(2).Text
    Debug.Print List1.FindItem("banana").Key; List1.FindItem("ap", lvwText, , lvwPartial).Key; (List1.FindItem("zzz") Is Nothing); List1.FindItem("fruit", lvwSubItem).Key
    Debug.Print List1.HitTest(100, 300).Text; (List1.HitTest(100, 2300) Is Nothing); List1.GetFirstVisible.Key
    Set List1.SelectedItem = List1.ListItems("b")
    Debug.Print List1.SelectedItem.Key; List1.ListItems("b").Selected
    On Error Resume Next
    List1.View = 7
    Debug.Print Err.Number
    Err.Clear
    Debug.Print List1.ListItems(1).SubItems(9)
    Debug.Print Err.Number
    Err.Clear
    List1.ListItems.Add , "a", "dup"
    Debug.Print Err.Number
End Sub
Private Sub List1_ItemClick(ByVal Item As MSComctlLib.ListItem)
    Debug.Print "itemclick "; Item.Key
End Sub
Private Sub List1_ItemCheck(ByVal Item As MSComctlLib.ListItem)
    Debug.Print "itemcheck "; Item.Key; Item.Checked
End Sub
Private Sub List1_ColumnClick(ByVal ColumnHeader As MSComctlLib.ColumnHeader)
    Debug.Print "column "; ColumnHeader.Key
End Sub
Private Sub List1_AfterLabelEdit(Cancel As Integer, NewString As String)
    Debug.Print "renamed "; NewString
End Sub
