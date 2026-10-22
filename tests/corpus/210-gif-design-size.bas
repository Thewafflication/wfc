Option Explicit
' A GIF may state the size it is laid out at (a "wfc:size" comment), so a
' sprite with more pixels keeps its place: its width and height (in
' himetric) are those of the stated size, not of its pixels.
Sub Main()
    Dim p As StdPicture
    Set p = LoadPicture(App.Path & "\210-gif-design-size.plain.gif")
    Print p.Width; p.Height
    Set p = LoadPicture(App.Path & "\210-gif-design-size.sprite.gif")
    Print p.Width; p.Height
End Sub
