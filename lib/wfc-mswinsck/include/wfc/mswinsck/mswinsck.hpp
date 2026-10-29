// The member table of the Winsock (mswinsck.ocx, library MSWinsockLib).
// Internal to the WFC runtime; not part of the public API.
//
// The table is plain data. It is linked statically into the program or, when
// the library is built shared (WFC_MSWINSCK_SHARED), loaded from
// wfcmswinsck.dll; only the accessor crosses the library boundary.

#ifndef WFC_MSWINSCK_MSWINSCK_HPP
#define WFC_MSWINSCK_MSWINSCK_HPP

#include <span>

#include <wfc/ocx/control_class.hpp>

#if defined(_WIN32) && defined(WFC_MSWINSCK_SHARED)
#if defined(WFC_MSWINSCK_BUILD_DLL)
#define WFC_MSWINSCK_API __declspec(dllexport)
#else
#define WFC_MSWINSCK_API __declspec(dllimport)
#endif
#else
#define WFC_MSWINSCK_API
#endif

namespace wfc::ui {

// The classes of the library: Winsock.
[[nodiscard]] WFC_MSWINSCK_API std::span<const ControlClass> mswinsck_classes();

}  // namespace wfc::ui

#endif  // WFC_MSWINSCK_MSWINSCK_HPP
