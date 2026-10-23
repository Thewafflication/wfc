Option Explicit
Sub Main()
    Dim sh As Object, env As Object
    Set sh = CreateObject("WScript.Shell")
    Print TypeName(sh) Like "*Wsh*"
    Set env = sh.Environment("Process")
    env("WFC_COM_TEST") = "abc"
    Print env("WFC_COM_TEST"); sh.ExpandEnvironmentStrings("%WFC_COM_TEST%")
    Dim doc As Object, nodes As Object, n As Object
    Set doc = CreateObject("MSXML2.DOMDocument.6.0")
    doc.async = False
    Print doc.loadXML("<r><i n='1'>a</i><i n='2'>b</i></r>")
    Set nodes = doc.selectNodes("//i")
    Print nodes.length
    For Each n In nodes
        Print n.getAttribute("n"); n.Text
    Next
    With doc.documentElement
        Print .nodeName; .childNodes(1).Text
        .setAttribute "k", "v"
        Print .getAttribute("k")
    End With
    Print doc.loadXML("<broken"); doc.parseError.errorCode <> 0
    Dim e1 As Object, e2 As Object
    Set e1 = doc.documentElement
    Set e2 = doc.documentElement
    Print e1 Is e2; e1 Is doc
    Print sh.Run("cmd /c exit 3", 0, True)
    On Error Resume Next
    doc.noSuchMethod 1
    Print Err.Number; Err.Description
    Err.Clear
    sh.RegRead "HKEY_CURRENT_USER" & Chr(92) & "WfcNo" & Chr(92) & "Such"
    Print Err.Number <> 0; Err.Source
    Err.Clear
    Set sh = CreateObject("No.Such.Component")
    Print Err.Number; Err.Description
End Sub
