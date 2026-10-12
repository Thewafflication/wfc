VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Common controls"
   Begin MSComctlLib.ProgressBar Prog1
      Height          =   300
      Left            =   120
      Max             =   50
      Top             =   120
      Value           =   10
      Width           =   2400
   End
   Begin MSComctlLib.Slider Slider1
      Height          =   400
      Left            =   120
      Max             =   20
      Top             =   600
      Width           =   2400
   End
   Begin MSComctlLib.ImageList Images1
      Left            =   3000
      Top             =   120
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Debug.Print Prog1.Min; Prog1.Max; Prog1.Value; Prog1.Left; Prog1.Width
    Prog1.Value = 25
    Debug.Print Prog1.Value
    On Error Resume Next
    Prog1.Value = 99
    Debug.Print Err.Number
    Err.Clear
    Prog1.Min = 60
    Debug.Print Err.Number
    Err.Clear
    On Error GoTo 0
    Debug.Print Slider1.Min; Slider1.Max; Slider1.Value; Slider1.LargeChange; Slider1.GetNumTicks
    Slider1.Value = 7
    Slider1.Max = 5
    Debug.Print Slider1.Value; Slider1.Max
    Dim li As Object
    Set li = Images1.ListImages.Add(, "red", LoadPicture(App.Path & "\188-comctl-basic.red.bmp"))
    Debug.Print Images1.ListImages.Count; li.Key; li.Index; Images1.ImageWidth; TypeName(li)
    Set li = Images1.ListImages.Add(1, "second", LoadPicture(App.Path & "\188-comctl-basic.red.bmp"))
    Debug.Print Images1.ListImages("red").Index; Images1.ListImages(1).Key
    On Error Resume Next
    Images1.ListImages.Add , "red", LoadPicture(App.Path & "\188-comctl-basic.red.bmp")
    Debug.Print Err.Number
    Err.Clear
    Debug.Print Images1.ListImages("nope").Key
    Debug.Print Err.Number
    Err.Clear
    Images1.ImageWidth = 32
    Debug.Print Err.Number
    Images1.ListImages.Remove "second"
    Debug.Print Images1.ListImages.Count
    Dim x As Variant
    For Each x In Images1.ListImages
        Debug.Print x.Key
    Next
End Sub
Private Sub Slider1_Change()
    Debug.Print "change"; Slider1.Value
End Sub
Private Sub Slider1_Scroll()
    Debug.Print "scroll"
End Sub
