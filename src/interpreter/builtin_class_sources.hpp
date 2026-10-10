// VB source text of the built-in classes (Collection, Dictionary,
// FileSystemObject, RegExp, ...). Internal to the WFC evaluator; not part of
// the public API. Split out of src/evaluator.cpp; see
// src/interpreter/README.md.

#ifndef WFC_INTERPRETER_BUILTIN_CLASS_SOURCES_HPP
#define WFC_INTERPRETER_BUILTIN_CLASS_SOURCES_HPP

#include <string_view>

namespace wfc::detail {

// REQ-0243: the built-in `Collection`, written in the evaluator's own VB
// dialect and registered on demand; its storage is the native `WfcStore`.

inline constexpr std::string_view kCollectionSource =
    R"VB(Private Sub Class_Initialize()
Dim r As Long
r = WfcStore(10, Me, 1)
End Sub
Public Sub Add(ByVal Item As Variant, Optional Key As String = "", Optional)VB"
    R"VB( Before As Variant, Optional After As Variant)
Dim pos As Long
Dim k As Variant
If Not IsMissing(Before) And Not IsMissing(After) Then Err.Raise 5, ,)VB"
    R"VB( "Invalid procedure call or argument"
If Key <> "" Then
If WfcStore(2, Me, Key) > 0 Then Err.Raise 457, , "This key is already)VB"
    R"VB( associated with an element of this collection"
k = Key
End If
pos = 0
If Not IsMissing(Before) Then pos = Locate(Before)
If Not IsMissing(After) Then pos = Locate(After) + 1
pos = WfcStore(3, Me, k, Item, pos)
End Sub
Public Property Get Count() As Long
Count = WfcStore(1, Me)
End Property
Public Function Item(Index As Variant) As Variant
Attribute Item.VB_UserMemId = 0
Dim p As Long
p = Locate(Index)
If WfcStore(11, Me, p) Then
Set Item = WfcStore(4, Me, p)
Else
Item = WfcStore(4, Me, p)
End If
End Function
Public Sub Remove(Index As Variant)
Dim p As Long
p = Locate(Index)
p = WfcStore(8, Me, p)
End Sub
Private Function Locate(Index As Variant) As Long
Dim p As Long
If VarType(Index) = 8 Then
p = WfcStore(2, Me, Index)
If p = 0 Then Err.Raise 5, , "Invalid procedure call or argument"
Locate = p
Exit Function
End If
If Index < 1 Or Index > WfcStore(1, Me) Then Err.Raise 9, , "Subscript out)VB"
    R"VB( of range"
Locate = CLng(Index)
End Function
Public Function NewEnum() As Object
Set NewEnum = Me
End Function
Public Function WfcItems() As Variant
WfcItems = WfcStore(12, Me)
End Function
)VB";

// Scripting.Dictionary, provided as VB source like Collection (case-sensitive
// keys unless CompareMode = 1; reading a missing key adds an Empty entry, as
// the real object does).
inline constexpr std::string_view kDictionarySource =
    R"VB(Private mode As Long
Public Property Get CompareMode() As Long
CompareMode = mode
End Property
Public Property Let CompareMode(v As Long)
Dim r As Long
If WfcStore(1, Me) > 0 Then Err.Raise 5, , "Invalid procedure call or)VB"
    R"VB( argument"
mode = v
r = WfcStore(10, Me, v)
End Property
Public Property Get Count() As Long
Count = WfcStore(1, Me)
End Property
Private Function IndexOf(Key As Variant) As Long
IndexOf = WfcStore(2, Me, Key)
End Function
Public Sub Add(Key As Variant, Item As Variant)
Dim r As Long
If IndexOf(Key) > 0 Then Err.Raise 457, "Scripting.Dictionary", "This key)VB"
    R"VB( is already associated with an element of this collection"
