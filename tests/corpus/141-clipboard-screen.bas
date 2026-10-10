Option Explicit
Sub Main()
    Clipboard.Clear
    Print Clipboard.GetFormat(vbCFText); "[" & Clipboard.GetText & "]"
    Clipboard.SetText "hello clip"
    Print Clipboard.GetFormat(vbCFText); Clipboard.GetText
    Print Screen.TwipsPerPixelX > 0; Screen.TwipsPerPixelY > 0
    Print Screen.Width > 0; Screen.Height > 0
    Print Screen.FontCount > 0; Len(Screen.Fonts(0)) > 0
    Screen.MousePointer = vbHourglass
    Print Screen.MousePointer; Screen.ActiveForm Is Nothing
End Sub
