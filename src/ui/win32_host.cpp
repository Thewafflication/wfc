// The Win32 UiHost: real windows, standard controls, menus and a message loop.
// Internal to the WFC runtime; not part of the public API.

#ifdef _WIN32

#include "win32_host.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <commctrl.h>
#include <commdlg.h>
#include <objbase.h>
#include <ocidl.h>
#include <olectl.h>
#include <wincodec.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace wfc::ui {
namespace {

constexpr wchar_t kFormClass[] = L"WfcForm";
constexpr wchar_t kPictureClass[] = L"WfcPicture";
constexpr wchar_t kIdProperty[] = L"WfcId";

[[nodiscard]] std::wstring widen(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int length = MultiByteToWideChar(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        wide.data(), length);
    return wide;
}

[[nodiscard]] std::string narrow(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int length = WideCharToMultiByte(CP_UTF8, 0, text.data(),
                                           static_cast<int>(text.size()),
                                           nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        result.data(), length, nullptr, nullptr);
    return result;
}

}  // namespace

// Kinds that are LISTBOX windows (the others with items are COMBOBOXes).
[[nodiscard]] static bool list_box_kind(const std::string& kind) {
    return kind == "ListBox" || kind == "FileListBox" || kind == "DirListBox";
}

struct Win32Host::Entry {
    std::string kind;
    WindowId id{kNoWindow};
    WindowId parent{kNoWindow};
    HWND hwnd{nullptr};
    int command{0};  // menu command id
    bool is_form{false};
    bool mdi_child{false};
    HWND client{nullptr};  // MDICLIENT of an MDI form
    bool is_menu{false};
    bool lightweight{false};
    Rect bounds;
    bool visible{false};
    bool enabled{true};
    bool checked{false};
    std::string caption;
    long timer_interval{0};
    // The persistent drawing surface of a form or picture box.
    HDC surface_dc{nullptr};
    HBITMAP surface_bitmap{nullptr};
    int surface_width{0};
    int surface_height{0};
    long surface_background{0xF0F0F0};
    long label_fore{-1};
    long label_back{-1};
    bool label_transparent{false};
    bool applying{false};   // a snapshot is being applied to a common control
    void* images{nullptr};  // HIMAGELIST handed to the control
    void* small_images{nullptr};
};

// Window procedure and helpers that need the host's internals.
struct Win32Support {
    using AtlAxWinInitFunction = BOOL(WINAPI*)();
    using AtlAxGetControlFunction = HRESULT(WINAPI*)(HWND, IUnknown**);
    static inline AtlAxGetControlFunction atl_get_control{nullptr};

    // Loads the system ATL library, whose "AtlAxWin" window class is a full
    // ActiveX control container.
    static bool load_atl() {
        static const bool loaded = [] {
            const HMODULE module = LoadLibraryW(L"atl.dll");
            if (module == nullptr) {
                return false;
            }
            const auto init = reinterpret_cast<AtlAxWinInitFunction>(
                GetProcAddress(module, "AtlAxWinInit"));
            atl_get_control = reinterpret_cast<AtlAxGetControlFunction>(
                GetProcAddress(module, "AtlAxGetControl"));
            return init != nullptr && atl_get_control != nullptr &&
                   init() != FALSE;
        }();
        return loaded;
    }

    // Mouse input of every hosted window (subclassed): MouseDown / MouseUp /
    // MouseMove events in twips, and the drop that ends a drag.
    static LRESULT CALLBACK mouse_proc(HWND hwnd, UINT message, WPARAM wparam,
                                       LPARAM lparam, UINT_PTR,
                                       DWORD_PTR data) {
        auto* const host = reinterpret_cast<Win32Host*>(data);
        long button = 0;
        bool down = false;
        bool up = false;
        switch (message) {
            case WM_LBUTTONDOWN:
                button = 1;
                down = true;
                break;
            case WM_RBUTTONDOWN:
                button = 2;
                down = true;
                break;
            case WM_MBUTTONDOWN:
                button = 4;
                down = true;
                break;
            case WM_LBUTTONUP:
                button = 1;
                up = true;
                break;
            case WM_RBUTTONUP:
                button = 2;
                up = true;
                break;
            case WM_MBUTTONUP:
                button = 4;
                up = true;
                break;
            case WM_MOUSEMOVE:
                break;
            default:
                return DefSubclassProc(hwnd, message, wparam, lparam);
        }
        const WindowId window = id_of(hwnd);
        if (host == nullptr || window == kNoWindow) {
            return DefSubclassProc(hwnd, message, wparam, lparam);
        }
        const auto to_twips = [&](const int pixels, const int dpi) {
            return static_cast<double>(pixels) * 1440.0 /
                   static_cast<double>(dpi);
        };
        if (up && host->drag_source_ != kNoWindow) {
            // The drag ends over whichever hosted window is under the pointer.
            const WindowId source = host->drag_source_;
            host->drag_source_ = kNoWindow;
            ReleaseCapture();
            POINT screen{static_cast<LONG>(static_cast<short>(LOWORD(lparam))),
                         static_cast<LONG>(static_cast<short>(HIWORD(lparam)))};
            ClientToScreen(hwnd, &screen);
            HWND target = WindowFromPoint(screen);
            while (target != nullptr && id_of(target) == kNoWindow) {
                target = GetParent(target);
            }
            if (target == nullptr) {
                target = hwnd;  // nothing of ours is there: the source itself
            }
            if (target != nullptr) {
                POINT inside = screen;
                ScreenToClient(target, &inside);
                host->queue(UiEvent{id_of(target),
                                    "DropOn",
                                    {static_cast<long>(source),
                                     to_twips(inside.x, host->dpi_x_),
                                     to_twips(inside.y, host->dpi_y_)}});
            }
            return 0;
        }
        long shift = 0;
        shift |= (GetKeyState(VK_SHIFT) & 0x8000) != 0 ? 1 : 0;
        shift |= (GetKeyState(VK_CONTROL) & 0x8000) != 0 ? 2 : 0;
        shift |= (GetKeyState(VK_MENU) & 0x8000) != 0 ? 4 : 0;
        if (message == WM_MOUSEMOVE) {
            button = ((wparam & MK_LBUTTON) != 0 ? 1 : 0) |
                     ((wparam & MK_RBUTTON) != 0 ? 2 : 0) |
                     ((wparam & MK_MBUTTON) != 0 ? 4 : 0);
        }
        host->queue(UiEvent{
            window,
            down ? "MouseDown"
            : up ? "MouseUp"
                 : "MouseMove",
            {button, shift,
             to_twips(static_cast<short>(LOWORD(lparam)), host->dpi_x_),
             to_twips(static_cast<short>(HIWORD(lparam)), host->dpi_y_)}});
        return DefSubclassProc(hwnd, message, wparam, lparam);
    }

