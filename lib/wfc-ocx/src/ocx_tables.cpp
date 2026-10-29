// Member tables of the native replacements for two common ActiveX controls:
// the Common Dialog (`comdlg32.ocx`, library MSComDlg) and Winsock
// (`mswinsck.ocx`, library MSWinsockLib). Written by hand from the documented
// members of the two controls; there is no type-library evidence for them.

#include <wfc/ocx/control_tables.hpp>

namespace wfc::ui {
namespace {

const MemberRow kDialogMembers[] = {
    {"Action", 'g', "Integer", 1, ""},
    {"Action", 'l', "Integer", 1, ""},
    {"CancelError", 'g', "Boolean", 2, ""},
    {"CancelError", 'l', "Boolean", 2, ""},
    {"Color", 'g', "Long", 3, ""},
    {"Color", 'l', "Long", 3, ""},
    {"Copies", 'g', "Integer", 4, ""},
    {"Copies", 'l', "Integer", 4, ""},
    {"DefaultExt", 'g', "String", 5, ""},
    {"DefaultExt", 'l', "String", 5, ""},
    {"DialogTitle", 'g', "String", 6, ""},
    {"DialogTitle", 'l', "String", 6, ""},
    {"FileName", 'g', "String", 7, ""},
    {"FileName", 'l', "String", 7, ""},
    {"FileTitle", 'g', "String", 8, ""},
    {"Filter", 'g', "String", 9, ""},
    {"Filter", 'l', "String", 9, ""},
    {"FilterIndex", 'g', "Integer", 10, ""},
    {"FilterIndex", 'l', "Integer", 10, ""},
    {"Flags", 'g', "Long", 11, ""},
    {"Flags", 'l', "Long", 11, ""},
    {"FontBold", 'g', "Boolean", 12, ""},
    {"FontBold", 'l', "Boolean", 12, ""},
    {"FontItalic", 'g', "Boolean", 13, ""},
    {"FontItalic", 'l', "Boolean", 13, ""},
    {"FontName", 'g', "String", 14, ""},
    {"FontName", 'l', "String", 14, ""},
    {"FontSize", 'g', "Single", 15, ""},
    {"FontSize", 'l', "Single", 15, ""},
    {"FontStrikethru", 'g', "Boolean", 16, ""},
    {"FontStrikethru", 'l', "Boolean", 16, ""},
    {"FontUnderline", 'g', "Boolean", 17, ""},
    {"FontUnderline", 'l', "Boolean", 17, ""},
    {"FromPage", 'g', "Integer", 18, ""},
    {"FromPage", 'l', "Integer", 18, ""},
    {"HelpCommand", 'g', "Integer", 19, ""},
    {"HelpCommand", 'l', "Integer", 19, ""},
    {"HelpContext", 'g', "Long", 20, ""},
    {"HelpContext", 'l', "Long", 20, ""},
    {"HelpFile", 'g', "String", 21, ""},
    {"HelpFile", 'l', "String", 21, ""},
    {"HelpKey", 'g', "String", 22, ""},
    {"HelpKey", 'l', "String", 22, ""},
    {"InitDir", 'g', "String", 23, ""},
    {"InitDir", 'l', "String", 23, ""},
    {"Max", 'g', "Integer", 24, ""},
    {"Max", 'l', "Integer", 24, ""},
    {"MaxFileSize", 'g', "Integer", 25, ""},
    {"MaxFileSize", 'l', "Integer", 25, ""},
    {"Min", 'g', "Integer", 26, ""},
    {"Min", 'l', "Integer", 26, ""},
    {"Orientation", 'g', "Integer", 27, ""},
    {"Orientation", 'l', "Integer", 27, ""},
    {"PrinterDefault", 'g', "Boolean", 28, ""},
    {"PrinterDefault", 'l', "Boolean", 28, ""},
    {"ToPage", 'g', "Integer", 29, ""},
    {"ToPage", 'l', "Integer", 29, ""},
    {"hDC", 'g', "Long", 30, ""},
    {"ShowColor", 'm', "Void", 31, ""},
    {"ShowFont", 'm', "Void", 32, ""},
    {"ShowHelp", 'm', "Void", 33, ""},
    {"ShowOpen", 'm', "Void", 34, ""},
    {"ShowPrinter", 'm', "Void", 35, ""},
    {"ShowSave", 'm', "Void", 36, ""},
    {"AboutBox", 'm', "Void", -552, ""},
};

const MemberRow kWinsockMembers[] = {
    {"BytesReceived", 'g', "Long", 1, ""},
    {"LocalHostName", 'g', "String", 2, ""},
    {"LocalIP", 'g', "String", 3, ""},
    {"LocalPort", 'g', "Long", 4, ""},
    {"LocalPort", 'l', "Long", 4, ""},
    {"Protocol", 'g', "Long", 5, ""},
    {"Protocol", 'l', "Long", 5, ""},
    {"RemoteHost", 'g', "String", 6, ""},
    {"RemoteHost", 'l', "String", 6, ""},
    {"RemoteHostIP", 'g', "String", 7, ""},
    {"RemotePort", 'g', "Long", 8, ""},
    {"RemotePort", 'l', "Long", 8, ""},
    {"SocketHandle", 'g', "Long", 9, ""},
    {"State", 'g', "Long", 10, ""},
    {"Accept", 'm', "Void", 11, "requestID:Long"},
    {"Bind", 'm', "Void", 12, "LocalPort:Variant:o;LocalIP:Variant:o"},
    {"Close", 'm', "Void", 13, ""},
    {"Connect", 'm', "Void", 14, "RemoteHost:Variant:o;RemotePort:Variant:o"},
    {"GetData", 'm', "Void", 15,
     "data:Variant;type:Variant:o;maxLen:Variant:o"},
    {"Listen", 'm', "Void", 16, ""},
    {"PeekData", 'm', "Void", 17,
     "data:Variant;type:Variant:o;maxLen:Variant:o"},
    {"SendData", 'm', "Void", 18, "data:Variant"},
};

const MemberRow kWinsockEvents[] = {
    {"Close", 'm', "Void", 1, ""},
    {"Connect", 'm', "Void", 2, ""},
    {"ConnectionRequest", 'm', "Void", 3, "requestID:Long"},
    {"DataArrival", 'm', "Void", 4, "bytesTotal:Long"},
    {"Error", 'm', "Void", 5,
     "Number:Integer;Description:String;Scode:Long;Source:String;HelpFile:"
     "String;HelpContext:Long;CancelDisplay:Boolean"},
    {"SendComplete", 'm', "Void", 6, ""},
    {"SendProgress", 'm', "Void", 7, "bytesSent:Long;bytesRemaining:Long"},
};

const ControlClass kClasses[] = {
    {"CommonDialog", kDialogMembers, sizeof(kDialogMembers) / sizeof(MemberRow),
     nullptr, 0},
    {"Winsock", kWinsockMembers, sizeof(kWinsockMembers) / sizeof(MemberRow),
     kWinsockEvents, sizeof(kWinsockEvents) / sizeof(MemberRow)},
};

}  // namespace

std::span<const ControlClass> ocx_classes() {
    return kClasses;
}

}  // namespace wfc::ui