r = WfcStore(3, Me, Key, Item, 0)
End Sub
Public Function Exists(Key As Variant) As Boolean
Exists = IndexOf(Key) > 0
End Function
Public Property Get Item(Key As Variant) As Variant
Attribute Item.VB_UserMemId = 0
Dim i As Long
Dim r As Long
i = IndexOf(Key)
If i = 0 Then
r = WfcStore(3, Me, Key, Empty, 0)
i = WfcStore(1, Me)
End If
If WfcStore(11, Me, i) Then
Set Item = WfcStore(4, Me, i)
Else
Item = WfcStore(4, Me, i)
End If
End Property
Public Property Let Item(Key As Variant, NewItem As Variant)
Dim i As Long
Dim r As Long
i = IndexOf(Key)
If i = 0 Then
r = WfcStore(3, Me, Key, NewItem, 0)
Else
r = WfcStore(6, Me, i, NewItem)
End If
End Property
Public Property Set Item(Key As Variant, NewItem As Object)
Dim i As Long
Dim r As Long
i = IndexOf(Key)
If i = 0 Then
r = WfcStore(3, Me, Key, NewItem, 0)
Else
r = WfcStore(6, Me, i, NewItem)
End If
End Property
Public Property Let Key(OldKey As Variant, NewKey As Variant)
Dim i As Long
Dim r As Long
i = IndexOf(OldKey)
If i = 0 Then Err.Raise 32811, "Scripting.Dictionary", "Element not found"
If IndexOf(NewKey) > 0 Then Err.Raise 457, "Scripting.Dictionary", "This)VB"
    R"VB( key is already associated with an element of this collection"
r = WfcStore(7, Me, i, NewKey)
End Property
Public Sub Remove(Key As Variant)
Dim i As Long
i = IndexOf(Key)
If i = 0 Then Err.Raise 32811, "Scripting.Dictionary", "Element not found"
i = WfcStore(8, Me, i)
End Sub
Public Sub RemoveAll()
Dim r As Long
r = WfcStore(9, Me)
End Sub
Public Function Keys() As Variant
Keys = WfcStore(13, Me)
End Function
Public Function Items() As Variant
Items = WfcStore(12, Me)
End Function
Public Function WfcItems() As Variant
WfcItems = WfcStore(13, Me)
End Function
)VB";

// The global `App` object (the properties a console-style program reads).
inline constexpr std::string_view kClipboardSource =
    R"VB(Private mText As String
Private mHasText As Boolean
Public Sub Clear()
mText = ""
mHasText = False
End Sub
Public Sub SetText(ByVal Str As String, Optional ByVal Format As Variant)
mText = Str
mHasText = True
End Sub
Public Function GetText(Optional ByVal Format As Variant) As String
GetText = mText
End Function
Public Function GetFormat(ByVal Format As Integer) As Boolean
GetFormat = mHasText And (Format = 1 Or Format = 13)
End Function
)VB";

inline constexpr std::string_view kScreenSource =
    R"VB(Private mPointer As Integer
Public Property Get Width() As Single
Width = WfcSys(0) * TwipsPerPixelX
End Property
Public Property Get Height() As Single
Height = WfcSys(1) * TwipsPerPixelY
End Property
Public Property Get TwipsPerPixelX() As Single
TwipsPerPixelX = 1440 / WfcSys(2)
End Property
Public Property Get TwipsPerPixelY() As Single
TwipsPerPixelY = 1440 / WfcSys(3)
End Property
Public Property Get FontCount() As Integer
FontCount = WfcSys(4)
End Property
Public Property Get Fonts(ByVal Index As Integer) As String
Fonts = WfcSysFont(Index)
End Property
Public Property Get MousePointer() As Integer
MousePointer = mPointer
End Property
Public Property Let MousePointer(ByVal v As Integer)
mPointer = v
End Property
Public Property Get ActiveForm() As Object
Set ActiveForm = Nothing
End Property
Public Property Get ActiveControl() As Object
Set ActiveControl = Nothing
End Property
)VB";

inline constexpr std::string_view kAppSource =
    R"VB(Public Property Get Path() As String
