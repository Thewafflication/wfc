Option Explicit
Sub Main()
    Printer.FontSize = 12
    Printer.Print "Report"; " title"
    Printer.Print "Total:", 42
    Printer.NewPage
    Printer.Print "Page "; Printer.Page
    Debug.Print Printers.Count; Printer.DeviceName; Printer.Copies; Printer.Orientation
    Debug.Print Printers(0).DeviceName
    Printer.EndDoc
End Sub
