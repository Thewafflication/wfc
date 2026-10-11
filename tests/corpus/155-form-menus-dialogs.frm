VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Menus"
   Begin VB.Menu mnuFile
      Caption         =   "&File"
      Begin VB.Menu mnuOpen
         Caption         =   "&Open"
         Shortcut        =   ^O
      End
      Begin VB.Menu mnuSep
         Caption         =   "-"
      End
      Begin VB.Menu mnuExit
         Caption         =   "E&xit"
      End
   End
   Begin VB.Menu mnuView
      Caption         =   "&View"
      Begin VB.Menu mnuToolbar
         Caption         =   "&Toolbar"
         Checked         =   -1  'True
      End
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub mnuOpen_Click()
    Print "open"; mnuOpen.Caption; mnuOpen.Enabled
    mnuExit.Enabled = False
End Sub
Private Sub mnuToolbar_Click()
    mnuToolbar.Checked = Not mnuToolbar.Checked
    Print "toolbar checked="; mnuToolbar.Checked
End Sub
Private Sub mnuExit_Click()
    Print "exit"
    Unload Me
End Sub
Private Sub Form_Click()
    Dim r As Long, s As String
    r = MsgBox("Sure?", vbYesNo)
    s = InputBox("Name?", "Title", "def")
    Print "answers"; r; s
End Sub