Path = CurDir$
End Property
Private mTitle As String
Private mTitleSet As Boolean
Public Property Get Title() As String
If mTitleSet Then
Title = mTitle
Else
Title = ""
End If
End Property
Public Property Let Title(ByVal v As String)
mTitle = v
mTitleSet = True
End Property
Public Property Get EXEName() As String
EXEName = "Project1"
End Property
Public Property Get PrevInstance() As Boolean
PrevInstance = False
End Property
Public Property Get Major() As Long
Major = 1
End Property
Public Property Get Minor() As Long
Minor = 0
End Property
Public Property Get Revision() As Long
Revision = 0
End Property
Public Property Get hInstance() As Long
hInstance = 0
End Property
Public Property Get ThreadID() As Long
ThreadID = 0
End Property
Public Property Get CompanyName() As String
CompanyName = ""
End Property
Public Property Get ProductName() As String
ProductName = ""
End Property
Public Property Get FileDescription() As String
FileDescription = ""
End Property
Public Property Get Comments() As String
Comments = ""
End Property
Public Property Get LegalCopyright() As String
LegalCopyright = ""
End Property
Public Property Get LegalTrademarks() As String
LegalTrademarks = ""
End Property
Private mHelpFile As String
Public Property Get HelpFile() As String
HelpFile = mHelpFile
End Property
Public Property Let HelpFile(ByVal v As String)
mHelpFile = v
End Property
Private mTaskVisible As Boolean
Private mTaskSet As Boolean
Public Property Get TaskVisible() As Boolean
If mTaskSet Then
TaskVisible = mTaskVisible
Else
TaskVisible = True
End If
End Property
Public Property Let TaskVisible(ByVal v As Boolean)
mTaskVisible = v
mTaskSet = True
End Property
Public Property Get StartMode() As Integer
StartMode = 0
End Property
Public Property Get NonModalAllowed() As Boolean
NonModalAllowed = True
End Property
Public Property Get UnattendedApp() As Boolean
UnattendedApp = False
End Property
Public Property Get RetainedProject() As Boolean
RetainedProject = False
End Property
Private mLogMode As Long
Private mLogPath As String
Public Property Get LogMode() As Long
LogMode = mLogMode
End Property
Public Property Get LogPath() As String
LogPath = mLogPath
End Property
Public Sub StartLogging(LogTarget As String, LogModes As Long)
mLogPath = LogTarget
mLogMode = LogModes
End Sub
Private mBusyTimeout As Long
Private mBusyTimeoutSet As Boolean
Public Property Get OleServerBusyTimeout() As Long
If mBusyTimeoutSet Then
OleServerBusyTimeout = mBusyTimeout
Else
OleServerBusyTimeout = 10000
End If
End Property
Public Property Let OleServerBusyTimeout(ByVal v As Long)
mBusyTimeout = v
mBusyTimeoutSet = True
End Property
Private mBusyRaise As Boolean
Public Property Get OleServerBusyRaiseError() As Boolean
OleServerBusyRaiseError = mBusyRaise
End Property
Public Property Let OleServerBusyRaiseError(ByVal v As Boolean)
mBusyRaise = v
End Property
Private mBusyTitle As String
Private mBusyTitleSet As Boolean
Public Property Get OleServerBusyMsgTitle() As String
If mBusyTitleSet Then
OleServerBusyMsgTitle = mBusyTitle
Else
OleServerBusyMsgTitle = "Server Busy"
End If
End Property
Public Property Let OleServerBusyMsgTitle(ByVal v As String)
mBusyTitle = v
mBusyTitleSet = True
End Property
Private mBusyText As String
Private mBusyTextSet As Boolean
Public Property Get OleServerBusyMsgText() As String
If mBusyTextSet Then
OleServerBusyMsgText = mBusyText
Else
OleServerBusyMsgText = "This action cannot be completed because the other " & _
"application is busy. Choose 'Switch To' to activate the busy application " & _
"and correct the problem."
End If
End Property
Public Property Let OleServerBusyMsgText(ByVal v As String)
mBusyText = v
mBusyTextSet = True
End Property
Private mPendingTimeout As Long
Private mPendingTimeoutSet As Boolean
Public Property Get OleRequestPendingTimeout() As Long
If mPendingTimeoutSet Then
OleRequestPendingTimeout = mPendingTimeout
Else
OleRequestPendingTimeout = 5000
End If
End Property
Public Property Let OleRequestPendingTimeout(ByVal v As Long)
mPendingTimeout = v
mPendingTimeoutSet = True
End Property
Private mPendingTitle As String
Private mPendingTitleSet As Boolean
Public Property Get OleRequestPendingMsgTitle() As String
If mPendingTitleSet Then
OleRequestPendingMsgTitle = mPendingTitle
Else
OleRequestPendingMsgTitle = "Component Request Pending"
End If
End Property
Public Property Let OleRequestPendingMsgTitle(ByVal v As String)
mPendingTitle = v
mPendingTitleSet = True
End Property
Private mPendingText As String
Private mPendingTextSet As Boolean
Public Property Get OleRequestPendingMsgText() As String
If mPendingTextSet Then
OleRequestPendingMsgText = mPendingText
Else
OleRequestPendingMsgText = "An action cannot be completed because a " & _
"component is not responding. Choose 'Switch To' to activate the " & _
"component and correct the problem."
End If
End Property
Public Property Let OleRequestPendingMsgText(ByVal v As String)
mPendingText = v
mPendingTextSet = True
End Property
Public Sub LogEvent(LogBuffer As String, Optional EventType As Long = 1)
End Sub
)VB";

