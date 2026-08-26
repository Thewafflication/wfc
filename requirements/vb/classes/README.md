# Visual Basic 6.0 Class Requirements

**Content type:** Project requirement index

**Status:** Proposed

**Reference baseline:** Installed `VB6.OLB`, Visual Basic type library 6.0

These requirements capture all 32 coclasses exposed by the VB run-time object
library. `VB6EXT.OLB` is the separate VBIDE extensibility model and is outside
this form-runtime requirement set.

| Requirement | Class | Interface | Properties | Methods | Events |
| --- | --- | --- | ---: | ---: | ---: |
| `REQ-0037` | [PictureBox](req-0037-picture-box-api-contract.md) | `_PictureBox` | 67 | 22 | 26 |
| `REQ-0038` | [Label](req-0038-label-api-contract.md) | `_Label` | 47 | 13 | 18 |
| `REQ-0039` | [TextBox](req-0039-text-box-api-contract.md) | `_TextBox` | 57 | 14 | 24 |
| `REQ-0040` | [Frame](req-0040-frame-api-contract.md) | `_Frame` | 36 | 9 | 13 |
| `REQ-0041` | [CommandButton](req-0041-command-button-api-contract.md) | `_CommandButton` | 44 | 10 | 17 |
| `REQ-0042` | [CheckBox](req-0042-check-box-api-contract.md) | `_CheckBox` | 49 | 10 | 18 |
| `REQ-0043` | [OptionButton](req-0043-option-button-api-contract.md) | `_OptionButton` | 44 | 10 | 19 |
| `REQ-0044` | [ComboBox](req-0044-combo-box-api-contract.md) | `_ComboBox` | 55 | 10 | 19 |
| `REQ-0045` | [ListBox](req-0045-list-box-api-contract.md) | `_ListBox` | 55 | 10 | 21 |
| `REQ-0046` | [HScrollBar](req-0046-hscroll-bar-api-contract.md) | `_HScrollBar` | 28 | 9 | 10 |
| `REQ-0047` | [VScrollBar](req-0047-vscroll-bar-api-contract.md) | `_VScrollBar` | 28 | 9 | 10 |
| `REQ-0048` | [Timer](req-0048-timer-api-contract.md) | `_Timer` | 7 | 3 | 1 |
| `REQ-0049` | [Printer](req-0049-printer-api-contract.md) | `_Printer` | 42 | 12 | 0 |
| `REQ-0050` | [Form](req-0050-form-api-contract.md) | `_Form` | 65 | 22 | 31 |
| `REQ-0051` | [Screen](req-0051-screen-api-contract.md) | `_Screen` | 10 | 0 | 0 |
| `REQ-0052` | [Clipboard](req-0052-clipboard-api-contract.md) | `_Clipboard` | 0 | 6 | 0 |
| `REQ-0053` | [DriveListBox](req-0053-drive-list-box-api-contract.md) | `_DriveListBox` | 39 | 10 | 16 |
| `REQ-0054` | [DirListBox](req-0054-dir-list-box-api-contract.md) | `_DirListBox` | 40 | 10 | 20 |
| `REQ-0055` | [FileListBox](req-0055-file-list-box-api-contract.md) | `_FileListBox` | 49 | 10 | 22 |
| `REQ-0056` | [Menu](req-0056-menu-api-contract.md) | `_Menu` | 11 | 0 | 1 |
| `REQ-0057` | [MDIForm](req-0057-mdiform-api-contract.md) | `_MDIForm` | 34 | 10 | 25 |
| `REQ-0058` | [App](req-0058-app-api-contract.md) | `_App` | 30 | 2 | 0 |
| `REQ-0059` | [Shape](req-0059-shape-api-contract.md) | `_Shape` | 20 | 7 | 0 |
| `REQ-0060` | [Line](req-0060-line-api-contract.md) | `_Line` | 15 | 6 | 0 |
| `REQ-0061` | [Image](req-0061-image-api-contract.md) | `_Image` | 29 | 9 | 13 |
| `REQ-0062` | [Data](req-0062-data-api-contract.md) | `_Data` | 45 | 8 | 15 |
| `REQ-0063` | [OLE](req-0063-ole-api-contract.md) | `_OLE` | 59 | 20 | 16 |
| `REQ-0064` | [UserControl](req-0064-user-control-api-contract.md) | `_UserControl` | 63 | 22 | 35 |
| `REQ-0065` | [PropertyPage](req-0065-property-page-api-contract.md) | `_PropertyPage` | 49 | 16 | 24 |
| `REQ-0066` | [UserDocument](req-0066-user-document-api-contract.md) | `_UserDocument` | 57 | 21 | 32 |
| `REQ-0067` | [Global](req-0067-global-api-contract.md) | `VBGlobal` | 7 | 7 | 0 |
| `REQ-0068` | [VBControlExtender](req-0068-vbcontrol-extender-api-contract.md) | `_VBControlExtender` | 0 | 5 | 6 |

## Form Runtime Boundary

The primary form classes are `Form` and `MDIForm`. `UserControl`,
`PropertyPage`, and `UserDocument` define additional hosted visual surfaces;
the intrinsic controls provide the standard children used by VB6 forms.