    static Win32Host* host_of(HWND hwnd) {
        return reinterpret_cast<Win32Host*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    static WindowId id_of(HWND hwnd) {
        return static_cast<WindowId>(
            reinterpret_cast<std::uintptr_t>(GetPropW(hwnd, kIdProperty)));
    }

    static LRESULT CALLBACK form_proc(HWND hwnd, UINT message, WPARAM wparam,
                                      LPARAM lparam) {
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
            void* parameter = create->lpCreateParams;
            if ((create->dwExStyle & WS_EX_MDICHILD) != 0) {
                // The system wraps the creation parameter for MDI children.
                parameter = reinterpret_cast<void*>(
                    static_cast<const MDICREATESTRUCTW*>(parameter)->lParam);
            }
            SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>(parameter));
        }
        Win32Host* const host = host_of(hwnd);
        if (host == nullptr) {
            return DefWindowProcW(hwnd, message, wparam, lparam);
        }
        const WindowId form = id_of(hwnd);
        switch (message) {
            case WM_SIZE:
                if (const auto* frame = host->find(form);
                    frame != nullptr && frame->client != nullptr) {
                    RECT area;
                    GetClientRect(hwnd, &area);
                    MoveWindow(frame->client, 0, 0, area.right, area.bottom,
                               TRUE);
                }
                host->queue(UiEvent{form, "Resize", {}});
                break;
            case WM_CLOSE:
                host->queue(UiEvent{form, "Close", {}});
                return 0;
            case WM_ERASEBKGND:
                if (host->paint_surface(hwnd, reinterpret_cast<HDC>(wparam))) {
                    return 1;
                }
                break;
            case WM_CTLCOLORSTATIC:
                if (HBRUSH brush =
                        host->label_brush(reinterpret_cast<HWND>(lparam),
                                          reinterpret_cast<HDC>(wparam))) {
                    return reinterpret_cast<LRESULT>(brush);
                }
                break;
            case WM_TIMER:
                host->queue(
                    UiEvent{static_cast<WindowId>(wparam), "Timer", {}});
                return 0;
            case WM_NOTIFY: {
                const auto* header = reinterpret_cast<const NMHDR*>(lparam);
                if (header != nullptr &&
                    host->common_notify(header,
                                        static_cast<std::intptr_t>(lparam))) {
                    return 0;
                }
                break;
            }
            case WM_COMMAND: {
                const int code = HIWORD(wparam);
                if (lparam == 0) {
                    // A menu command.
                    for (const auto& [id, entry] : host->windows_) {
                        if (entry->is_menu &&
                            entry->command ==
                                static_cast<int>(LOWORD(wparam))) {
                            host->queue(UiEvent{id, "Click", {}});
                            break;
                        }
                    }
                    return 0;
                }
                const WindowId control = id_of(reinterpret_cast<HWND>(lparam));
                if (control == kNoWindow) {
                    break;
                }
                const auto* entry = host->find(control);
                if (entry == nullptr) {
                    break;
                }
                if (entry->kind == "Toolbar") {
                    host->queue(UiEvent{control,
                                        "ComctlInput",
                                        {std::string{"button"},
                                         std::to_string(LOWORD(wparam))}});
                    return 0;
                }
                if (entry->kind == "ImageCombo") {
                    if (entry->applying) {
                        return 0;
                    }
                    if (code == CBN_SELCHANGE) {
                        host->queue(UiEvent{
                            control,
                            "ComctlInput",
                            {std::string{"select"},
                             std::to_string(
                                 SendMessageW(reinterpret_cast<HWND>(lparam),
                                              CB_GETCURSEL, 0, 0) +
                                 1)}});
                    } else if (code == CBN_DROPDOWN) {
                        host->queue(
                            UiEvent{control,
                                    "ComctlInput",
                                    {std::string{"dropdown"}, std::string{}}});
                    } else if (code == CBN_EDITCHANGE) {
                        host->queue(UiEvent{
                            control,
                            "ComctlInput",
                            {std::string{"type"}, host->text(control)}});
                    }
                    return 0;
                }
                if (code == BN_CLICKED && !list_box_kind(entry->kind) &&
                    entry->kind != "ComboBox" &&
                    entry->kind != "DriveListBox") {
                    host->queue(UiEvent{control, "Click", {}});
                } else if (code == BN_DBLCLK) {
                    host->queue(UiEvent{control, "DblClick", {}});
                } else if (code == EN_CHANGE || code == CBN_EDITCHANGE) {
                    host->queue(UiEvent{control, "Change", {}});
                } else if (code == LBN_SELCHANGE || code == CBN_SELCHANGE) {
                    const LRESULT chosen =
                        SendMessageW(reinterpret_cast<HWND>(lparam),
                                     list_box_kind(entry->kind) ? LB_GETCURSEL
                                                                : CB_GETCURSEL,
                                     0, 0);
                    host->queue(UiEvent{
                        control, "UserSelect", {static_cast<long>(chosen)}});
                } else if (code == LBN_DBLCLK) {
                    host->queue(UiEvent{control, "DblClick", {}});
                }
                return 0;
            }
            case WM_HSCROLL:
            case WM_VSCROLL: {
                const HWND bar = reinterpret_cast<HWND>(lparam);
                const WindowId control = bar != nullptr ? id_of(bar) : 0U;
                if (control == kNoWindow) {
                    break;
                }
                if (const auto* slider = host->find(control);
                    slider != nullptr && slider->kind == "Slider") {
                    host->queue(UiEvent{
                        control,
                        "ComctlInput",
                        {std::string{"value"},
                         std::to_string(SendMessageW(bar, TBM_GETPOS, 0, 0))}});
                    return 0;
                }
                SCROLLINFO info{};
                info.cbSize = sizeof(info);
                info.fMask = SIF_ALL;
                GetScrollInfo(bar, SB_CTL, &info);
                const int request = LOWORD(wparam);
                if (request == SB_THUMBTRACK || request == SB_THUMBPOSITION) {
                    host->queue(UiEvent{control,
                                        "UserScroll",
                                        {static_cast<double>(info.nTrackPos)}});
                } else if (request != SB_ENDSCROLL) {
                    const int delta =
                        request == SB_LINEUP || request == SB_LINELEFT      ? -1
                        : request == SB_LINEDOWN || request == SB_LINERIGHT ? 1
                        : request == SB_PAGEUP || request == SB_PAGELEFT
                            ? -static_cast<int>(info.nPage)
                            : static_cast<int>(info.nPage);
                    host->queue(UiEvent{
                        control, "UserScrollBy", {static_cast<double>(delta)}});
                }
                return 0;
            }
            default:
                break;
        }
        if (const auto* entry = host->find(form)) {
            if (entry->client != nullptr) {
                return DefFrameProcW(hwnd, entry->client, message, wparam,
                                     lparam);
            }
        }
        if ((GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_MDICHILD) != 0) {
            // Also during creation, before the host registers the window.
            return DefMDIChildProcW(hwnd, message, wparam, lparam);
        }
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    static LRESULT CALLBACK picture_proc(HWND hwnd, UINT message, WPARAM wparam,
                                         LPARAM lparam) {
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
            SetWindowLongPtrW(
                hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        }
        Win32Host* const host = host_of(hwnd);
        if (host != nullptr && message == WM_ERASEBKGND &&
            host->paint_surface(hwnd, reinterpret_cast<HDC>(wparam))) {
            return 1;
        }
        if (host != nullptr && message == WM_CTLCOLORSTATIC) {
            if (HBRUSH brush =
                    host->label_brush(reinterpret_cast<HWND>(lparam),
                                      reinterpret_cast<HDC>(wparam))) {
                return reinterpret_cast<LRESULT>(brush);
            }
        }
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    static bool register_form_class() {
        static const bool registered = [] {
            WNDCLASSEXW window_class{};
            window_class.cbSize = sizeof(window_class);
            window_class.style = CS_HREDRAW | CS_VREDRAW;
            window_class.lpfnWndProc = form_proc;
            window_class.hInstance = GetModuleHandleW(nullptr);
            window_class.hCursor =
                LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
            window_class.hbrBackground = reinterpret_cast<HBRUSH>(
                static_cast<INT_PTR>(COLOR_BTNFACE + 1));
            window_class.lpszClassName = kFormClass;
            if (RegisterClassExW(&window_class) == 0) {
                return false;
            }
            window_class.lpfnWndProc = picture_proc;
            window_class.lpszClassName = kPictureClass;
            window_class.hbrBackground = nullptr;
            return RegisterClassExW(&window_class) != 0;
        }();
        return registered;
    }
};

Win32Host::Win32Host() {
    INITCOMMONCONTROLSEX controls{
        sizeof(controls), ICC_WIN95_CLASSES | ICC_BAR_CLASSES |
                              ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES |
                              ICC_TAB_CLASSES | ICC_USEREX_CLASSES |
                              ICC_PROGRESS_CLASS};
    InitCommonControlsEx(&controls);
    // System DPI awareness: windows are laid out from twips at the system DPI
    // instead of being bitmap-scaled by Windows. (Already set is fine.)
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
    if (const HDC context = GetDC(nullptr)) {
        dpi_x_ = GetDeviceCaps(context, LOGPIXELSX);
        dpi_y_ = GetDeviceCaps(context, LOGPIXELSY);
        ReleaseDC(nullptr, context);
        usable_ =
            dpi_x_ > 0 && dpi_y_ > 0 && Win32Support::register_form_class();
    }
    if (usable_) {
        // A locked workstation shows the secure desktop: windows would be
        // created but never shown or painted, so report the host unusable.
        if (const HDESK input =
                OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS)) {
            CloseDesktop(input);
        } else {
            usable_ = false;
        }
    }
    if (usable_) {
        // MS Sans Serif 8.25 pt, as VB6 forms use by default.
        font_ = CreateFontW(-MulDiv(8, static_cast<int>(dpi_y_), 72), 0, 0, 0,
                            FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS,
                            L"MS Shell Dlg");
    }
}