// Scripting.FileSystemObject and its TextStream / File objects, written in VB
// on top of the native file statements.
inline constexpr std::string_view kTextStreamSource =
    R"VB(Private fnum As Integer
Private mLine As Long
Private isOpen As Boolean
Public Sub Init(ByVal path As String, ByVal m As Long)
fnum = FreeFile
Select Case m
Case 1
Open path For Input As #fnum
Case 2
Open path For Output As #fnum
Case Else
Open path For Append As #fnum
End Select
isOpen = True
End Sub
Public Sub Write(ByVal s As String)
Print #fnum, s;
End Sub
Public Sub WriteLine(Optional ByVal s As String = "")
Print #fnum, s
End Sub
Public Sub WriteBlankLines(ByVal n As Long)
Dim i As Long
For i = 1 To n
Print #fnum, ""
Next i
End Sub
Public Function ReadLine() As String
Line Input #fnum, ReadLine
mLine = mLine + 1
End Function
Public Function ReadAll() As String
Dim t As String, first As Boolean
first = True
Do While Not EOF(fnum)
Line Input #fnum, t
If Not first Then ReadAll = ReadAll & vbCrLf
ReadAll = ReadAll & t
first = False
mLine = mLine + 1
Loop
End Function
Public Function Read(ByVal n As Long) As String
Read = Input$(n, #fnum)
End Function
Public Sub SkipLine()
Dim t As String
Line Input #fnum, t
mLine = mLine + 1
End Sub
Public Property Get AtEndOfStream() As Boolean
AtEndOfStream = EOF(fnum)
End Property
Public Property Get Line() As Long
Line = mLine + 1
End Property
Public Sub Close()
If isOpen Then Close #fnum
isOpen = False
End Sub
Private Sub Class_Terminate()
If isOpen Then Close #fnum
End Sub
)VB";

inline constexpr std::string_view kFileObjectSource =
    R"VB(Private mPath As String
