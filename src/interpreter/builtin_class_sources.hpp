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
Set ActiveForm = WfcActive(0)
End Property
Public Property Get ActiveControl() As Object
Set ActiveControl = WfcActive(1)
End Property
)VB";

inline constexpr std::string_view kPrinterSource =
    R"VB(Private mDoc As String
Private mPage As Integer
Private mCopies As Integer
Private mOrientation As Integer
Private mFontName As String
Private mFontSize As Single
Private mFontBold As Boolean
Private mFontItalic As Boolean
Private mFontUnderline As Boolean
Private mFontStrikethru As Boolean
Private mScaleMode As Integer
Private mX As Single
Private mY As Single
Private mInit As Boolean
Private Sub Setup()
If mInit Then Exit Sub
mInit = True
mPage = 1
mCopies = 1
mOrientation = 1
mFontName = "Arial"
mFontSize = 8.25
mScaleMode = 1
End Sub
Public Property Get DeviceName() As String
DeviceName = "WFC Virtual Printer"
End Property
Public Property Get DriverName() As String
DriverName = "WFCPRN"
End Property
Public Property Get Port() As String
Port = "FILE:"
End Property
Public Property Get hDC() As Long
hDC = 0
End Property
Public Property Get ColorMode() As Integer
ColorMode = 1
End Property
Public Property Get Duplex() As Integer
Duplex = 1
End Property
Public Property Get PrintQuality() As Integer
PrintQuality = -4
End Property
Public Property Get PaperSize() As Integer
PaperSize = 1
End Property
Public Property Get PaperBin() As Integer
PaperBin = 1
End Property
Public Property Get Zoom() As Integer
Zoom = 100
End Property
Public Property Get TrackDefault() As Boolean
TrackDefault = True
End Property
Public Property Get Copies() As Integer
Setup
Copies = mCopies
End Property
Public Property Let Copies(ByVal v As Integer)
Setup
mCopies = v
End Property
Public Property Get Orientation() As Integer
Setup
Orientation = mOrientation
End Property
Public Property Let Orientation(ByVal v As Integer)
Setup
mOrientation = v
End Property
Public Property Get Page() As Integer
Setup
Page = mPage
End Property
Public Property Get Width() As Single
Setup
If mOrientation = 2 Then Width = 15840 Else Width = 12240
End Property
Public Property Get Height() As Single
Setup
If mOrientation = 2 Then Height = 12240 Else Height = 15840
End Property
Public Property Get ScaleWidth() As Single
ScaleWidth = Width
End Property
Public Property Get ScaleHeight() As Single
ScaleHeight = Height
End Property
Public Property Get ScaleMode() As Integer
Setup
ScaleMode = mScaleMode
End Property
Public Property Let ScaleMode(ByVal v As Integer)
Setup
mScaleMode = v
End Property
Public Property Get TwipsPerPixelX() As Single
TwipsPerPixelX = 2.4
End Property
Public Property Get TwipsPerPixelY() As Single
TwipsPerPixelY = 2.4
End Property
Public Property Get FontName() As String
Setup
FontName = mFontName
End Property
Public Property Let FontName(ByVal v As String)
Setup
mFontName = v
End Property
Public Property Get FontSize() As Single
Setup
FontSize = mFontSize
End Property
Public Property Let FontSize(ByVal v As Single)
Setup
mFontSize = v
End Property
Public Property Get FontBold() As Boolean
FontBold = mFontBold
End Property
Public Property Let FontBold(ByVal v As Boolean)
mFontBold = v
End Property
Public Property Get FontItalic() As Boolean
FontItalic = mFontItalic
End Property
Public Property Let FontItalic(ByVal v As Boolean)
mFontItalic = v
End Property
Public Property Get FontUnderline() As Boolean
FontUnderline = mFontUnderline
End Property
Public Property Let FontUnderline(ByVal v As Boolean)
mFontUnderline = v
End Property
Public Property Get FontStrikethru() As Boolean
FontStrikethru = mFontStrikethru
End Property
Public Property Let FontStrikethru(ByVal v As Boolean)
mFontStrikethru = v
End Property
Public Property Get CurrentX() As Single
CurrentX = mX
End Property
Public Property Let CurrentX(ByVal v As Single)
mX = v
End Property
Public Property Get CurrentY() As Single
CurrentY = mY
End Property
Public Property Let CurrentY(ByVal v As Single)
mY = v
End Property
Public Function TextWidth(ByVal Str As String) As Single
Setup
TextWidth = Len(Str) * mFontSize * 10
End Function
Public Function TextHeight(ByVal Str As String) As Single
Setup
TextHeight = mFontSize * 24
End Function
Public Sub NewPage()
Setup
mPage = mPage + 1
mDoc = mDoc & Chr$(12) & vbLf
End Sub
Public Sub KillDoc()
mDoc = ""
mPage = 1
End Sub
Public Sub EndDoc()
Dim lines() As String
Dim i As Long
Setup
lines = Split(mDoc, vbLf)
For i = 0 To UBound(lines)
If Left$(lines(i), 1) = Chr$(12) Then
Debug.Print "[Printer] -- new page --"
ElseIf Not (i = UBound(lines) And lines(i) = "") Then
Debug.Print "[Printer] " & lines(i)
End If
Next i
mDoc = ""
mPage = 1
End Sub
)VB";

