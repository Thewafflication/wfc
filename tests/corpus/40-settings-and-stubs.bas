Sub Main()
    SaveSetting "app", "sec", "k1", "v1"
    SaveSetting "app", "sec", "k2", "v2"
    Dim a As Variant
    a = GetAllSettings("app", "sec")
    Print UBound(a, 1), a(0, 0), a(1, 1)
    AppActivate "x"
    SendKeys "abc", True
    Print IsEmpty(GetAllSettings("app", "none"))
End Sub
