VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Pics"
   ClientHeight    =   3000
   ClientWidth     =   4000
   Begin VB.PictureBox Picture1
      AutoSize        =   -1  'True
      Height          =   300
      Left            =   100
      Top             =   100
      Width           =   300
   End
   Begin VB.PictureBox Picture2
      Height          =   500
      Left            =   2000
      Top             =   100
      Width           =   500
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Activate()
    Dim p As StdPicture
    Print Picture1.Picture.Handle; Picture1.Picture.Type
    Set p = LoadPicture(App.Path & "\162-form-pictures.red.bmp")
    Print p.Type; p.Width; p.Height; (p.Handle <> 0)
    Set Picture1.Picture = p
    Print Picture1.Width; Picture1.Height; Picture1.Picture.Type
    Picture2.Picture = Picture1.Picture
    Set Picture1.Picture = LoadPicture("")
    Print Picture1.Picture.Handle
    SavePicture p, Environ("TEMP") & "\wfc_copy.bmp"
    Set Picture2.Picture = LoadPicture(Environ("TEMP") & "\wfc_copy.bmp")
    Print Picture2.Picture.Width
    On Error Resume Next
    Set p = LoadPicture("missing.bmp")
    Print Err.Number; Err.Description
    Err.Clear
    Picture2.Picture = 5
    Print Err.Number
    Picture2.Cls
    Kill Environ("TEMP") & "\wfc_copy.bmp"
End Sub