Win32Host::~Win32Host() {
    std::vector<WindowId> forms;
    for (const auto& [id, entry] : windows_) {
        if (entry->is_form) {
            forms.push_back(id);
        }
    }
    for (const WindowId id : forms) {
        destroy_window(id);
    }
    for (auto& [id, picture] : pictures_) {
        static_cast<IPicture*>(picture)->Release();
    }
    if (font_ != nullptr) {
        DeleteObject(font_);
    }
}

Win32Host::Entry* Win32Host::find(const WindowId window) {
    const auto found = windows_.find(window);
    return found == windows_.end() ? nullptr : found->second.get();
}

const Win32Host::Entry* Win32Host::find(const WindowId window) const {
    const auto found = windows_.find(window);
    return found == windows_.end() ? nullptr : found->second.get();
}

void Win32Host::queue(UiEvent event) {
    queue_.push_back(std::move(event));
}

HWND__* Win32Host::handle_of(const WindowId window) const {
    const auto* entry = find(window);
    return entry != nullptr ? entry->hwnd : nullptr;
}

WindowId Win32Host::create_window(const std::string& kind,
                                  const WindowId parent) {
    auto entry = std::make_unique<Entry>();
    entry->kind = kind;
    entry->id = next_id_++;
    entry->parent = parent;
    entry->is_form = kind == "Form" || kind == "MDIForm";
    entry->is_menu = kind == "Menu";
    HWND parent_handle = nullptr;
    if (const auto* parent_entry = find(parent)) {
        parent_handle = parent_entry->hwnd;
    }
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    const auto id_value =
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(entry->id));
    const auto make_child = [&](const wchar_t* class_name, DWORD style,
                                DWORD extended = 0) {
        entry->hwnd = CreateWindowExW(
            extended, class_name, L"", style | WS_CHILD | WS_CLIPSIBLINGS, 0, 0,
            0, 0, parent_handle, id_value, instance, nullptr);
    };
    if (kind.rfind("ActiveX:", 0) == 0 && parent_handle != nullptr) {
        // ATL's container window creates the control from its CLSID text and
        // sites it (in-place activation, ambient properties, sizing).
        if (Win32Support::load_atl()) {
            entry->hwnd = CreateWindowExW(
                0, L"AtlAxWin", widen(kind.substr(8)).c_str(),
                WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 0, 0,
                parent_handle, id_value, instance, nullptr);
        }
        if (entry->hwnd == nullptr) {
            entry->lightweight = true;
        }
    } else if (entry->is_form) {
        const auto* frame = find(parent);
        if (frame != nullptr && frame->client != nullptr) {
            entry->mdi_child = true;
            entry->hwnd = CreateWindowExW(
                WS_EX_MDICHILD, kFormClass, L"", WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, frame->client, nullptr,
                instance, this);
        } else {
            entry->hwnd = CreateWindowExW(
                0, kFormClass, L"", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, nullptr, nullptr,
                instance, this);
            if (kind == "MDIForm" && entry->hwnd != nullptr) {
                CLIENTCREATESTRUCT client_info{};
                client_info.idFirstChild = 50000;
                entry->client = CreateWindowExW(
                    WS_EX_CLIENTEDGE, L"MDICLIENT", L"",
                    WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                        WS_VSCROLL | WS_HSCROLL,
                    0, 0, 0, 0, entry->hwnd, nullptr, instance, &client_info);
            }
        }
    } else if (entry->is_menu) {
        entry->command = next_command_++;
    } else if (parent_handle == nullptr) {
        entry->lightweight = true;
    } else if (kind == "CommandButton") {
        make_child(L"BUTTON", BS_PUSHBUTTON | BS_MULTILINE | WS_TABSTOP);
    } else if (kind == "CheckBox") {
        make_child(L"BUTTON", BS_CHECKBOX | WS_TABSTOP);
    } else if (kind == "OptionButton") {
        make_child(L"BUTTON", BS_RADIOBUTTON | WS_TABSTOP);
    } else if (kind == "Frame") {
        make_child(L"BUTTON", BS_GROUPBOX);
    } else if (kind == "Label") {
        make_child(L"STATIC", SS_LEFT);
    } else if (kind == "TextBox") {
        make_child(L"EDIT", ES_AUTOHSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE);
    } else if (kind.rfind("TextBox:", 0) == 0) {
        // Multi-line: the scroll bars follow VB's ScrollBars setting.
        const int bars = std::atoi(kind.c_str() + 8);
        DWORD style =
            ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_TABSTOP;
        if ((bars & 2) != 0) {
            style |= WS_VSCROLL;
        }
        if ((bars & 1) != 0) {
            style |= WS_HSCROLL | ES_AUTOHSCROLL;
        }
        make_child(L"EDIT", style, WS_EX_CLIENTEDGE);
    } else if (kind == "ListBox" || kind == "FileListBox" ||
               kind == "DirListBox") {
        make_child(L"LISTBOX", LBS_NOTIFY | WS_VSCROLL | WS_TABSTOP,
                   WS_EX_CLIENTEDGE);
    } else if (kind == "DriveListBox") {
        make_child(L"COMBOBOX", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP);
    } else if (kind == "ComboBox") {
        make_child(L"COMBOBOX",
                   CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_VSCROLL | WS_TABSTOP);
    } else if (kind == "TabStrip") {
        make_child(L"SysTabControl32", TCS_TABS | WS_TABSTOP);
    } else if (kind == "Toolbar") {
        make_child(L"ToolbarWindow32", TBSTYLE_FLAT | TBSTYLE_LIST |
                                           CCS_NOPARENTALIGN | CCS_NORESIZE |
                                           CCS_NODIVIDER);
    } else if (kind == "StatusBar") {
        make_child(L"msctls_statusbar32",
                   CCS_NOPARENTALIGN | CCS_NORESIZE | SBARS_TOOLTIPS);
    } else if (kind == "ProgressBar") {
        make_child(L"msctls_progress32", 0);
    } else if (kind == "Slider") {
        make_child(L"msctls_trackbar32", TBS_AUTOTICKS | WS_TABSTOP);
    } else if (kind == "TreeView") {
        make_child(L"SysTreeView32",
                   TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT |
                       TVS_SHOWSELALWAYS | WS_TABSTOP,
                   WS_EX_CLIENTEDGE);
    } else if (kind == "ListView") {
        make_child(L"SysListView32",
                   LVS_REPORT | LVS_SHOWSELALWAYS | WS_TABSTOP,
                   WS_EX_CLIENTEDGE);
    } else if (kind == "ImageCombo") {
        make_child(L"ComboBoxEx32", CBS_DROPDOWN | WS_TABSTOP);
    } else if (kind == "HScrollBar") {
        make_child(L"SCROLLBAR", SBS_HORZ | WS_TABSTOP);
    } else if (kind == "VScrollBar") {
        make_child(L"SCROLLBAR", SBS_VERT | WS_TABSTOP);
    } else if (kind == "PictureBox" || kind == "UserControl" || kind == "OLE") {
        entry->hwnd =
            CreateWindowExW(WS_EX_CLIENTEDGE, kPictureClass, L"",
                            WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0,
                            0, 0, parent_handle, id_value, instance, this);
        entry->surface_background = 0xFFFFFF;
    } else {
        entry->lightweight = true;  // Timer, Shape, Line, Image, ...
    }
    if (entry->hwnd != nullptr) {
        SetPropW(entry->hwnd, kIdProperty,
                 reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(entry->id)));
        if (kind.rfind("ActiveX:", 0) != 0) {
            SetWindowSubclass(entry->hwnd, Win32Support::mouse_proc, 1,
                              reinterpret_cast<DWORD_PTR>(this));
        }
        if (!entry->is_form && font_ != nullptr) {
            SendMessageW(entry->hwnd, WM_SETFONT,
                         reinterpret_cast<WPARAM>(font_), TRUE);
        }
    }
    const WindowId id = entry->id;
    const bool menu = entry->is_menu;
    windows_.emplace(id, std::move(entry));
    if (menu) {
        // A menu belongs to the nearest form above it.
        WindowId form = parent;
        while (const auto* up = find(form)) {
            if (up->is_form) {
                break;
            }
            form = up->parent;
        }
        refresh_menu_bar(form);
    }
    return id;
}

