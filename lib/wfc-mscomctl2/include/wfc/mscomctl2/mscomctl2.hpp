// The member table of the Windows Common Controls-2 (mscomct2.ocx, library MSComCtl2).
// Internal to the WFC runtime; not part of the public API.
//
// The table is plain data. It is linked statically into the program or, when
// the library is built shared (WFC_MSCOMCTL2_SHARED), loaded from
// wfcmscomctl2.dll; only the accessor crosses the library boundary.

#ifndef WFC_MSCOMCTL2_MSCOMCTL2_HPP
#define WFC_MSCOMCTL2_MSCOMCTL2_HPP

#include <span>

#include <wfc/ocx/control_class.hpp>

#if defined(_WIN32) && defined(WFC_MSCOMCTL2_SHARED)
#if defined(WFC_MSCOMCTL2_BUILD_DLL)
#define WFC_MSCOMCTL2_API __declspec(dllexport)
#else
#define WFC_MSCOMCTL2_API __declspec(dllimport)
#endif
#else
#define WFC_MSCOMCTL2_API
#endif

namespace wfc::ui {

// The classes of the library: Animation, UpDown, MonthView, DTPicker, FlatScrollBar.
[[nodiscard]] WFC_MSCOMCTL2_API std::span<const ControlClass> mscomctl2_classes();

}  // namespace wfc::ui

#endif  // WFC_MSCOMCTL2_MSCOMCTL2_HPP
