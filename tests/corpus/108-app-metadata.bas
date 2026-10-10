Attribute VB_Name = "modMain"
Option Explicit
Public Sub Main()
    Print App.Title, App.EXEName, App.Major; App.Minor; App.Revision
    Print App.CompanyName & "|" & App.ProductName & "|" & App.FileDescription
    Print App.LegalCopyright & "|" & App.Comments
End Sub
