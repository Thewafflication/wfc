VERSION 5.00
Object = "{248DD890-BB45-11CF-9ABC-0080C7E7B78D}#1.0#0"; "MSWINSCK.OCX"
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Begin VB.Form Form1
   Caption         =   "Winsock and Common Dialog"
   ClientHeight    =   3000
   ClientWidth     =   4000
   Begin MSComDlg.CommonDialog Dialog1
      Left            =   120
      Top             =   120
      _ExtentX        =   847
      _ExtentY        =   847
   End
   Begin MSWinsockLib.Winsock wsListen
      Left            =   720
      Top             =   120
   End
   Begin MSWinsockLib.Winsock wsClient
      Left            =   1320
      Top             =   120
   End
   Begin MSWinsockLib.Winsock wsServer
      Left            =   1920
      Top             =   120
   End
   Begin MSWinsockLib.Winsock wsUdpA
      Left            =   2520
      Top             =   120
      Protocol        =   1
   End
   Begin MSWinsockLib.Winsock wsUdpB
      Left            =   3120
      Top             =   120
      Protocol        =   1
   End
End
Attribute VB_Name = "Form1"
Option Explicit
Private mGot As String

Private Sub Form_Load()
    On Error Resume Next
    Debug.Print "state"; wsClient.State; wsClient.Protocol; wsUdpA.Protocol
    wsClient.SendData "x"
    Debug.Print "send closed"; Err.Number
    Err.Clear
    wsListen.LocalPort = 0
    wsListen.Listen
    Debug.Print "listening"; wsListen.State; (wsListen.LocalPort > 0)
    wsClient.Connect "127.0.0.1", wsListen.LocalPort
    Debug.Print "connecting"; wsClient.State
    wsClient.Connect "127.0.0.1", 1
    Debug.Print "connect twice"; Err.Number
    Err.Clear
    Dim ip As String
    ip = wsClient.LocalIP
    Debug.Print "localip"; (Len(ip) > 0); (Len(wsClient.LocalHostName) > 0)
    wsUdpA.Bind 0, "127.0.0.1"
    wsUdpB.RemoteHost = "127.0.0.1"
    wsUdpB.RemotePort = wsUdpA.LocalPort
    Debug.Print "udp"; wsUdpA.State
End Sub

Private Sub wsListen_ConnectionRequest(ByVal requestID As Long)
    Debug.Print "request"
    wsServer.Accept requestID
    Debug.Print "accepted"; wsServer.State
End Sub

Private Sub wsClient_Connect()
    Debug.Print "client connected"; wsClient.State
    wsClient.SendData "hello"
End Sub

Private Sub wsClient_SendComplete()
    Debug.Print "sent"
End Sub

Private Sub wsServer_DataArrival(ByVal bytesTotal As Long)
    Dim s As String
    wsServer.GetData s
    Debug.Print "server got "; s; bytesTotal
    wsServer.SendData "reply:" & s
End Sub

Private Sub wsClient_DataArrival(ByVal bytesTotal As Long)
    Dim s As String
    wsClient.GetData s, vbString, 5
    Debug.Print "client got "; s; bytesTotal; wsClient.BytesReceived
    wsClient.GetData s
    Debug.Print "rest "; s
    wsClient.Close
    Debug.Print "closed"; wsClient.State
End Sub

Private Sub wsServer_Close()
    Debug.Print "server sees close"; wsServer.State
    wsServer.Close
    wsUdpB.SendData "datagram"
End Sub

Private Sub wsUdpA_DataArrival(ByVal bytesTotal As Long)
    Dim s As String
    wsUdpA.GetData s
    Debug.Print "udp got "; s; bytesTotal; wsUdpA.RemoteHostIP
    Debug.Print "dialogs"
    Dialog1.CancelError = True
    Dialog1.Filter = "Text (*.txt)|*.txt|All|*.*"
    On Error Resume Next
    Dialog1.ShowOpen
    Debug.Print "open"; Err.Number; Dialog1.FileName; Dialog1.FileTitle
    Err.Clear
    Dialog1.ShowOpen
    Debug.Print "cancelled"; Err.Number
    Err.Clear
    Dialog1.Color = &H80000006
    Dialog1.ShowColor
    Debug.Print "color"; Hex(Dialog1.Color)
    Dialog1.ShowFont
    Debug.Print "font none"; Err.Number
    Err.Clear
    Dialog1.Flags = 1
    Dialog1.ShowFont
    Debug.Print "font"; Err.Number; Dialog1.FontName; Dialog1.FontSize; Dialog1.FontBold; Dialog1.FontItalic
    Err.Clear
    Dialog1.CancelError = False
    Dialog1.ShowSave
    Debug.Print "save cancelled quietly"; Err.Number
End Sub

Private Sub wsClient_Error(ByVal Number As Integer, Description As String, ByVal Scode As Long, ByVal Source As String, ByVal HelpFile As String, ByVal HelpContext As Long, CancelDisplay As Boolean)
    Debug.Print "error"; Number
End Sub
