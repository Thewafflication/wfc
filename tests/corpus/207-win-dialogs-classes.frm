VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Dialogs"
   ClientHeight    =   3000
   ClientWidth     =   3000
   Begin VB.CommandButton Command1
      Caption         =   "Go"
      Height          =   400
      Left            =   100
      Top             =   100
      Width           =   900
   End
End
Attribute VB_Name = "Form1"
Private Sub Command1_Click()
    Dim c As New ColorDialog
    Dim o As New OpenFileDialog
    Dim s As New SaveFileDialog
    Dim f As New FontDialog
    c.Color = 255
    Print c.ShowDialog = vbOK; c.Color
    o.Filter = "Text|*.txt|All|*.*"
    o.InitialDirectory = "C:\"
    Print o.ShowDialog(Me) = VbMsgBoxResult.vbOK; o.FileName; o.FilterIndex
    Print o.SafeFileName
    s.DefaultExt = ".smbk"
    Print s.ShowDialog = vbOK; s.FileName
    f.FontName = "Arial"
    Print f.ShowDialog = vbOK; f.FontName; f.FontSize; f.FontBold
    Print ColorConstants.vbYellow; VbMsgBoxResult.vbCancel
    c.Color = 7
    Print c.ShowDialog = vbCancel; c.Color
End Sub
