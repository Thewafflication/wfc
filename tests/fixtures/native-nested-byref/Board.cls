VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "Board"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit
Private m_Cells() As Long
Private m_Width As Byte
Private m_Height As Byte

Public Sub Reset(ByVal aWidth As Byte, ByVal aHeight As Byte)
    m_Width = aWidth
    m_Height = aHeight
    ReDim m_Cells(1 To m_Width, 1 To m_Height)
    m_Cells(1, 1) = CLng(m_Width) * CLng(m_Height)
End Sub

Public Sub Clear()
    m_Cells(1, 1) = 0
End Sub

Public Function Used() As Long
    Dim i As Long, j As Long
    For i = 1 To m_Width
        For j = 1 To m_Height
            If m_Cells(i, j) <> 0 Then Used = Used + 1
        Next
    Next
End Function