void Win32Host::refresh_menu_bar(const WindowId form) {
    auto* form_entry = find(form);
    if (form_entry == nullptr || form_entry->hwnd == nullptr ||
        form_entry->mdi_child) {
        return;
    }
    // Rebuild the bar from the menu entries in creation order.
    const auto build = [&](const auto& self, const WindowId owner) -> HMENU {
        HMENU menu = owner == form ? CreateMenu() : CreatePopupMenu();
        for (const auto& [id, entry] : windows_) {
            if (!entry->is_menu || entry->parent != owner || !entry->visible) {
                continue;
            }
            bool has_children = false;
            for (const auto& [other_id, other] : windows_) {
                has_children =
                    has_children || (other->is_menu && other->parent == id);
            }
            UINT flags = MF_STRING;
            flags |= entry->enabled ? MF_ENABLED : MF_GRAYED;
            flags |= entry->checked ? MF_CHECKED : MF_UNCHECKED;
            const auto text = widen(entry->caption);
            if (entry->caption == "-") {
                AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            } else if (has_children) {
                AppendMenuW(menu, flags | MF_POPUP,
                            reinterpret_cast<UINT_PTR>(self(self, id)),
                            text.c_str());
            } else {
                AppendMenuW(menu, flags, static_cast<UINT_PTR>(entry->command),
                            text.c_str());
            }
        }
        return menu;
    };
    HMENU previous = GetMenu(form_entry->hwnd);
    SetMenu(form_entry->hwnd, build(build, form));
    if (previous != nullptr) {
        DestroyMenu(previous);
    }
    DrawMenuBar(form_entry->hwnd);
}

void Win32Host::destroy_window(const WindowId window) {
    auto* entry = find(window);
    if (entry == nullptr) {
        return;
    }
    std::vector<WindowId> children;
    for (const auto& [id, child] : windows_) {
        if (child->parent == window) {
            children.push_back(id);
        }
    }
    for (const WindowId child : children) {
        destroy_window(child);
    }
    entry = find(window);
    if (entry == nullptr) {
        return;
    }
    std::erase_if(queue_, [window](const UiEvent& event) {
        return event.window == window;
    });
    const bool menu = entry->is_menu;
    WindowId form = entry->parent;
    HWND handle = entry->hwnd;
    if (entry->surface_dc != nullptr) {
        DeleteDC(entry->surface_dc);
        DeleteObject(entry->surface_bitmap);
    }
    windows_.erase(window);
    if (handle != nullptr) {
        DestroyWindow(handle);
    }
    if (menu) {
        while (const auto* up = find(form)) {
            if (up->is_form) {
                break;
            }
            form = up->parent;
        }
        refresh_menu_bar(form);
    }
}

void Win32Host::set_bounds(const WindowId window, const Rect& bounds) {
    auto* entry = find(window);
    if (entry == nullptr) {
        return;
    }
    entry->bounds = bounds;
    if (entry->hwnd == nullptr) {
        return;
    }
    const auto to_x = [&](const double twips) {
        return static_cast<int>(twips * static_cast<double>(dpi_x_) / 1440.0);
    };
    const auto to_y = [&](const double twips) {
        return static_cast<int>(twips * static_cast<double>(dpi_y_) / 1440.0);
    };
    int width = std::max(to_x(bounds.width), 1);
    int height = std::max(to_y(bounds.height), 1);
    if (entry->is_form) {
        // A form's size is its client area; add the frame, title and menu.
        RECT frame{0, 0, width, height};
        AdjustWindowRectEx(
            &frame,
            static_cast<DWORD>(GetWindowLongPtrW(entry->hwnd, GWL_STYLE)),
            GetMenu(entry->hwnd) != nullptr,
            static_cast<DWORD>(GetWindowLongPtrW(entry->hwnd, GWL_EXSTYLE)));
        width = frame.right - frame.left;
        height = frame.bottom - frame.top;
    }
    SetWindowPos(entry->hwnd, nullptr, to_x(bounds.left), to_y(bounds.top),
                 width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

Rect Win32Host::bounds(const WindowId window) const {
    const auto* entry = find(window);
    return entry != nullptr ? entry->bounds : Rect{};
}

void Win32Host::set_visible(const WindowId window, const bool visible) {
    auto* entry = find(window);
    if (entry == nullptr) {
        return;
    }
    entry->visible = visible;
    if (entry->is_menu) {
        WindowId form = entry->parent;
        while (const auto* up = find(form)) {
            if (up->is_form) {
                break;
            }
            form = up->parent;
        }
        refresh_menu_bar(form);
        return;
    }
    if (entry->hwnd != nullptr) {
        ShowWindow(entry->hwnd, visible ? SW_SHOW : SW_HIDE);
    }
}

bool Win32Host::visible(const WindowId window) const {
    const auto* entry = find(window);
    return entry != nullptr && entry->visible;
}

void Win32Host::set_border(const WindowId window, const bool border) {
    const auto it = windows_.find(window);
    if (it == windows_.end() || it->second->hwnd == nullptr) {
        return;
    }
    const HWND hwnd = it->second->hwnd;
    auto style = static_cast<LONG_PTR>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
    style = border ? (style | WS_EX_CLIENTEDGE) : (style & ~WS_EX_CLIENTEDGE);
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, style);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                     SWP_FRAMECHANGED);
}

void Win32Host::set_label_style(const WindowId window, const long fore,
                                const long back, const bool transparent) {
    const auto it = windows_.find(window);
    if (it == windows_.end() || it->second->hwnd == nullptr) {
        return;
    }
    Entry& entry = *it->second;
    entry.label_fore = fore;
    entry.label_back = back;
    entry.label_transparent = transparent;
    auto style =
        static_cast<LONG_PTR>(GetWindowLongPtrW(entry.hwnd, GWL_EXSTYLE));
    style = transparent ? (style | WS_EX_TRANSPARENT)
                        : (style & ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT));
    SetWindowLongPtrW(entry.hwnd, GWL_EXSTYLE, style);
    if (HWND parent = GetParent(entry.hwnd)) {
        RECT area;
        GetWindowRect(entry.hwnd, &area);
        MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&area), 2);
        InvalidateRect(parent, &area, TRUE);
    }
    InvalidateRect(entry.hwnd, nullptr, TRUE);
}

HBRUSH Win32Host::label_brush(HWND label, HDC dc) {
    const auto id = static_cast<WindowId>(
        reinterpret_cast<std::uintptr_t>(GetPropW(label, kIdProperty)));
    const auto* entry = find(id);
    if (entry == nullptr || entry->kind != "Label") {
        return nullptr;
    }
    if (entry->label_fore >= 0) {
        SetTextColor(dc, static_cast<COLORREF>(entry->label_fore));
    }
    if (entry->label_transparent) {
        SetBkMode(dc, TRANSPARENT);
        return static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    }
    if (entry->label_back < 0) {
        return nullptr;
    }
    SetBkColor(dc, static_cast<COLORREF>(entry->label_back));
    auto& brush = label_brushes_[entry->label_back];
    if (brush == nullptr) {
        brush = CreateSolidBrush(static_cast<COLORREF>(entry->label_back));
    }
    return brush;
}

void Win32Host::set_enabled(const WindowId window, const bool enabled) {
    auto* entry = find(window);
    if (entry == nullptr) {
        return;
    }
    entry->enabled = enabled;
    if (entry->is_menu) {
        WindowId form = entry->parent;
        while (const auto* up = find(form)) {
            if (up->is_form) {
                break;
            }
            form = up->parent;
        }
        refresh_menu_bar(form);
    } else if (entry->hwnd != nullptr) {
        EnableWindow(entry->hwnd, enabled ? TRUE : FALSE);
    }
}

bool Win32Host::enabled(const WindowId window) const {
    const auto* entry = find(window);
    return entry != nullptr && entry->enabled;
}

void Win32Host::set_text(const WindowId window, const std::string& text) {
    auto* entry = find(window);
    if (entry == nullptr) {
        return;
    }
    entry->caption = text;
    if (entry->is_menu) {
        WindowId form = entry->parent;
        while (const auto* up = find(form)) {
            if (up->is_form) {
                break;
            }
            form = up->parent;
        }
        refresh_menu_bar(form);
    } else if (entry->hwnd != nullptr) {
        SetWindowTextW(entry->hwnd, widen(text).c_str());
    }
}

std::string Win32Host::text(const WindowId window) const {
    const auto* entry = find(window);
    if (entry == nullptr) {
        return {};
    }
    if (entry->hwnd == nullptr) {
        return entry->caption;
    }
    const int length = GetWindowTextLengthW(entry->hwnd);
    std::wstring buffer(static_cast<std::size_t>(length) + 1U, L'\0');
    const int copied = GetWindowTextW(entry->hwnd, buffer.data(), length + 1);
    buffer.resize(static_cast<std::size_t>(std::max(copied, 0)));
    return narrow(buffer);
}

