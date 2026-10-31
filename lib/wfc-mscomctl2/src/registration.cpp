// Registration of the Windows Common Controls-2 library with Windows: the entry
// points regsvr32 calls. The work is in wfc/ocx/registration.hpp. Internal to
// the WFC libraries.
//
// The class table lists the controls of the library: WFC's own ProgIDs and
// class ids, and the Microsoft ones used when WFC_REGISTER_MICROSOFT_IDS=1.
// The library does not create the controls itself yet (DllGetClassObject
// reports that the class is not available); the registration records which
// library and which version serves them.

#include <wfc/ocx/registration.hpp>
#include "registration_info.hpp"

#include <iterator>

namespace {

const wfc::registration::Library kLibrary{L"mscomctl2", L"" WFC_LIB_VERSION,
                                          L"" WFC_LIB_TARGET};

const wfc::registration::ComClass kClasses[] = {
    {L"Animation", L"{7F0C74C8-0C87-5AE2-A134-DAD91A6F99F7}",
     L"WFC.MSComCtl2.Animation", L"{B09DE715-87C1-11D1-8BE3-0000F8754DA1}",
     L"MSComCtl2.Animation.2"},
    {L"UpDown", L"{14C1B9B8-1541-5BCE-A4B9-7137848B6708}",
     L"WFC.MSComCtl2.UpDown", L"{603C7E80-87C2-11D1-8BE3-0000F8754DA1}",
     L"MSComCtl2.UpDown.2"},
    {L"MonthView", L"{AB9666ED-78F0-5E96-AFB3-A8667C357E80}",
     L"WFC.MSComCtl2.MonthView", L"{232E456A-87C3-11D1-8BE3-0000F8754DA1}",
     L"MSComCtl2.MonthView.2"},
    {L"DTPicker", L"{D79C5747-8AEF-5741-9F7E-5445F5777595}",
     L"WFC.MSComCtl2.DTPicker", L"{20DD1B9E-87C4-11D1-8BE3-0000F8754DA1}",
     L"MSComCtl2.DTPicker.2"},
    {L"FlatScrollBar", L"{E5EE73B3-49BC-5CF6-82F1-39EEC8F37260}",
     L"WFC.MSComCtl2.FlatScrollBar", L"{FE38753A-44A3-11D1-B5B7-0000C09000C4}",
     L"MSComCtl2.FlatScrollBar.2"},
};

HMODULE this_module() {
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&this_module), &module);
    return module;
}

}  // namespace

extern "C" {

HRESULT __stdcall DllRegisterServer() {
    return wfc::registration::register_library(this_module(), kLibrary,
                                               kClasses, std::size(kClasses));
}

HRESULT __stdcall DllUnregisterServer() {
    return wfc::registration::unregister_library(
        kLibrary, kClasses, std::size(kClasses), this_module());
}

HRESULT __stdcall DllCanUnloadNow() {
    return S_OK;
}

HRESULT __stdcall DllGetClassObject(REFCLSID, REFIID, void** object) {
    if (object != nullptr) {
        *object = nullptr;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

}  // extern "C"
