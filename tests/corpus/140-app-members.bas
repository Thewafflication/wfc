Option Explicit
Sub Main()
    App.Title = "Demo"
    App.HelpFile = "demo.hlp"
    App.TaskVisible = False
    Print App.Title; App.HelpFile; App.TaskVisible; App.StartMode
    Print App.NonModalAllowed; App.UnattendedApp; App.RetainedProject
    Print App.OleServerBusyTimeout; App.OleRequestPendingTimeout
    App.OleServerBusyTimeout = 250
    Print App.OleServerBusyTimeout; App.OleServerBusyRaiseError
    Print App.OleServerBusyMsgTitle; "|"; App.OleRequestPendingMsgTitle
    App.StartLogging "log.txt", 2
    Print App.LogPath; App.LogMode; "[" & App.LegalTrademarks & "]"
End Sub