Public Sub Init(ByVal path As String)
mPath = path
End Sub
Public Property Get Path() As String
Path = mPath
End Property
Public Property Get Name() As String
Dim i As Long
i = InStrRev(mPath, "\")
If InStrRev(mPath, "/") > i Then i = InStrRev(mPath, "/")
Name = Mid$(mPath, i + 1)
End Property
Public Property Get Size() As Long
Size = FileLen(mPath)
End Property
Public Property Get DateLastModified() As Date
DateLastModified = FileDateTime(mPath)
End Property
Public Sub Delete()
Kill mPath
End Sub
Public Property Get Attributes() As Long
Attributes = GetAttr(mPath)
End Property
Public Property Get DateCreated() As Date
DateCreated = FileDateTime(mPath)
End Property
Public Property Get DateLastAccessed() As Date
DateLastAccessed = FileDateTime(mPath)
End Property
Public Sub Copy(ByVal dst As String, Optional ByVal overwrite As Boolean = True)
FileCopy mPath, dst
End Sub
Public Sub Move(ByVal dst As String)
Name mPath As dst
mPath = dst
End Sub
)VB";

// A Scripting.Folder: path, name, and the files and folders directly inside.
inline constexpr std::string_view kFolderObjectSource =
    R"VB(Private mPath As String
Public Sub Init(ByVal path As String)
If Len(path) > 3 And (Right$(path, 1) = "\" Or Right$(path, 1) = "/") Then
path = Left$(path, Len(path) - 1)
End If
mPath = path
End Sub
Public Property Get Path() As String
Attribute Path.VB_UserMemId = 0
Path = mPath
End Property
Public Property Get Name() As String
Dim i As Long
i = InStrRev(mPath, "\")
If InStrRev(mPath, "/") > i Then i = InStrRev(mPath, "/")
Name = Mid$(mPath, i + 1)
End Property
Public Property Get DateLastModified() As Date
DateLastModified = FileDateTime(mPath)
End Property
Public Property Get Attributes() As Long
Attributes = GetAttr(mPath)
End Property
Public Property Get Files() As Object
Dim c As New Collection, n As String, f As Object
n = Dir$(mPath & "\*", 0)
Do While n <> ""
Set f = New WfcFile
f.Init mPath & "\" & n
c.Add f
n = Dir$
Loop
Set Files = c
End Property
Public Property Get SubFolders() As Object
Dim c As New Collection, n As String, f As Object, names() As String
Dim cnt As Long, i As Long
ReDim names(0 To 0)
n = Dir$(mPath & "\*", 16)
Do While n <> ""
If n <> "." And n <> ".." Then
If (GetAttr(mPath & "\" & n) And 16) <> 0 Then
cnt = cnt + 1
ReDim Preserve names(0 To cnt)
names(cnt) = n
End If
End If
n = Dir$
Loop
For i = 1 To cnt
Set f = New WfcFolder
f.Init mPath & "\" & names(i)
c.Add f
Next i
Set SubFolders = c
End Property
Public Sub Delete()
RmDir mPath
End Sub
)VB";

inline constexpr std::string_view kFileSystemObjectSource =
    R"VB(Public Function FileExists(ByVal path As String) As Boolean
On Error Resume Next
Err.Clear
FileExists = ((GetAttr(path) And 16) = 0)
If Err.Number <> 0 Then FileExists = False
End Function
Public Function FolderExists(ByVal path As String) As Boolean
On Error Resume Next
Err.Clear
FolderExists = ((GetAttr(path) And 16) <> 0)
If Err.Number <> 0 Then FolderExists = False
End Function
Private Function LastSep(ByVal path As String) As Long
LastSep = InStrRev(path, "\")
If InStrRev(path, "/") > LastSep Then LastSep = InStrRev(path, "/")
End Function
Public Function GetFileName(ByVal path As String) As String
GetFileName = Mid$(path, LastSep(path) + 1)
End Function
Public Function GetExtensionName(ByVal path As String) As String
Dim n As String, i As Long
n = GetFileName(path)
i = InStrRev(n, ".")
If i > 0 Then GetExtensionName = Mid$(n, i + 1)
End Function
Public Function GetBaseName(ByVal path As String) As String
Dim n As String, i As Long
n = GetFileName(path)
i = InStrRev(n, ".")
If i > 0 Then GetBaseName = Left$(n, i - 1) Else GetBaseName = n
End Function
Public Function GetParentFolderName(ByVal path As String) As String
Dim i As Long
i = LastSep(path)
If i > 1 Then GetParentFolderName = Left$(path, i - 1) Else)VB"
    R"VB( GetParentFolderName = ""
End Function
Public Function BuildPath(ByVal a As String, ByVal b As String) As String
If a = "" Then
BuildPath = b
ElseIf Right$(a, 1) = "\" Or Right$(a, 1) = "/" Then
BuildPath = a & b
Else
BuildPath = a & "\" & b
End If
End Function
Public Function GetAbsolutePathName(ByVal path As String) As String
If Mid$(path, 2, 1) = ":" Or Left$(path, 1) = "\" Or Left$(path, 1) = "/")VB"
    R"VB( Then
GetAbsolutePathName = path
Else
GetAbsolutePathName = BuildPath(CurDir$, path)
End If
End Function
Public Function GetTempName() As String
GetTempName = "rad" & Hex$(Int(Rnd * 65535)) & ".tmp"
End Function
Public Function CreateTextFile(ByVal path As String, Optional ByVal)VB"
    R"VB( overwrite As Boolean = True) As Object
Dim t As New WfcTextStream
If Not overwrite And FileExists(path) Then Err.Raise 58,)VB"
    R"VB( "FileSystemObject", "File already exists"
t.Init path, 2
Set CreateTextFile = t
End Function
Public Function OpenTextFile(ByVal path As String, Optional ByVal mode As)VB"
    R"VB( Long = 1, Optional ByVal create As Boolean = False) As Object
Dim t As New WfcTextStream
If mode = 1 And Not FileExists(path) Then Err.Raise 53, "FileSystemObject",)VB"
    R"VB( "File not found"
t.Init path, mode
Set OpenTextFile = t
End Function
Public Function GetFile(ByVal path As String) As Object
Dim f As New WfcFile
If Not FileExists(path) Then Err.Raise 53, "FileSystemObject", "File not)VB"
    R"VB( found"
f.Init path
Set GetFile = f
End Function
Public Sub DeleteFile(ByVal path As String)
Kill path
End Sub
Public Sub CopyFile(ByVal src As String, ByVal dst As String, Optional)VB"
    R"VB( ByVal overwrite As Boolean = True)
If Not overwrite And FileExists(dst) Then Err.Raise 58, "FileSystemObject",)VB"
    R"VB( "File already exists"