void Win32Host::set_items(const WindowId window,
                          const std::vector<std::string>& items) {
    const auto* entry = find(window);
    if (entry == nullptr || entry->hwnd == nullptr) {
        return;
    }
    const bool list = list_box_kind(entry->kind);
    SendMessageW(entry->hwnd, list ? LB_RESETCONTENT : CB_RESETCONTENT, 0, 0);
    for (const auto& item : items) {
        SendMessageW(entry->hwnd, list ? LB_ADDSTRING : CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(widen(item).c_str()));
    }
}

void Win32Host::set_selection(const WindowId window, const long index) {
    const auto* entry = find(window);
    if (entry == nullptr || entry->hwnd == nullptr) {
        return;
    }
    SendMessageW(entry->hwnd,
                 list_box_kind(entry->kind) ? LB_SETCURSEL : CB_SETCURSEL,
                 static_cast<WPARAM>(index), 0);
}

long Win32Host::selection(const WindowId window) const {
    const auto* entry = find(window);
    if (entry == nullptr || entry->hwnd == nullptr) {
        return -1;
    }
    return static_cast<long>(SendMessageW(
        entry->hwnd, list_box_kind(entry->kind) ? LB_GETCURSEL : CB_GETCURSEL,
        0, 0));
}

void Win32Host::set_value(const WindowId window, const long value) {
    auto* entry = find(window);
    if (entry == nullptr) {
        return;
    }
    entry->checked = value != 0;
    if (entry->is_menu) {
        WindowId form = entry->parent;
        while (const auto* up = find(form)) {
            if (up->is_form) {
                break;
            }
            form = up->parent;
        }
        refresh_menu_bar(form);
    } else if (entry->hwnd != nullptr) {
        SendMessageW(entry->hwnd, BM_SETCHECK,
                     value != 0 ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

void Win32Host::set_scroll(const WindowId window, const long minimum,
                           const long maximum, const long value,
                           const long small_change, const long large_change) {
    static_cast<void>(small_change);
    const auto* entry = find(window);
    if (entry == nullptr || entry->hwnd == nullptr) {
        return;
    }
    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_ALL;
    info.nMin = static_cast<int>(std::min(minimum, maximum));
    const int page = static_cast<int>(std::max(large_change, 1L));
    info.nPage = static_cast<UINT>(page);
    // A scroll bar's largest position is nMax - nPage + 1.
    info.nMax = static_cast<int>(std::max(minimum, maximum)) + page - 1;
    info.nPos = static_cast<int>(value);
    SetScrollInfo(entry->hwnd, SB_CTL, &info, TRUE);
}

namespace {

[[nodiscard]] COLORREF to_colorref(const long color) {
    // 0x00BBGGRR is already a COLORREF.
    return static_cast<COLORREF>(color & 0xFFFFFFL);
}

}  // namespace

Win32Host::Entry* Win32Host::ensure_surface(const WindowId window) {
    auto* entry = find(window);
    if (entry == nullptr || entry->hwnd == nullptr) {
        return nullptr;
    }
    RECT client{};
    GetClientRect(entry->hwnd, &client);
    const int width = std::max(static_cast<int>(client.right), 1);
    const int height = std::max(static_cast<int>(client.bottom), 1);
    if (entry->surface_dc != nullptr && entry->surface_width >= width &&
        entry->surface_height >= height) {
        return entry;
    }
    // (Re)create at least as large as the client area, keeping the contents.
    const int new_width = std::max(width, entry->surface_width);
    const int new_height = std::max(height, entry->surface_height);
    HDC screen = GetDC(entry->hwnd);
    HDC memory = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, new_width, new_height);
    ReleaseDC(entry->hwnd, screen);
    HGDIOBJ previous = SelectObject(memory, bitmap);
    static_cast<void>(previous);
    HBRUSH background =
        CreateSolidBrush(to_colorref(entry->surface_background));
    RECT whole{0, 0, new_width, new_height};
    FillRect(memory, &whole, background);
    DeleteObject(background);
    if (entry->surface_dc != nullptr) {
        BitBlt(memory, 0, 0, entry->surface_width, entry->surface_height,
               entry->surface_dc, 0, 0, SRCCOPY);
        DeleteDC(entry->surface_dc);
        DeleteObject(entry->surface_bitmap);
    }
    entry->surface_dc = memory;
    entry->surface_bitmap = bitmap;
    entry->surface_width = new_width;
    entry->surface_height = new_height;
    return entry;
}

bool Win32Host::paint_surface(HWND__* hwnd, HDC__* target) {
    const WindowId window = Win32Support::id_of(hwnd);
    auto* entry = find(window);
    if (entry == nullptr || entry->surface_dc == nullptr) {
        return false;
    }
    RECT client{};
    GetClientRect(hwnd, &client);
    BitBlt(target, 0, 0, std::min<int>(client.right, entry->surface_width),
           std::min<int>(client.bottom, entry->surface_height),
           entry->surface_dc, 0, 0, SRCCOPY);
    return true;
}

std::optional<long> Win32Host::message_box(const WindowId owner,
                                           const std::string& text,
                                           const std::string& title,
                                           const long style) {
    const auto* entry = find(owner);
    HWND parent = entry != nullptr ? entry->hwnd : nullptr;
    // VB's button, icon, default-button and modality bits are MB_* values.
    const int result =
        MessageBoxW(parent, widen(text).c_str(), widen(title).c_str(),
                    static_cast<UINT>(style) | MB_SETFOREGROUND);
    return static_cast<long>(result);
}

namespace {

struct InputDialog {
    std::wstring prompt;
    std::wstring initial;
    std::wstring result;
    bool accepted{false};
    bool done{false};
    HWND edit{nullptr};
};

constexpr int kEditId = 101;

LRESULT CALLBACK input_proc(HWND hwnd, UINT message, WPARAM wparam,
                            LPARAM lparam) {
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    auto* dialog =
        reinterpret_cast<InputDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (dialog == nullptr) {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }
    if (message == WM_COMMAND) {
        const int id = LOWORD(wparam);
        if (id == IDOK || id == IDCANCEL) {
            if (id == IDOK) {
                const int length = GetWindowTextLengthW(dialog->edit);
                std::wstring text(static_cast<std::size_t>(length) + 1U, L'\0');
                GetWindowTextW(dialog->edit, text.data(), length + 1);
                text.resize(static_cast<std::size_t>(length));
                dialog->result = text;
                dialog->accepted = true;
            }
            dialog->done = true;
            return 0;
        }
    } else if (message == WM_CLOSE) {
        dialog->done = true;
        return 0;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

}  // namespace

std::optional<std::string> Win32Host::input_box(const WindowId owner,
                                                const std::string& prompt,
                                                const std::string& title,
                                                const std::string& initial,
                                                bool& supported) {
    supported = true;
    static const bool registered = [] {
        WNDCLASSEXW window_class{};
        window_class.cbSize = sizeof(window_class);
        window_class.lpfnWndProc = input_proc;
        window_class.hInstance = GetModuleHandleW(nullptr);
        window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        window_class.hbrBackground =
            reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(COLOR_BTNFACE + 1));
        window_class.lpszClassName = L"WfcInputBox";
        return RegisterClassExW(&window_class) != 0;
    }();
    if (!registered) {
        supported = false;
        return std::nullopt;
    }
    InputDialog dialog;
    dialog.prompt = widen(prompt);
    dialog.initial = widen(initial);
    const auto* entry = find(owner);
    HWND parent = entry != nullptr ? entry->hwnd : nullptr;
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    const int width = 420;
    const int height = 190;
    const int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
    HWND window = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, L"WfcInputBox",
        widen(title).c_str(), WS_POPUP | WS_CAPTION | WS_SYSMENU, x, y, width,
        height, parent, nullptr, instance, &dialog);
    if (window == nullptr) {
        supported = false;
        return std::nullopt;
    }
    const auto child = [&](const wchar_t* cls, const wchar_t* text, DWORD style,
                           const int cx, const int cy, const int cw,
                           const int ch, const int id) {
        HWND control = CreateWindowExW(
            0, cls, text, style | WS_CHILD | WS_VISIBLE, cx, cy, cw, ch, window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance,
            nullptr);
        if (font_ != nullptr) {
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font_),
                         TRUE);
        }
        return control;
    };
    child(L"STATIC", dialog.prompt.c_str(), SS_LEFT, 12, 10, 390, 60, 100);
    dialog.edit = child(L"EDIT", dialog.initial.c_str(),
                        ES_AUTOHSCROLL | WS_TABSTOP | WS_BORDER, 12, 80, 390,
                        24, kEditId);
    child(L"BUTTON", L"OK", BS_DEFPUSHBUTTON | WS_TABSTOP, 220, 118, 85, 26,
          IDOK);
    child(L"BUTTON", L"Cancel", BS_PUSHBUTTON | WS_TABSTOP, 317, 118, 85, 26,
          IDCANCEL);
    ShowWindow(window, SW_SHOW);
    SetForegroundWindow(window);
    SetFocus(dialog.edit);
    SendMessageW(dialog.edit, EM_SETSEL, 0, -1);
    if (parent != nullptr) {
        EnableWindow(parent, FALSE);
    }
    MSG message;
    while (!dialog.done && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(window, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    if (parent != nullptr) {
        EnableWindow(parent, TRUE);
    }
    DestroyWindow(window);
    if (!dialog.accepted) {
        return std::nullopt;
    }
    return narrow(dialog.result);
}

bool Win32Host::send_key(const long code, const long shift, const long ascii) {
    std::vector<INPUT> inputs;
    const auto key = [&](const WORD virtual_key, const bool down) {
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = virtual_key;
        input.ki.dwFlags = down ? 0U : KEYEVENTF_KEYUP;
        inputs.push_back(input);
    };
    if ((shift & 1) != 0) {
        key(VK_SHIFT, true);
    }
    if ((shift & 2) != 0) {
        key(VK_CONTROL, true);
    }
    if ((shift & 4) != 0) {
        key(VK_MENU, true);
    }
    key(static_cast<WORD>(code), true);
    key(static_cast<WORD>(code), false);
    if ((shift & 4) != 0) {
        key(VK_MENU, false);
    }
    if ((shift & 2) != 0) {
        key(VK_CONTROL, false);
    }
    if ((shift & 1) != 0) {
        key(VK_SHIFT, false);
    }
    static_cast<void>(ascii);
    const UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(),
                                sizeof(INPUT));
    pump();
    return sent == inputs.size();
}

void* Win32Host::activex_object(const WindowId window) {
    const auto* entry = find(window);
    if (entry == nullptr || entry->hwnd == nullptr ||
        Win32Support::atl_get_control == nullptr) {
        return nullptr;
    }
    IUnknown* object = nullptr;
    if (FAILED(Win32Support::atl_get_control(entry->hwnd, &object))) {
        return nullptr;
    }
    return object;  // AtlAxGetControl added the reference
}

void Win32Host::begin_drag(const WindowId window) {
    if (const auto* entry = find(window);
        entry != nullptr && entry->hwnd != nullptr) {
        drag_source_ = window;
        SetCapture(entry->hwnd);
    }
}

void Win32Host::set_button_image(const WindowId window, const long picture) {
    const auto* entry = find(window);
    const auto known = pictures_.find(picture);
    if (entry == nullptr || entry->hwnd == nullptr ||
        known == pictures_.end()) {
        return;
    }
    auto* const image = static_cast<IPicture*>(known->second);
    SHORT type = 0;
    OLE_HANDLE handle = 0;
    if (FAILED(image->get_Type(&type)) || FAILED(image->get_Handle(&handle))) {
        return;
    }
    const bool icon = type == PICTYPE_ICON;
    LONG_PTR style = GetWindowLongPtrW(entry->hwnd, GWL_STYLE);
    style &= ~static_cast<LONG_PTR>(BS_BITMAP | BS_ICON);
    style |= icon ? BS_ICON : BS_BITMAP;
    SetWindowLongPtrW(entry->hwnd, GWL_STYLE, style);
    SendMessageW(entry->hwnd, BM_SETIMAGE, icon ? IMAGE_ICON : IMAGE_BITMAP,
                 static_cast<LPARAM>(handle));
}

void Win32Host::arrange(const WindowId mdi_form, const long mode) {
    const auto* entry = find(mdi_form);
    if (entry == nullptr || entry->client == nullptr) {
        return;
    }
    if (mode == 0) {
        SendMessageW(entry->client, WM_MDICASCADE, 0, 0);
    } else if (mode == 1 || mode == 2) {
        SendMessageW(entry->client, WM_MDITILE,
                     mode == 1 ? MDITILE_HORIZONTAL : MDITILE_VERTICAL, 0);
    } else {
        SendMessageW(entry->client, WM_MDIICONARRANGE, 0, 0);
    }
}

void Win32Host::pump() {
    MSG message;
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        bool handled = false;
        for (const auto& [id, window] : windows_) {
            if (window->client != nullptr &&
                TranslateMDISysAccel(window->client, &message)) {
                handled = true;
                break;
            }
        }
        if (handled) {
            continue;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

bool Win32Host::capture(const WindowId window, const std::string& path) {
    const auto* entry = find(window);
    if (entry == nullptr || entry->hwnd == nullptr) {
        return false;
    }
    pump();
    Sleep(60);  // let the window manager show a window created a moment ago
    pump();
    // A form shown from inside a handler has not painted yet.
    RedrawWindow(entry->hwnd, nullptr, nullptr,
                 RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    RECT area{};
    GetWindowRect(entry->hwnd, &area);
    const int width = area.right - area.left;
    const int height = area.bottom - area.top;
    if (width <= 0 || height <= 0) {
        return false;
    }
    HDC screen = GetDC(nullptr);
    HDC memory = CreateCompatibleDC(screen);
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;  // top-down
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* pixels = nullptr;
    HBITMAP bitmap =
        CreateDIBSection(screen, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    ReleaseDC(nullptr, screen);
    if (bitmap == nullptr) {
        DeleteDC(memory);
        return false;
    }
    HGDIOBJ previous = SelectObject(memory, bitmap);
    // PW_RENDERFULLCONTENT (2) also renders layered and child windows.
    const bool printed = PrintWindow(entry->hwnd, memory, 2U) != FALSE;
    if (printed && entry->client != nullptr) {
        // PrintWindow leaves the MDI children out; composite them.
        for (HWND child = GetWindow(entry->client, GW_CHILD); child != nullptr;
             child = GetWindow(child, GW_HWNDNEXT)) {
            RECT bounds;
            if (!IsWindowVisible(child) || IsIconic(child) ||
                !GetWindowRect(child, &bounds)) {
                continue;
            }
            HDC child_dc = CreateCompatibleDC(memory);
            HBITMAP child_bitmap = CreateCompatibleBitmap(
                memory, bounds.right - bounds.left, bounds.bottom - bounds.top);
            const HGDIOBJ old = SelectObject(child_dc, child_bitmap);
            if (PrintWindow(child, child_dc, 2U) != FALSE) {
                BitBlt(memory, bounds.left - area.left, bounds.top - area.top,
                       bounds.right - bounds.left, bounds.bottom - bounds.top,
                       child_dc, 0, 0, SRCCOPY);
            }
            SelectObject(child_dc, old);
            DeleteObject(child_bitmap);
            DeleteDC(child_dc);
        }
    }
    SelectObject(memory, previous);
    bool saved = false;
    if (printed) {
        IWICImagingFactory* factory = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                       CLSCTX_INPROC_SERVER,
                                       IID_PPV_ARGS(&factory))) &&
            factory != nullptr) {
            IWICBitmapEncoder* encoder = nullptr;
            IWICStream* stream = nullptr;
            IWICBitmapFrameEncode* frame = nullptr;
            IPropertyBag2* options = nullptr;
            const auto wide = widen(path);
            if (SUCCEEDED(factory->CreateStream(&stream)) &&
                SUCCEEDED(stream->InitializeFromFilename(wide.c_str(),
                                                         GENERIC_WRITE)) &&
                SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng,
                                                 nullptr, &encoder)) &&
                SUCCEEDED(
                    encoder->Initialize(stream, WICBitmapEncoderNoCache)) &&
                SUCCEEDED(encoder->CreateNewFrame(&frame, &options)) &&
                SUCCEEDED(frame->Initialize(options)) &&
                SUCCEEDED(frame->SetSize(static_cast<UINT>(width),
                                         static_cast<UINT>(height)))) {
                WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
                frame->SetPixelFormat(&format);
                // The window is opaque: force the alpha channel to 255.
                auto* data = static_cast<std::uint8_t*>(pixels);
                for (std::size_t i = 3;
                     i < static_cast<std::size_t>(width) *
                             static_cast<std::size_t>(height) * 4U;
                     i += 4U) {
                    data[i] = 0xFF;
                }
                if (SUCCEEDED(frame->WritePixels(static_cast<UINT>(height),
                                                 static_cast<UINT>(width) * 4U,
                                                 static_cast<UINT>(width) *
                                                     static_cast<UINT>(height) *
                                                     4U,
                                                 static_cast<BYTE*>(pixels))) &&
                    SUCCEEDED(frame->Commit()) &&
                    SUCCEEDED(encoder->Commit())) {
                    saved = true;
                }
            }
            if (options != nullptr) {
                options->Release();
            }
            if (frame != nullptr) {
                frame->Release();
            }
            if (encoder != nullptr) {
                encoder->Release();
            }
            if (stream != nullptr) {
                stream->Release();
            }
            factory->Release();
        }
    }
    DeleteObject(bitmap);
    DeleteDC(memory);
    return saved;
}

