Option Explicit
Sub Main()
    Print Screen.Width > 0; Screen.Height > 0
    Print Screen.TwipsPerPixelX; Screen.TwipsPerPixelY
    Print App.EXEName; App.Major; App.PrevInstance
    App.Title = "native"
    Print App.Title
    Print Screen.MousePointer
    Screen.MousePointer = 11
    Print Screen.MousePointer
    Print Len(App.Path) > 0
End Sub
