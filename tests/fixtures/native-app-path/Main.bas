Attribute VB_Name = "Main"
Option Explicit

Sub Main()
    ' App.Path is where the program is, not where it was built: the files the
    ' project carries are found beside it.
    Print "reads " & (Dir$(App.Path & "\AppPath.vbp") <> "")
End Sub
