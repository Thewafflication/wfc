Option Explicit
Sub Main()
    Dim doc As MSXML2.DOMDocument60
    Dim sh As New IWshRuntimeLibrary.WshShell
    Dim node As MSXML2.IXMLDOMNode
    Set doc = New MSXML2.DOMDocument60
    doc.loadXML "<a><b/></a>"
    Set node = doc.documentElement
    Print node.nodeName, node.nodeType = NODE_ELEMENT, TypeName(doc)
    Print sh.ExpandEnvironmentStrings("%OS%")
    Dim d2 As DOMDocument60
    Set d2 = CreateObject("MSXML2.DOMDocument.6.0")
    Print d2 Is Nothing
End Sub