FileCopy src, dst
End Sub
Public Sub MoveFile(ByVal src As String, ByVal dst As String)
Name src As dst
End Sub
Public Function CreateFolder(ByVal path As String) As Object
Dim f As New WfcFolder
MkDir path
f.Init path
Set CreateFolder = f
End Function
Public Function GetFolder(ByVal path As String) As Object
Dim f As New WfcFolder
If Not FolderExists(path) Then Err.Raise 76, "FileSystemObject", "Path not)VB"
    R"VB( found"
f.Init path
Set GetFolder = f
End Function
Public Function GetSpecialFolder(ByVal which As Long) As Object
Dim f As New WfcFolder, p As String
Select Case which
Case 0
p = Environ$("SystemRoot")
Case 1
p = Environ$("SystemRoot") & "\System32"
Case Else
p = Environ$("TEMP")
End Select
f.Init p
Set GetSpecialFolder = f
End Function
Public Function GetDriveName(ByVal path As String) As String
If Mid$(path, 2, 1) = ":" Then GetDriveName = Left$(path, 2)
End Function
Public Sub DeleteFolder(ByVal path As String, Optional ByVal force As)VB"
    R"VB( Boolean = False)
Dim f As Object, x As Object
Set f = GetFolder(path)
For Each x In f.Files
Kill x.Path
Next x
For Each x In f.SubFolders
DeleteFolder x.Path
Next x
RmDir path
End Sub
)VB";

