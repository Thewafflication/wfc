// The member table of the Windows Common Controls (mscomctl.ocx, library MSComctlLib).
// Internal to the WFC runtime; not part of the public API.
//
// The table is plain data. It is linked statically into the program or, when
// the library is built shared (WFC_MSCOMCTL_SHARED), loaded from
// wfcmscomctl.dll; only the accessor crosses the library boundary.

#ifndef WFC_MSCOMCTL_MSCOMCTL_HPP
#define WFC_MSCOMCTL_MSCOMCTL_HPP

#include <span>

#include <wfc/ocx/control_class.hpp>

#if defined(_WIN32) && defined(WFC_MSCOMCTL_SHARED)
#if defined(WFC_MSCOMCTL_BUILD_DLL)
#define WFC_MSCOMCTL_API __declspec(dllexport)
#else
#define WFC_MSCOMCTL_API __declspec(dllimport)
#endif
#else
#define WFC_MSCOMCTL_API
#endif

namespace wfc::ui {

// The classes of the library: TabStrip, Toolbar, StatusBar, ProgressBar, TreeView, ListView, ImageList, Slider, ImageCombo.
[[nodiscard]] WFC_MSCOMCTL_API std::span<const ControlClass> mscomctl_classes();

}  // namespace wfc::ui

#endif  // WFC_MSCOMCTL_MSCOMCTL_HPP
