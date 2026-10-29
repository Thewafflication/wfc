// The member table of the Common Dialog (comdlg32.ocx, library MSComDlg).
// Internal to the WFC runtime; not part of the public API.
//
// The table is plain data. It is linked statically into the program or, when
// the library is built shared (WFC_COMDLG32_SHARED), loaded from
// wfccomdlg32.dll; only the accessor crosses the library boundary.

#ifndef WFC_COMDLG32_COMDLG32_HPP
#define WFC_COMDLG32_COMDLG32_HPP

#include <span>

#include <wfc/ocx/control_class.hpp>

#if defined(_WIN32) && defined(WFC_COMDLG32_SHARED)
#if defined(WFC_COMDLG32_BUILD_DLL)
#define WFC_COMDLG32_API __declspec(dllexport)
#else
#define WFC_COMDLG32_API __declspec(dllimport)
#endif
#else
#define WFC_COMDLG32_API
#endif

namespace wfc::ui {

// The classes of the library: CommonDialog.
[[nodiscard]] WFC_COMDLG32_API std::span<const ControlClass> comdlg32_classes();

}  // namespace wfc::ui

#endif  // WFC_COMDLG32_COMDLG32_HPP
