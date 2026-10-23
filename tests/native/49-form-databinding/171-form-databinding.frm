VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Bind"
   Begin VB.TextBox Text1
      DataField       =   "PersonName"
      Height          =   300
      Width           =   2000
   End
   Begin VB.TextBox Text2
      DataField       =   "PersonName"
      DataMember      =   "Spouse"
      Height          =   300
      Top             =   400
      Width           =   2000
   End
   Begin VB.CheckBox Check1
      Caption         =   "Active"
      Top             =   800
      Width           =   1000
   End
   Begin VB.Label Label1
      Height          =   300
      Top             =   1200
      Width           =   2000
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Dim p As Person
Private Sub Form_Load()
    Set p = New Person
    p.PersonName = "Ann"
    p.Active = True
    p.Age = 30
    Set p.Spouse = New Person
    p.Spouse.PersonName = "Bob"
    Set Text1.DataSource = p
    Set Text2.DataSource = p
    Check1.DataField = "Active"
    Set Check1.DataSource = p
    Label1.DataField = "Age"
    Set Label1.DataSource = p
    Debug.Print Text1.Text; Text2.Text; Check1.Value; Label1.Caption; Text1.DataChanged
    Text1.Text = "Anna"
    Debug.Print Text1.DataChanged; p.PersonName
End Sub
Private Sub Text1_Change()
    Debug.Print "change "; Text1.Text; Text1.DataChanged
End Sub
Private Sub Form_Unload(Cancel As Integer)
    Debug.Print "unload "; p.PersonName; p.Spouse.PersonName
End Sub