long Win32Host::load_picture(const std::string& bytes) {
    const auto known = picture_ids_.find(bytes);
    if (known != picture_ids_.end()) {
        return known->second;
    }
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes.size());
    if (memory == nullptr) {
        return 0;
    }
    void* locked = GlobalLock(memory);
    std::memcpy(locked, bytes.data(), bytes.size());
    GlobalUnlock(memory);
    IStream* stream = nullptr;
    long id = 0;
    if (SUCCEEDED(CreateStreamOnHGlobal(memory, TRUE, &stream))) {
        IPicture* picture = nullptr;
        if (SUCCEEDED(OleLoadPicture(stream, static_cast<LONG>(bytes.size()),
                                     FALSE, IID_IPicture,
                                     reinterpret_cast<void**>(&picture))) &&
            picture != nullptr) {
            id = static_cast<long>(pictures_.size()) + 1;
            pictures_.emplace(id, picture);
            picture_ids_.emplace(bytes, id);
        }
        stream->Release();
    } else {
        GlobalFree(memory);
    }
    return id;
}

std::optional<long> Win32Host::pixel(const WindowId window, const double x,
                                     const double y) const {
    const auto* entry = find(window);
    if (entry == nullptr || entry->surface_dc == nullptr) {
        return std::nullopt;
    }
    const int px =
        static_cast<int>(std::floor(x * static_cast<double>(dpi_x_) / 1440.0));
    const int py =
        static_cast<int>(std::floor(y * static_cast<double>(dpi_y_) / 1440.0));
    if (px < 0 || py < 0 || px >= entry->surface_width ||
        py >= entry->surface_height) {
        return std::nullopt;
    }
    const COLORREF color = GetPixel(entry->surface_dc, px, py);
    if (color == CLR_INVALID) {
        return std::nullopt;
    }
    return static_cast<long>(color);
}

