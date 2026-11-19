VERSION 5.00
Begin VB.Form FMain
   Caption         =   "Main"
   ClientHeight    =   1000
   ClientWidth     =   2000
End
Attribute VB_Name = "FMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Private m_Board As Board
Private m_P1 As Person
Private m_P2 As Person

Public Sub Run()
    Dim round As Long, junk(1 To 20) As Board, k As Long, bad As Long
    Set m_Board = New Board
    Set m_P1 = New Person
    Set m_P2 = New Person
    For round = 1 To 200
        If FDlg.Fresh(Me, m_Board, m_P1, m_P2) <> vbOK Then bad = bad + 1
        m_Board.Clear
        For k = 1 To 20
            Set junk(k) = New Board
            junk(k).Reset 7, 6
        Next
        If m_Board.Used <> 0 Then bad = bad + 1
    Next
    Print "bad " & bad
    Dim n As Long, txt As String
    n = 10: txt = "t"
    FDlg.Bump n, txt
    Print n & " " & txt
    FDlg.Bump n, txt
    Print n & " " & txt
End Sub