inline constexpr std::string_view kPrintersSource =
    R"VB(Public Property Get Count() As Long
Count = 1
End Property
Public Property Get Item(ByVal Index As Long) As Object
Attribute Item.VB_UserMemId = 0
If Index <> 0 Then Err.Raise 9, , "Subscript out of range"
Set Item = Printer
End Property
Public Function WfcItems() As Variant
WfcItems = Array(Printer)
End Function
)VB";

inline constexpr std::string_view kPropertyBagSource =
    R"VB(Private mNames() As String
Private mTypes() As Long
Private mValues() As String
Private mCount As Long
Private Function Find(ByVal Name As String) As Long
Dim i As Long
For i = 1 To mCount
If LCase$(mNames(i)) = LCase$(Name) Then
Find = i
Exit Function
End If
Next i
Find = 0
End Function
Private Sub Remove(ByVal Index As Long)
Dim i As Long
For i = Index To mCount - 1
mNames(i) = mNames(i + 1)
mTypes(i) = mTypes(i + 1)
mValues(i) = mValues(i + 1)
Next i
mCount = mCount - 1
End Sub
Public Sub WriteProperty(Name As String, Value, Optional DefaultValue)
Dim i As Long
If IsObject(Value) Then Err.Raise 5, , "Invalid procedure call or argument"
i = Find(Name)
If Not IsMissing(DefaultValue) Then
If VarType(Value) = VarType(DefaultValue) Then
If Value = DefaultValue Then
If i > 0 Then Remove i
Exit Sub
End If
End If
End If
If i = 0 Then
mCount = mCount + 1
ReDim Preserve mNames(1 To mCount)
ReDim Preserve mTypes(1 To mCount)
ReDim Preserve mValues(1 To mCount)
i = mCount
End If
mNames(i) = Name
mTypes(i) = VarType(Value)
If VarType(Value) = 11 Then
mValues(i) = CStr(CInt(Value))
Else
mValues(i) = CStr(Value)
End If
End Sub
Public Function ReadProperty(Name As String, Optional DefaultValue) As Variant
Dim i As Long
i = Find(Name)
If i = 0 Then
If IsMissing(DefaultValue) Then
ReadProperty = Empty
Else
ReadProperty = DefaultValue
End If
Exit Function
End If
Select Case mTypes(i)
Case 2: ReadProperty = CInt(mValues(i))
Case 3: ReadProperty = CLng(mValues(i))
Case 4: ReadProperty = CSng(mValues(i))
Case 5: ReadProperty = CDbl(mValues(i))
Case 6: ReadProperty = CCur(mValues(i))
Case 7: ReadProperty = CDate(mValues(i))
Case 11: ReadProperty = (mValues(i) <> "0")
Case 17: ReadProperty = CByte(mValues(i))
Case Else: ReadProperty = mValues(i)
End Select
End Function
Private Function Escape(ByVal s As String) As String
s = Replace(s, "\", "\\")
s = Replace(s, vbCr, "\r")
s = Replace(s, vbLf, "\n")
Escape = Replace(s, "=", "\e")
End Function
Private Function Unescape(ByVal s As String) As String
Dim i As Long
Dim c As String
Dim r As String
i = 1
Do While i <= Len(s)
c = Mid$(s, i, 1)
If c = "\" And i < Len(s) Then
i = i + 1
Select Case Mid$(s, i, 1)
Case "r": r = r & vbCr
Case "n": r = r & vbLf
Case "e": r = r & "="
Case Else: r = r & Mid$(s, i, 1)
End Select
Else
r = r & c
End If
i = i + 1
Loop
Unescape = r
End Function
Public Property Get Contents() As Variant
Dim i As Long
Dim blob As String
For i = 1 To mCount
blob = blob & Escape(mNames(i)) & "=" & mTypes(i) & "="
blob = blob & Escape(mValues(i)) & vbLf
Next i
Contents = StrConv(blob, vbFromUnicode)
End Property
Public Property Let Contents(ByVal v As Variant)
Dim blob As String
Dim lines() As String
Dim parts() As String
Dim i As Long
blob = StrConv(v, vbUnicode)
mCount = 0
lines = Split(blob, vbLf)
For i = 0 To UBound(lines)
If Len(lines(i)) > 0 Then
parts = Split(lines(i), "=")
If UBound(parts) <> 2 Then Err.Raise 5, , "Invalid procedure call or argument"
mCount = mCount + 1
ReDim Preserve mNames(1 To mCount)
ReDim Preserve mTypes(1 To mCount)
ReDim Preserve mValues(1 To mCount)
mNames(mCount) = Unescape(parts(0))
mTypes(mCount) = CLng(parts(1))
mValues(mCount) = Unescape(parts(2))
End If
Next i
End Property
)VB";

