Option Explicit
Sub Main()
    Dim b As New Bank
    Dim x As Account, y As Account
    Set x = b.Open("ann", 100)
    Set y = b.Open("bob")
    b.Watch y
    On Error Resume Next
    y.Withdraw 50
    Print Err.Number - vbObjectError, Err.Description, Err.Source
    Err.Clear
    x.TransferTo y, 30.5
    y.Deposit -5
    Print Err.Number - vbObjectError, Err.Description
    On Error GoTo 0
    Print x.Balance, y.Balance, b.Total, b.Count, b.Alerts
    Print b.Richest.Owner, x.History(), y.History()
    Print TypeName(x), x Is b.Richest, y Is b.Richest
End Sub
