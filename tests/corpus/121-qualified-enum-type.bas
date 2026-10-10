Option Explicit
Public Sub Main()
    Dim c As New Cfg, d As New Cfg
    c.Current = ModeB
    Print c.Current, Cfg.ModeA, c.Version, c.Sec, c.Hidden
    Print c.Bump, c.Bump, d.Bump
    Dim m As Cfg.Mode
    m = ModeA
    Print m
End Sub