inline constexpr std::string_view kAmbientSource =
    R"VB(Public Property Get BackColor() As Long
BackColor = -2147483633
End Property
Public Property Get ForeColor() As Long
ForeColor = -2147483630
End Property
Public Property Get DisplayName() As String
DisplayName = ""
End Property
Public Property Get Font() As Object
Dim f As New StdFont
f.Name = "MS Sans Serif"
f.Size = 8.25
Set Font = f
End Property
Public Property Get LocaleID() As Long
LocaleID = 1033
End Property
Public Property Get UserMode() As Boolean
UserMode = True
End Property
Public Property Get UIDead() As Boolean
UIDead = False
End Property
Public Property Get SupportsMnemonics() As Boolean
SupportsMnemonics = True
End Property
Public Property Get ShowGrabHandles() As Boolean
ShowGrabHandles = False
End Property
Public Property Get ShowHatching() As Boolean
ShowHatching = False
End Property
Public Property Get MessageReflect() As Boolean
MessageReflect = False
End Property
Public Property Get TextAlign() As Integer
TextAlign = 0
End Property
Public Property Get ScaleUnits() As String
ScaleUnits = "Twip"
End Property
Public Property Get DisplayAsDefault() As Boolean
DisplayAsDefault = False
End Property
Public Property Get RightToLeft() As Boolean
RightToLeft = False
End Property
)VB";

inline constexpr std::string_view kLicensesSource =
    R"VB(Private mIds() As String