// VBScript.RegExp, written in VB over two native helpers (WfcRegexMatches /
// WfcRegexReplace, std::regex ECMAScript flavor).
inline constexpr std::string_view kRegExpSource = R"VB(Public Pattern As String
Public Global As Boolean
Public IgnoreCase As Boolean
Public MultiLine As Boolean
Public Function Test(ByVal text As String) As Boolean
Dim m As Variant
m = WfcRegexMatches(Pattern, text, IgnoreCase, MultiLine, False)
Test = UBound(m) >= 0
End Function
Public Function Execute(ByVal text As String) As Object
Dim raw As Variant, i As Long, mc As New WfcMatchCollection
raw = WfcRegexMatches(Pattern, text, IgnoreCase, MultiLine, Global)
For i = 0 To UBound(raw)
mc.AddRaw raw(i)
Next i
Set Execute = mc
End Function
Public Function Replace(ByVal text As String, ByVal replacement As String))VB"
                                                  R"VB( As String
Replace = WfcRegexReplace(Pattern, text, replacement, IgnoreCase,)VB"
                                                  R"VB( MultiLine, Global)
End Function
)VB";

inline constexpr std::string_view kMatchCollectionSource =
    R"VB(Private items() As Variant
Private n As Long
Public Sub AddRaw(raw As Variant)
Dim m As New WfcMatch
m.Init raw
n = n + 1
If n = 1 Then
ReDim items(0 To 3)
ElseIf n > UBound(items) + 1 Then
ReDim Preserve items(0 To UBound(items) * 2 + 1)
End If
Set items(n - 1) = m
End Sub
Public Property Get Count() As Long
Count = n
End Property
Public Function Item(ByVal Index As Long) As Object
Attribute Item.VB_UserMemId = 0
If Index < 0 Or Index >= n Then Err.Raise 9, , "Subscript out of range"
Set Item = items(Index)
End Function
Public Function WfcItems() As Variant
Dim r() As Variant, i As Long
If n = 0 Then
WfcItems = Array()
Else
ReDim r(1 To n)
For i = 1 To n
Set r(i) = items(i - 1)
Next i
WfcItems = r
End If
End Function
)VB";

inline constexpr std::string_view kMatchSource = R"VB(Private mIndex As Long
Private mLength As Long
Private mValue As String
Private subs As Variant
Public Sub Init(raw As Variant)
Dim sm As New WfcSubMatches, i As Long
mIndex = raw(0)
mLength = raw(1)
mValue = raw(2)
For i = 3 To UBound(raw)
sm.AddText raw(i)
Next i
Set subs = sm
End Sub
Public Property Get Value() As String
Attribute Value.VB_UserMemId = 0
Value = mValue
End Property
Public Property Get FirstIndex() As Long
FirstIndex = mIndex
End Property
Public Property Get Length() As Long
Length = mLength
End Property
Public Property Get SubMatches() As Object
Set SubMatches = subs
End Property
)VB";

inline constexpr std::string_view kSubMatchesSource =
    R"VB(Private items() As Variant
Private n As Long
Public Sub AddText(ByVal s As String)
n = n + 1
If n = 1 Then
ReDim items(0 To 3)
ElseIf n > UBound(items) + 1 Then
ReDim Preserve items(0 To UBound(items) * 2 + 1)
End If
items(n - 1) = s
End Sub
Public Property Get Count() As Long
Count = n
End Property
Public Function Item(ByVal Index As Long) As String
Attribute Item.VB_UserMemId = 0
If Index < 0 Or Index >= n Then Err.Raise 9, , "Subscript out of range"
Item = items(Index)
End Function
Public Function WfcItems() As Variant
Dim r() As Variant, i As Long
If n = 0 Then
WfcItems = Array()
Else
ReDim r(1 To n)
For i = 1 To n
r(i) = items(i - 1)
Next i
WfcItems = r
End If
End Function
)VB";

}  // namespace wfc::detail

#endif  // WFC_INTERPRETER_BUILTIN_CLASS_SOURCES_HPP
