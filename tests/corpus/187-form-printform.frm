VERSION 5.00
Begin VB.Form Form1
   Caption         =   "Print"
   ClientHeight    =   3000
   ClientWidth     =   4000
End
Attribute VB_Name = "Form1"
Option Explicit
Private Sub Form_Load()
    Printer.Print "before"
    PrintForm
    Me.PrintForm
    Printer.EndDoc
End Sub