Private mKeys() As String
Private mCount As Long
Private Function Find(ByVal Id As String) As Long
Dim i As Long
For i = 1 To mCount
If LCase$(mIds(i)) = LCase$(Id) Then
Find = i
Exit Function
End If
Next i
End Function
Public Property Get Count() As Long
Count = mCount
End Property
Public Sub Add(ByVal ProgId As String, Optional ByVal LicenseKey As Variant)
Dim i As Long
If Find(ProgId) <> 0 Then
Err.Raise 457, , "This key is already associated with an element"
End If
mCount = mCount + 1
ReDim Preserve mIds(1 To mCount)
ReDim Preserve mKeys(1 To mCount)
mIds(mCount) = ProgId
mKeys(mCount) = ""
If Not IsMissing(LicenseKey) Then mKeys(mCount) = CStr(LicenseKey)
End Sub
Public Property Get Item(ByVal Index As Variant) As String
Attribute Item.VB_UserMemId = 0
Dim i As Long
If VarType(Index) = 8 Then
i = Find(CStr(Index))
Else
i = CLng(Index)
End If
If i < 1 Or i > mCount Then Err.Raise 5, , "Invalid procedure call or argument"
Item = mKeys(i)
End Property
Public Sub Remove(ByVal Index As Variant)
Dim i As Long
Dim j As Long
If VarType(Index) = 8 Then
i = Find(CStr(Index))
Else
i = CLng(Index)
End If
If i < 1 Or i > mCount Then Err.Raise 5, , "Invalid procedure call or argument"
For j = i To mCount - 1
mIds(j) = mIds(j + 1)
mKeys(j) = mKeys(j + 1)
Next j
mCount = mCount - 1
End Sub
)VB";

inline constexpr std::string_view kDataObjectFilesSource =
    R"VB(Private mNames() As String
Private mCount As Long
Public Property Get Count() As Long
Count = mCount
End Property
Public Sub Add(ByVal bstrFilename As String)
mCount = mCount + 1
ReDim Preserve mNames(1 To mCount)
mNames(mCount) = bstrFilename
End Sub
Public Sub Clear()
mCount = 0
End Sub
Public Property Get Item(ByVal lIndex As Long) As String
Attribute Item.VB_UserMemId = 0
If lIndex < 1 Or lIndex > mCount Then Err.Raise 9, , "Subscript out of range"
Item = mNames(lIndex)
End Property
Public Sub Remove(ByVal vIndex As Long)
Dim i As Long
If vIndex < 1 Or vIndex > mCount Then Err.Raise 9, , "Subscript out of range"
For i = vIndex To mCount - 1
mNames(i) = mNames(i + 1)
Next i
mCount = mCount - 1
End Sub
)VB";

inline constexpr std::string_view kDataObjectSource =
    R"VB(Private mFormats() As Long
Private mData() As Variant
Private mCount As Long
Private mFiles As WfcDataObjectFiles
Private Function Find(ByVal Format As Long) As Long
Dim i As Long
For i = 1 To mCount
If mFormats(i) = Format Then
Find = i
Exit Function
End If
Next i
End Function
Public Property Get Files() As Object
If mFiles Is Nothing Then Set mFiles = New WfcDataObjectFiles
Set Files = mFiles
End Property
Public Sub Clear()
mCount = 0
If Not mFiles Is Nothing Then mFiles.Clear
End Sub
Public Sub SetData(Optional Value, Optional Format)
Dim f As Long
Dim i As Long
If IsMissing(Format) Then f = 1 Else f = CLng(Format)
If f = 15 Then Err.Raise 5, , "Invalid procedure call or argument"
i = Find(f)
If i = 0 Then
mCount = mCount + 1
ReDim Preserve mFormats(1 To mCount)
ReDim Preserve mData(1 To mCount)
i = mCount
End If
mFormats(i) = f
If IsObject(Value) Then Set mData(i) = Value Else mData(i) = Value
End Sub
Public Function GetData(ByVal Format As Integer) As Variant
Dim i As Long
i = Find(Format)
If i = 0 Then Err.Raise 461, , "Specified format doesn't match format of data"
If IsObject(mData(i)) Then Set GetData = mData(i) Else GetData = mData(i)
End Function
Public Function GetFormat(ByVal Format As Integer) As Boolean
If Format = 15 Then
GetFormat = (Not mFiles Is Nothing) And (Files.Count > 0)
Else
GetFormat = Find(Format) <> 0
End If
End Function
)VB";

inline constexpr std::string_view kFormsSource =
    R"VB(Public Property Get Count() As Long
Count = 0
End Property
Public Property Get Item(ByVal Index As Long) As Object
Attribute Item.VB_UserMemId = 0
Err.Raise 9, , "Subscript out of range"
End Property
Public Function WfcItems() As Variant
WfcItems = Array()
End Function
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
