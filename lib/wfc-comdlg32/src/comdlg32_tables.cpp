// Member table of the native replacement for the Common Dialog (`comdlg32.ocx`,
// library MSComDlg). Written by hand from the documented members of the control;
// there is no type-library evidence for it.

#include <wfc/comdlg32/comdlg32.hpp>

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

const ControlClass kClasses[] = {
    {"CommonDialog", kDialogMembers, sizeof(kDialogMembers) / sizeof(MemberRow),
     nullptr, 0},
};

}  // namespace

std::span<const ControlClass> comdlg32_classes() {
    return kClasses;
}

}  // namespace wfc::ui