void Win32Host::draw(const WindowId window, const DrawOp& operation) {
    Entry* entry = ensure_surface(window);
    if (entry == nullptr) {
        return;
    }
    const auto to_x = [&](const double twips) {
        return static_cast<int>(
            std::lround(twips * static_cast<double>(dpi_x_) / 1440.0));
    };
    const auto to_y = [&](const double twips) {
        return static_cast<int>(
            std::lround(twips * static_cast<double>(dpi_y_) / 1440.0));
    };
    HDC dc = entry->surface_dc;
    const COLORREF color = to_colorref(operation.color);
    if (operation.kind == "cls") {
        HBRUSH brush = CreateSolidBrush(color);
        RECT whole{0, 0, entry->surface_width, entry->surface_height};
        FillRect(dc, &whole, brush);
        DeleteObject(brush);
        entry->surface_background = operation.color;
    } else if (operation.kind == "pset") {
        SetPixelV(dc, to_x(operation.x1), to_y(operation.y1), color);
    } else if (operation.kind == "line") {
        const int x1 = to_x(operation.x1);
        const int y1 = to_y(operation.y1);
        const int x2 = to_x(operation.x2);
        const int y2 = to_y(operation.y2);
        if (operation.box && operation.filled) {
            HBRUSH brush = CreateSolidBrush(color);
            RECT area{std::min(x1, x2), std::min(y1, y2), std::max(x1, x2) + 1,
                      std::max(y1, y2) + 1};
            FillRect(dc, &area, brush);
            DeleteObject(brush);
        } else {
            HPEN pen = CreatePen(PS_SOLID, operation.draw_width, color);
            HGDIOBJ old_pen = SelectObject(dc, pen);
            if (operation.box) {
                // A box is filled with FillColor unless FillStyle is
                // transparent.
                HBRUSH box_fill =
                    operation.fill_style == 1
                        ? static_cast<HBRUSH>(GetStockObject(NULL_BRUSH))
                        : CreateSolidBrush(to_colorref(operation.fill_color));
                HGDIOBJ old_brush = SelectObject(dc, box_fill);
                Rectangle(dc, std::min(x1, x2), std::min(y1, y2),
                          std::max(x1, x2) + 1, std::max(y1, y2) + 1);
                SelectObject(dc, old_brush);
                if (operation.fill_style != 1) {
                    DeleteObject(box_fill);
                }
            } else {
                MoveToEx(dc, x1, y1, nullptr);
                LineTo(dc, x2, y2);
            }
            SelectObject(dc, old_pen);
            DeleteObject(pen);
        }
    } else if (operation.kind == "circle") {
        const int cx = to_x(operation.x1);
        const int cy = to_y(operation.y1);
        const int rx = std::max(to_x(operation.radius), 1);
        const int ry =
            std::max(static_cast<int>(std::lround(static_cast<double>(rx) *
                                                  operation.aspect)),
                     1);
        HPEN pen = CreatePen(PS_SOLID, operation.draw_width, color);
        HGDIOBJ old_pen = SelectObject(dc, pen);
        const bool fill_circle = operation.fill_style != 1 && !operation.arc;
        HBRUSH circle_fill =
            fill_circle ? CreateSolidBrush(to_colorref(operation.fill_color))
                        : static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
        HGDIOBJ old_brush = SelectObject(dc, circle_fill);
        if (operation.arc) {
            const double start = std::fabs(operation.start);
            const double end = std::fabs(operation.end);
            // GDI arcs run counter-clockwise from the start point to the
            // end point; both lie on the ellipse.
            Arc(dc, cx - rx, cy - ry, cx + rx, cy + ry,
                cx + static_cast<int>(std::lround(rx * std::cos(start))),
                cy - static_cast<int>(std::lround(ry * std::sin(start))),
                cx + static_cast<int>(std::lround(rx * std::cos(end))),
                cy - static_cast<int>(std::lround(ry * std::sin(end))));
        } else {
            Ellipse(dc, cx - rx, cy - ry, cx + rx, cy + ry);
        }
        SelectObject(dc, old_brush);
        if (fill_circle) {
            DeleteObject(circle_fill);
        }
        SelectObject(dc, old_pen);
        DeleteObject(pen);
    } else if (operation.kind == "shape") {
        const int left = to_x(operation.x1);
        const int top = to_y(operation.y1);
        int right = left + std::max(to_x(operation.x2), 1);
        int bottom = top + std::max(to_y(operation.y2), 1);
        const int type = static_cast<int>(operation.radius);
        if (type == 1 || type == 3 || type == 5) {  // square, circle, rounded
            const int side = std::min(right - left, bottom - top);
            right = left + side;
            bottom = top + side;
        }
        HPEN pen = operation.draw_style == 5
                       ? static_cast<HPEN>(GetStockObject(NULL_PEN))
                       : CreatePen(PS_SOLID, operation.draw_width, color);
        HBRUSH brush =
            operation.fill_style == 1
                ? static_cast<HBRUSH>(GetStockObject(NULL_BRUSH))
                : CreateSolidBrush(to_colorref(operation.fill_color));
        HGDIOBJ old_pen = SelectObject(dc, pen);
        HGDIOBJ old_brush = SelectObject(dc, brush);
        if (type == 2 || type == 3) {
            Ellipse(dc, left, top, right, bottom);
        } else if (type == 4 || type == 5) {
            RoundRect(dc, left, top, right, bottom, (right - left) / 4,
                      (bottom - top) / 4);
        } else {
            Rectangle(dc, left, top, right, bottom);
        }
        SelectObject(dc, old_brush);
        SelectObject(dc, old_pen);
        if (operation.fill_style != 1) {
            DeleteObject(brush);
        }
        if (operation.draw_style != 5) {
            DeleteObject(pen);
        }
    } else if (operation.kind == "picture") {
        const auto found = pictures_.find(operation.picture);
        if (found != pictures_.end()) {
            auto* picture = static_cast<IPicture*>(found->second);
            OLE_XSIZE_HIMETRIC width = 0;
            OLE_YSIZE_HIMETRIC height = 0;
            picture->get_Width(&width);
            picture->get_Height(&height);
            const int x = to_x(operation.x1);
            const int y = to_y(operation.y1);
            const int cx = std::max(to_x(operation.x2), 1);
            const int cy = std::max(to_y(operation.y2), 1);
            if (operation.has_source && operation.source_width > 0 &&
                operation.source_height > 0) {
                // The source rectangle is in twips of the natural size.
                const double hm_per_twip = 2540.0 / 1440.0;
                const long sx =
                    static_cast<long>(operation.source_x * hm_per_twip);
                const long sy =
                    static_cast<long>(operation.source_y * hm_per_twip);
                const long sw =
                    static_cast<long>(operation.source_width * hm_per_twip);
                const long sh =
                    static_cast<long>(operation.source_height * hm_per_twip);
                picture->Render(dc, x, y, cx, cy, sx, height - sy, sw, -sh,
                                nullptr);
            } else {
                picture->Render(dc, x, y, cx, cy, 0, height, width, -height,
                                nullptr);
            }
        }
    } else if (operation.kind == "text") {
        const int height = -static_cast<int>(std::lround(
            operation.font.size * static_cast<double>(dpi_y_) / 72.0));
        HFONT font = CreateFontW(
            height, 0, 0, 0, operation.font.bold ? FW_BOLD : FW_NORMAL,
            operation.font.italic ? TRUE : FALSE,
            operation.font.underline ? TRUE : FALSE,
            operation.font.strikethrough ? TRUE : FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS, widen(operation.font.name).c_str());
        HGDIOBJ old_font = SelectObject(dc, font);
        SetTextColor(dc, color);
        SetBkMode(dc, TRANSPARENT);
        const auto text = widen(operation.text);
        TextOutW(dc, to_x(operation.x1), to_y(operation.y1), text.c_str(),
                 static_cast<int>(text.size()));
        SelectObject(dc, old_font);
        DeleteObject(font);
    }
    InvalidateRect(entry->hwnd, nullptr, FALSE);
}

