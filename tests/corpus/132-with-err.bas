Option Explicit
Sub Main()
    On Error Resume Next
    Error 5
    With Err
        Print .Number, .Description
    End With
    Err.Raise 7, "src", "boom", "help.hlp", 99
    With Err
        Print .Source, .Description, .HelpFile, .HelpContext
        .Clear
        Print .Number, .HelpContext
        .Raise 11
    End With
    Print Err.Number, Err.Description
End Sub
