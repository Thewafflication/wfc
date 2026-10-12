VERSION 5.00
Begin VB.Form Form1
   Caption         =   "TreeView"
   Begin MSComctlLib.TreeView Tree1
      Height          =   2400
      Left            =   0
      Top             =   0
      Width           =   3000
      Checkboxes      =   -1  'True
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Dim a As Node, b As Node, c As Node, d As Node
    Set a = Tree1.Nodes.Add(, , "root", "Root")
    Set b = Tree1.Nodes.Add(a, tvwChild, "b", "Bravo")
    Set c = Tree1.Nodes.Add(a, tvwChild, "c", "Charlie")
    Set d = Tree1.Nodes.Add(c, tvwChild, "d", "Delta")
    Tree1.Nodes.Add b, tvwNext, "b2", "Between"
    Tree1.Nodes.Add c, tvwPrevious, "bp", "Before C"
    Tree1.Nodes.Add b, tvwFirst, "first", "First"
    Tree1.Nodes.Add b, tvwLast, "last", "Last"
    Debug.Print Tree1.Nodes.Count; a.Children; a.Child.Key; a.Child.Next.Key; a.Child.Next.Next.Key
    Debug.Print d.Parent.Key; d.Root.Key; d.FullPath; d.Index; b.Previous.Key; b.FirstSibling.Key; b.LastSibling.Key
    Debug.Print (a.Parent Is Nothing); (d.Child Is Nothing); TypeName(a)
    Debug.Print d.EnsureVisible; a.Expanded; c.Expanded
    a.Expanded = True
    Debug.Print Tree1.GetVisibleCount
    Set Tree1.SelectedItem = d
    Debug.Print Tree1.SelectedItem.Key; d.Selected
    Tree1.PathSeparator = "/"
    Debug.Print d.FullPath
    Debug.Print Tree1.HitTest(400, 10).Key; (Tree1.HitTest(2900, 10) Is Nothing)
    Tree1.Nodes.Remove "c"
    Debug.Print Tree1.Nodes.Count; (Tree1.SelectedItem Is Nothing)
    On Error Resume Next
    Tree1.Nodes.Add , , "b", "dup"
    Debug.Print Err.Number
    Err.Clear
    Tree1.Nodes.Add "nope", tvwChild, "z", "z"
    Debug.Print Err.Number
    On Error GoTo 0
    Tree1.Sorted = True
    Dim n As Node
    Set n = a.Child
    Do While Not n Is Nothing
        Debug.Print n.Text;
        Set n = n.Next
    Loop
    Debug.Print
End Sub
Private Sub Tree1_Expand(ByVal Node As MSComctlLib.Node)
    Debug.Print "expand "; Node.Key
End Sub
Private Sub Tree1_Collapse(ByVal Node As MSComctlLib.Node)
    Debug.Print "collapse "; Node.Key
End Sub
Private Sub Tree1_NodeClick(ByVal Node As MSComctlLib.Node)
    Debug.Print "nodeclick "; Node.Key
End Sub
Private Sub Tree1_NodeCheck(ByVal Node As MSComctlLib.Node)
    Debug.Print "check "; Node.Key; Node.Checked
End Sub
Private Sub Tree1_BeforeLabelEdit(Cancel As Integer)
    Debug.Print "before edit"
End Sub
Private Sub Tree1_AfterLabelEdit(Cancel As Integer, NewString As String)
    Debug.Print "after edit "; NewString
    If NewString = "bad" Then Cancel = True
End Sub
