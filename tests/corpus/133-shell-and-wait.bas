Option Explicit
Private Declare Function OpenProcess Lib "kernel32" (ByVal dwDesiredAccess As Long, ByVal bInheritHandle As Long, ByVal dwProcessId As Long) As Long
Private Declare Function WaitForSingleObject Lib "kernel32" (ByVal hHandle As Long, ByVal dwMilliseconds As Long) As Long
Private Declare Function GetExitCodeProcess Lib "kernel32" (ByVal hProcess As Long, lpExitCode As Long) As Long
Private Declare Function CloseHandle Lib "kernel32" (ByVal hObject As Long) As Long
Const SYNCHRONIZE = &H100000
Const PROCESS_QUERY_INFORMATION = &H400
Const INFINITE = -1&

Function ShellAndWait(cmd As String) As Long
    Dim pid As Long, h As Long, code As Long
    pid = Shell(cmd, vbHide)
    h = OpenProcess(SYNCHRONIZE Or PROCESS_QUERY_INFORMATION, 0, pid)
    WaitForSingleObject h, INFINITE
    GetExitCodeProcess h, code
    CloseHandle h
    ShellAndWait = code
End Function

Sub Main()
    Dim t As Single, f As String
    f = Environ("TEMP") & "\wfc_sh_out.txt"
    Print ShellAndWait("cmd /c exit 3")
    Print ShellAndWait("cmd /c echo done > """ & f & """")
    Print Dir(f) <> ""
    Kill f
    t = Timer
    Print ShellAndWait("cmd /c ping -n 2 127.0.0.1 > nul") , Timer - t > 0.5
    On Error Resume Next
    Shell "definitely_not_a_program.exe"
    Print Err.Number, Err.Description
End Sub
