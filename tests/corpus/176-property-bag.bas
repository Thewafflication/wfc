Option Explicit
Sub Main()
    Dim a As New PropertyBag
    Dim b As PropertyBag
    a.WriteProperty "Caption", "Hello = world" & vbCrLf & "x\y"
    a.WriteProperty "Count", 42&
    a.WriteProperty "Ratio", 2.5
    a.WriteProperty "On", True
    a.WriteProperty "Skip", 7, 7
    a.WriteProperty "Keep", 8, 7
    a.WriteProperty "When", #3/4/2001#
    Set b = New PropertyBag
    b.Contents = a.Contents
    Debug.Print b.ReadProperty("Caption") = "Hello = world" & vbCrLf & "x\y"
    Debug.Print b.ReadProperty("Count"); TypeName(b.ReadProperty("Count"))
    Debug.Print b.ReadProperty("Ratio"); b.ReadProperty("On"); b.ReadProperty("Keep")
    Debug.Print b.ReadProperty("Skip", 99); IsEmpty(b.ReadProperty("Missing"))
    Debug.Print b.ReadProperty("When")
    Debug.Print UBound(a.Contents) > 10
End Sub
