VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Files"
   Begin VB.DriveListBox Drive1
      Height          =   315
      Width           =   1500
   End
   Begin VB.DirListBox Dir1
      Height          =   1500
      Top             =   400
      Width           =   2000
   End
   Begin VB.FileListBox File1
      Height          =   1500
      Left            =   2200
      Top             =   400
      Width           =   2000
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Dir1.Path = App.Path & "\170-form-filelists-data"
    File1.Path = Dir1.Path
    Debug.Print Right$(Dir1.Path, 8); Dir1.ListCount - Dir1.ListIndex - 1; File1.ListCount; File1.List(0); File1.List(1); File1.List(2)
    File1.Pattern = "*.txt;*.dat"
    Debug.Print File1.ListCount
    File1.Pattern = "*.dat"
    Debug.Print File1.ListCount; File1.List(0)
    File1.Pattern = "*.*"
    Debug.Print Dir1.List(Dir1.ListCount - 1) = Dir1.Path & "\Sub2"
    Debug.Print Left$(Drive1.Drive, 1) = LCase$(Left$(CurDir$, 1))
    Dir1.ListIndex = Dir1.ListCount - 2
    On Error Resume Next
    Dir1.Path = App.Path & "\nope"
    Debug.Print Err.Number; Err.Description
    Err.Clear
    Drive1.Drive = "9"
    Debug.Print Err.Number
End Sub
Private Sub Dir1_Change()
    Debug.Print "dir change "; Right$(Dir1.Path, 4)
    File1.Path = Dir1.Path
End Sub
Private Sub File1_PathChange()
    Debug.Print "file path change "; File1.ListCount
End Sub
Private Sub File1_Click()
    Debug.Print "file click "; File1.FileName
End Sub
