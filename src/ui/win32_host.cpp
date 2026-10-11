// The Win32 UiHost: real windows, standard controls, menus and a message loop.
// Internal to the WFC runtime; not part of the public API.

#ifdef _WIN32

#include "win32_host.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <cstdint>

namespace wfc::ui {
namespace {

constexpr wchar_t kFormClass[] = L"WfcForm";
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

struct Win32Host::Entry {
    std::string kind;
    WindowId id{kNoWindow};
    WindowId parent{kNoWindow};
    HWND hwnd{nullptr};
    int command{0};  // menu command id
    bool is_form{false};
    bool is_menu{false};
    bool lightweight{false};
    Rect bounds;
    bool visible{false};
    bool enabled{true};
    bool checked{false};
    std::string caption;
    long timer_interval{0};
};

// Window procedure and helpers that need the host's internals.
struct Win32Support {
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
            SetWindowLongPtrW(
                hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        }
        Win32Host* const host = host_of(hwnd);
        if (host == nullptr) {
            return DefWindowProcW(hwnd, message, wparam, lparam);
        }
        const WindowId form = id_of(hwnd);
        switch (message) {
            case WM_CLOSE:
                host->queue(UiEvent{form, "Close", {}});
                return 0;
            case WM_SIZE:
                host->queue(UiEvent{form, "Resize", {}});
                break;
            case WM_TIMER:
                host->queue(
                    UiEvent{static_cast<WindowId>(wparam), "Timer", {}});
                return 0;
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
                if (code == BN_CLICKED && entry->kind != "ListBox" &&
                    entry->kind != "ComboBox") {
                    host->queue(UiEvent{control, "Click", {}});
                } else if (code == BN_DBLCLK) {
                    host->queue(UiEvent{control, "DblClick", {}});
                } else if (code == EN_CHANGE || code == CBN_EDITCHANGE) {
                    host->queue(UiEvent{control, "Change", {}});
                } else if (code == LBN_SELCHANGE || code == CBN_SELCHANGE) {
                    const LRESULT chosen = SendMessageW(
                        reinterpret_cast<HWND>(lparam),
                        entry->kind == "ListBox" ? LB_GETCURSEL : CB_GETCURSEL,
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
            return RegisterClassExW(&window_class) != 0;
        }();
        return registered;
    }
};

Win32Host::Win32Host() {
    if (const HDC context = GetDC(nullptr)) {
        dpi_x_ = GetDeviceCaps(context, LOGPIXELSX);
        dpi_y_ = GetDeviceCaps(context, LOGPIXELSY);
        ReleaseDC(nullptr, context);
        usable_ =
            dpi_x_ > 0 && dpi_y_ > 0 && Win32Support::register_form_class();
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
    if (entry->is_form) {
        entry->hwnd = CreateWindowExW(0, kFormClass, L"", WS_OVERLAPPEDWINDOW,
                                      CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
                                      nullptr, nullptr, instance, this);
    } else if (entry->is_menu) {
        entry->command = next_command_++;
    } else if (parent_handle == nullptr) {
        entry->lightweight = true;
    } else if (kind == "CommandButton") {
        make_child(L"BUTTON", BS_PUSHBUTTON | WS_TABSTOP);
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
    } else if (kind == "ListBox") {
        make_child(L"LISTBOX", LBS_NOTIFY | WS_VSCROLL | WS_TABSTOP,
                   WS_EX_CLIENTEDGE);
    } else if (kind == "ComboBox") {
        make_child(L"COMBOBOX",
                   CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_VSCROLL | WS_TABSTOP);
    } else if (kind == "HScrollBar") {
        make_child(L"SCROLLBAR", SBS_HORZ | WS_TABSTOP);
    } else if (kind == "VScrollBar") {
        make_child(L"SCROLLBAR", SBS_VERT | WS_TABSTOP);
    } else if (kind == "PictureBox") {
        make_child(L"STATIC", SS_OWNERDRAW);
    } else {
        entry->lightweight = true;  // Timer, Shape, Line, Image, ...
    }
    if (entry->hwnd != nullptr) {
        SetPropW(entry->hwnd, kIdProperty,
                 reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(entry->id)));
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
    if (form_entry == nullptr || form_entry->hwnd == nullptr) {
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
    SetWindowPos(entry->hwnd, nullptr, to_x(bounds.left), to_y(bounds.top),
                 std::max(to_x(bounds.width), 1),
                 std::max(to_y(bounds.height), 1),
                 SWP_NOZORDER | SWP_NOACTIVATE);
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
    const bool list = entry->kind == "ListBox";
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
                 entry->kind == "ListBox" ? LB_SETCURSEL : CB_SETCURSEL,
                 static_cast<WPARAM>(index), 0);
}

long Win32Host::selection(const WindowId window) const {
    const auto* entry = find(window);
    if (entry == nullptr || entry->hwnd == nullptr) {
        return -1;
    }
    return static_cast<long>(SendMessageW(
        entry->hwnd, entry->kind == "ListBox" ? LB_GETCURSEL : CB_GETCURSEL, 0,
        0));
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

std::optional<UiEvent> Win32Host::next_event(const bool wait) {
    while (true) {
        if (!queue_.empty()) {
            UiEvent event = std::move(queue_.front());
            queue_.pop_front();
            return event;
        }
        MSG message;
        if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
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

}  // namespace wfc::ui

#endif