TextMetrics Win32Host::measure_text(const WindowId window,
                                    const std::string& text,
                                    const FontSpec& font) const {
    const auto* entry = find(window);
    HWND owner = entry != nullptr ? entry->hwnd : nullptr;
    HDC dc = GetDC(owner);
    const int height = -static_cast<int>(
        std::lround(font.size * static_cast<double>(dpi_y_) / 72.0));
    HFONT handle =
        CreateFontW(height, 0, 0, 0, font.bold ? FW_BOLD : FW_NORMAL,
                    font.italic ? TRUE : FALSE, font.underline ? TRUE : FALSE,
                    font.strikethrough ? TRUE : FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, widen(font.name).c_str());
    HGDIOBJ old_font = SelectObject(dc, handle);
    double widest = 0.0;
    double total_height = 0.0;
    std::size_t start = 0;
    while (start <= text.size()) {
        auto end = text.find('\n', start);
        if (end == std::string::npos) {
            end = text.size();
        }
        const auto line = widen(text.substr(start, end - start));
        SIZE extent{};
        GetTextExtentPoint32W(dc, line.c_str(), static_cast<int>(line.size()),
                              &extent);
        widest = std::max(widest, static_cast<double>(extent.cx));
        total_height += static_cast<double>(extent.cy);
        if (end == text.size()) {
            break;
        }
        start = end + 1U;
    }
    SelectObject(dc, old_font);
    DeleteObject(handle);
    ReleaseDC(owner, dc);
    return TextMetrics{widest * 1440.0 / static_cast<double>(dpi_x_),
                       total_height * 1440.0 / static_cast<double>(dpi_y_)};
}

void Win32Host::set_focus(const WindowId window) {
    const auto* entry = find(window);
    if (entry != nullptr && entry->hwnd != nullptr) {
        SetFocus(entry->hwnd);
    }
}

WindowId Win32Host::focus() const {
    const HWND focused = GetFocus();
    return focused != nullptr ? Win32Support::id_of(focused) : kNoWindow;
}

void Win32Host::set_timer(const WindowId window, const long interval_ms) {
    auto* entry = find(window);
    if (entry == nullptr) {
        return;
    }
    // Timer events go to the owning form's window procedure.
    const auto* owner = find(entry->parent);
    if (owner == nullptr || owner->hwnd == nullptr || interval_ms <= 0) {
        return;
    }
    entry->timer_interval = interval_ms;
    SetTimer(owner->hwnd, static_cast<UINT_PTR>(window),
             static_cast<UINT>(interval_ms), nullptr);
}

void Win32Host::kill_timer(const WindowId window) {
    auto* entry = find(window);
    if (entry == nullptr) {
        return;
    }
    const auto* owner = find(entry->parent);
    if (owner != nullptr && owner->hwnd != nullptr) {
        KillTimer(owner->hwnd, static_cast<UINT_PTR>(window));
    }
    entry->timer_interval = 0;
}

std::intptr_t Win32Host::surface_dc(const WindowId window) {
    const auto* entry = ensure_surface(window);
    return entry != nullptr ? reinterpret_cast<std::intptr_t>(entry->surface_dc)
                            : 0;
}

void Win32Host::refresh_surfaces() {
    for (const auto& [id, window] : windows_) {
        if (window->surface_dc != nullptr && window->hwnd != nullptr) {
            InvalidateRect(window->hwnd, nullptr, FALSE);
        }
    }
}

void Win32Host::wake_timer(const int slot, const long interval_ms) {
    if (slot < 0 || slot > 1) {
        return;
    }
    UINT_PTR& id = wake_timers_[slot];
    if (interval_ms <= 0) {
        if (id != 0) {
            KillTimer(nullptr, id);
            id = 0;
        }
        return;
    }
    // With no window the system chooses the id; passing it back replaces it.
    id = SetTimer(nullptr, id, static_cast<UINT>(interval_ms), nullptr);
}

std::optional<UiEvent> Win32Host::next_event(const bool wait) {
    while (true) {
        if (!queue_.empty()) {
            UiEvent event = std::move(queue_.front());
            queue_.pop_front();
            return event;
        }
        MSG message;
        if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_TIMER && message.hwnd == nullptr) {
                for (int slot = 0; slot < 2; ++slot) {
                    if (wake_timers_[slot] != 0 &&
                        message.wParam == wake_timers_[slot]) {
                        queue(UiEvent{
                            kNoWindow, slot == 0 ? "WfcWake" : "WfcExit", {}});
                    }
                }
                continue;
            }
            if (message.message == WM_KEYDOWN || message.message == WM_KEYUP ||
                message.message == WM_CHAR) {
                const WindowId target = Win32Support::id_of(message.hwnd);
                if (target != kNoWindow) {
                    const long shift = (GetKeyState(VK_SHIFT) < 0 ? 1L : 0L) |
                                       (GetKeyState(VK_CONTROL) < 0 ? 2L : 0L) |
                                       (GetKeyState(VK_MENU) < 0 ? 4L : 0L);
                    if (message.message == WM_CHAR) {
                        queue(UiEvent{target,
                                      "KeyPress",
                                      {static_cast<long>(message.wParam)}});
                    } else {
                        queue(UiEvent{
                            target,
                            message.message == WM_KEYDOWN ? "KeyDown" : "KeyUp",
                            {static_cast<long>(message.wParam), shift}});
                    }
                }
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
            continue;
        }
        if (!wait || exhausted()) {
            return std::nullopt;
        }
        WaitMessage();
    }
}

void Win32Host::post_event(UiEvent event) {
    if (find(event.window) != nullptr) {
        queue(std::move(event));
    }
}

bool Win32Host::exhausted() const {
    if (!queue_.empty()) {
        return false;
    }
    for (const auto& [id, entry] : windows_) {
        if (entry->is_form) {
            return false;
        }
    }
    return true;
}

#include "win32_comctl.inc"
#include "win32_dialogs.inc"

}  // namespace wfc::ui

#endif
