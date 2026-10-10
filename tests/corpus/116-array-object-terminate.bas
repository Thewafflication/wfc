Option Explicit
Sub Work()
    Dim objs(1 To 2) As Object
    Set objs(1) = New Probe
    Print "in Work"
End Sub
Sub Main()
    Work
    Print "after Work"
    Dim arr() As Probe
    ReDim arr(1 To 1)
    Set arr(1) = New Probe
    Erase arr
    Print "after Erase"
    ReDim arr(1 To 2)
    Set arr(1) = New Probe
    Set arr(2) = New Probe
    ReDim Preserve arr(1 To 1)
    Print "after shrink"
    ReDim arr(1 To 3)
    Print "after ReDim"
End Sub
