Option Explicit
Sub Main()
    Licenses.Add "Vendor.Grid", "KEY-1"
    Licenses.Add "Vendor.Chart"
    Debug.Print Licenses.Count; Licenses("Vendor.Grid"); Licenses(2) = ""
    On Error Resume Next
    Licenses.Add "vendor.grid", "x"
    Debug.Print Err.Number
    Err.Clear
    Licenses.Remove "Vendor.Grid"
    Debug.Print Licenses.Count
End Sub
