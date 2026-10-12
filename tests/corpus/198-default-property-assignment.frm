VERSION 5.00
Begin VB.Form Form1
   Caption         =   "t"
   Begin VB.TextBox Text1
      Height          =   300
      Index           =   0
      Left            =   0
      Top             =   0
      Width           =   1000
   End
   Begin VB.TextBox Text1
      Height          =   300
      Index           =   1
      Left            =   0
      Top             =   400
      Width           =   1000
   End
   Begin VB.TextBox Text2
      Height          =   300
      Left            =   0
      Top             =   800
      Width           =   1000
   End
   Begin VB.PictureBox Pic2
      Height          =   300
      Left            =   1100
      Top             =   1200
      Width           =   1000
   End
   Begin VB.PictureBox Pic1
      Height          =   300
      Index           =   0
      Left            =   0
      Top             =   1200
      Width           =   1000
   End
End
Attribute VB_Name = "Form1"

Private Sub Form_Load()
    Text2 = "plain"
    Debug.Print Text2
    Text1(1) = "arr"
    Debug.Print Text1(1)
    Pic1(0) = LoadPicture()
    Pic2 = Pic1(0)
    Pic2.Picture = Pic1(0)
    Pic1(0) = Pic2
    Debug.Print TypeName(Pic1(0).Picture); TypeName(Pic2.Picture)
    Debug.Print Left("hello", 2); Right("hello", 2)
    If Len(Text2) > 100 Then
        Text2 = Text2 & "never" & vbNewLine
    End If
    Dim q
    If Text2 = "" Then
        Debug.Print "empty"
        If Len(q) = 0 Then
            Debug.Print "also empty"
        End If
    End If
    Debug.Print "ok"
End Sub
