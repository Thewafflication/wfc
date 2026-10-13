VERSION 5.00
Begin VB.Form Form1
   Caption         =   "GDI through Declare"
   ClientHeight    =   2400
   ClientWidth     =   4800
   ScaleMode       =   3  'Pixel
   Begin VB.Timer Timer1
      Interval        =   300
   End
   Begin VB.PictureBox Source
      AutoRedraw      =   -1  'True
      BackColor       =   &H00FFFFFF&
      Height          =   1500
      Left            =   120
      ScaleMode       =   3  'Pixel
      Top             =   120
      Width           =   1500
   End
   Begin VB.PictureBox Target
      AutoRedraw      =   -1  'True
      BackColor       =   &H00FFFFFF&
      Height          =   1500
      Left            =   2400
      ScaleMode       =   3  'Pixel
      Top             =   120
      Width           =   1500
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private Declare Function BitBlt Lib "gdi32" (ByVal hDestDC As Long, ByVal x As Long, ByVal y As Long, ByVal nWidth As Long, ByVal nHeight As Long, ByVal hSrcDC As Long, ByVal xSrc As Long, ByVal ySrc As Long, ByVal dwRop As Long) As Long
Private Declare Function SetPixelV Lib "gdi32" (ByVal hdc As Long, ByVal x As Long, ByVal y As Long, ByVal crColor As Long) As Long
Private Declare Function GetPixel Lib "gdi32" (ByVal hdc As Long, ByVal x As Long, ByVal y As Long) As Long
Private n As Long
Private Sub Timer1_Timer()
    n = n + 1
    If n = 1 Then
        Source.Line (10, 10)-(60, 60), RGB(255, 0, 0), BF
        Dim ok As Long
        ok = SetPixelV(Source.hdc, 70, 70, RGB(0, 0, 255))
        ok = BitBlt(Target.hdc, 5, 5, 100, 100, Source.hdc, 0, 0, &HCC0020)
        Debug.Print "blt"; ok
        Debug.Print "red"; (GetPixel(Target.hdc, 20, 20) = RGB(255, 0, 0))
        Debug.Print "blue"; (GetPixel(Target.hdc, 75, 75) = RGB(0, 0, 255))
        Debug.Print "white"; (GetPixel(Target.hdc, 2, 2) = RGB(255, 255, 255))
        Debug.Print "point"; (Target.Point(20, 20) = RGB(255, 0, 0))
    End If
    If n = 3 Then Unload Me
End Sub
