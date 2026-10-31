/* Black-box check of wfcmscomctl2.dll: exports, version resource and the
 * registration round trip (see dll_check.h). Run by CTest with the DLL,
 * the version and the target as arguments. */
#include "dll_check.h"

static const struct wfc_check_class kClasses[] = {
    {L"WFC.MSComCtl2.Animation",
     L"{7F0C74C8-0C87-5AE2-A134-DAD91A6F99F7}",
     L"MSComCtl2.Animation.2",
     L"{B09DE715-87C1-11D1-8BE3-0000F8754DA1}"},
    {L"WFC.MSComCtl2.UpDown",
     L"{14C1B9B8-1541-5BCE-A4B9-7137848B6708}",
     L"MSComCtl2.UpDown.2",
     L"{603C7E80-87C2-11D1-8BE3-0000F8754DA1}"},
    {L"WFC.MSComCtl2.MonthView",
     L"{AB9666ED-78F0-5E96-AFB3-A8667C357E80}",
     L"MSComCtl2.MonthView.2",
     L"{232E456A-87C3-11D1-8BE3-0000F8754DA1}"},
    {L"WFC.MSComCtl2.DTPicker",
     L"{D79C5747-8AEF-5741-9F7E-5445F5777595}",
     L"MSComCtl2.DTPicker.2",
     L"{20DD1B9E-87C4-11D1-8BE3-0000F8754DA1}"},
    {L"WFC.MSComCtl2.FlatScrollBar",
     L"{E5EE73B3-49BC-5CF6-82F1-39EEC8F37260}",
     L"MSComCtl2.FlatScrollBar.2",
     L"{FE38753A-44A3-11D1-B5B7-0000C09000C4}"},
};

int main(int argc, char** argv) {
    return wfc_check_dll(argc, argv, L"mscomctl2", kClasses,
                         sizeof(kClasses) / sizeof(kClasses[0]));
}
