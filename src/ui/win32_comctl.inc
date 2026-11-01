// Win32 rendering of the MSComctlLib controls (MP-0006): each control is a
// system common control rebuilt from a snapshot of the model, and user input
// becomes "ComctlInput" events (an action word and its argument) that the
// interpreter runs like the scripted `ui` actions. Included by win32_host.cpp
// inside namespace wfc::ui. Internal to the WFC runtime.

namespace {

[[nodiscard]] std::wstring wide_text(const std::string& text) {
    return widen(text);
}

// Hands `fresh` to the control (replacing and destroying `old`); returns the
// list now in use.
[[nodiscard]] void* replace_images(HWND hwnd, const UINT message,
                                   const WPARAM kind, void* old, void* fresh) {
    const LRESULT previous =
        SendMessageW(hwnd, message, kind, reinterpret_cast<LPARAM>(fresh));
    if (previous != 0 && reinterpret_cast<void*>(previous) != fresh) {
        ImageList_Destroy(reinterpret_cast<HIMAGELIST>(previous));
    }
    static_cast<void>(old);
    return fresh;
}

}  // namespace

void* Win32Host::build_image_list(const std::vector<long>& ids,
                                  const long width, const long height) {
    if (ids.empty() || width <= 0 || height <= 0) {
        return nullptr;
    }
    HIMAGELIST list = ImageList_Create(
        static_cast<int>(width), static_cast<int>(height),
        ILC_COLOR32 | ILC_MASK, static_cast<int>(ids.size()), 0);
    if (list == nullptr) {
        return nullptr;
    }
    HDC screen = GetDC(nullptr);
    for (const long id : ids) {
        HDC memory = CreateCompatibleDC(screen);
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(info.bmiHeader);
        info.bmiHeader.biWidth = static_cast<LONG>(width);
        info.bmiHeader.biHeight = -static_cast<LONG>(height);
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        HBITMAP bitmap =
            CreateDIBSection(screen, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (bitmap != nullptr) {
            const HGDIOBJ previous = SelectObject(memory, bitmap);
            RECT whole{0, 0, static_cast<LONG>(width),
                       static_cast<LONG>(height)};
            FillRect(memory, &whole,
                     static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
            const auto known = pictures_.find(id);
            if (known != pictures_.end()) {
                auto* const picture = static_cast<IPicture*>(known->second);
                OLE_XSIZE_HIMETRIC picture_width = 0;
                OLE_YSIZE_HIMETRIC picture_height = 0;
                picture->get_Width(&picture_width);
                picture->get_Height(&picture_height);
                picture->Render(memory, 0, 0, static_cast<LONG>(width),
                                static_cast<LONG>(height), 0, picture_height,
                                picture_width, -picture_height, nullptr);
            }
            SelectObject(memory, previous);
            ImageList_Add(list, bitmap, nullptr);
            DeleteObject(bitmap);
        }
        DeleteDC(memory);
    }
    ReleaseDC(nullptr, screen);
    return list;
}

static void month_day_states(const std::vector<double>& bold,
                             const SYSTEMTIME& first, int months,
                             MONTHDAYSTATE* states);
static std::string month_range_text(const SYSTEMTIME& first, int months);

bool Win32Host::month_hit(const WindowId window, const long x, const long y,
                          long& area, double& date) {
    const auto* entry = find(window);
    if (entry == nullptr || entry->hwnd == nullptr) {
        return false;
    }
    MCHITTESTINFO info{};
    // The first four members: the version of the control may predate rc.
    info.cbSize = offsetof(MCHITTESTINFO, rc);
    info.pt = POINT{static_cast<LONG>(x), static_cast<LONG>(y)};
    SendMessageW(entry->hwnd, MCM_HITTEST, 0, reinterpret_cast<LPARAM>(&info));
    // The values of MonthViewHitTestAreas (mvwCalendarBack is 0).
    switch (info.uHit) {
        case MCHT_CALENDARBK:
            area = 0;
            break;
        case MCHT_CALENDARDATE:
            area = 1;
            break;
        case MCHT_CALENDARDATENEXT:
            area = 2;
            break;
        case MCHT_CALENDARDATEPREV:
            area = 3;
            break;
        case MCHT_CALENDARDAY:
            area = 4;
            break;
        case MCHT_CALENDARWEEKNUM:
            area = 5;
            break;
        case MCHT_TITLEBK:
            area = 7;
            break;
        case MCHT_TITLEBTNNEXT:
            area = 8;
            break;
        case MCHT_TITLEBTNPREV:
            area = 9;
            break;
        case MCHT_TITLEMONTH:
            area = 10;
            break;
        case MCHT_TITLEYEAR:
            area = 11;
            break;
        case MCHT_TODAYLINK:
            area = 12;
            break;
        default:
            area = 6;  // mvwNoWhere
            break;
    }
    double serial = 0;
    if (SystemTimeToVariantTime(&info.st, &serial) != 0) {
        date = serial;
    }
    return true;
}

bool Win32Host::month_size(const WindowId window, const long rows,
                           const long columns, double& width, double& height) {
    const auto* entry = find(window);
    if (entry == nullptr || entry->hwnd == nullptr) {
        return false;
    }
    RECT one{};
    if (SendMessageW(entry->hwnd, MCM_GETMINREQRECT, 0,
                     reinterpret_cast<LPARAM>(&one)) == 0) {
        return false;
    }
    width = static_cast<double>(one.right - one.left) *
            static_cast<double>(std::max(1L, columns)) * 1440.0 /
            static_cast<double>(dpi_x_);
    height = static_cast<double>(one.bottom - one.top) *
             static_cast<double>(std::max(1L, rows)) * 1440.0 /
             static_cast<double>(dpi_y_);
    return true;
}

void Win32Host::animate(const WindowId window, const std::string& action,
                        const std::string& path, const long repeat,
                        const long first, const long last) {
    const auto* entry = find(window);
    if (entry == nullptr || entry->hwnd == nullptr) {
        return;
    }
    HWND const hwnd = entry->hwnd;
    if (action == "open") {
        const std::wstring file = wide_text(path);
        SendMessageW(hwnd, ACM_OPENW, 0,
                     reinterpret_cast<LPARAM>(file.c_str()));
    } else if (action == "play") {
        SendMessageW(
            hwnd, ACM_PLAY, static_cast<WPARAM>(repeat),
            MAKELPARAM(static_cast<WORD>(first), static_cast<WORD>(last)));
    } else if (action == "stop") {
        SendMessageW(hwnd, ACM_STOP, 0, 0);
    } else if (action == "close") {
        SendMessageW(hwnd, ACM_OPENW, 0, 0);
    }
}

void Win32Host::sync_common(const WindowId window,
                            const CommonSnapshot& snapshot) {
    auto* entry = find(window);
    if (entry == nullptr || entry->hwnd == nullptr) {
        return;
    }
    HWND const hwnd = entry->hwnd;
    const std::string& kind = snapshot.kind;
    entry->applying = true;
    if (kind == "progressbar") {
        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        style = snapshot.smooth ? (style | PBS_SMOOTH) : (style & ~PBS_SMOOTH);
        style = snapshot.vertical ? (style | PBS_VERTICAL)
                                  : (style & ~PBS_VERTICAL);
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);
        SendMessageW(hwnd, PBM_SETRANGE32, snapshot.min, snapshot.max);
        SendMessageW(hwnd, PBM_SETPOS, snapshot.value, 0);
    } else if (kind == "slider") {
        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        style &= ~static_cast<LONG_PTR>(TBS_VERT | TBS_NOTICKS | TBS_BOTH |
                                        TBS_TOP | TBS_ENABLESELRANGE);
        if (snapshot.vertical) {
            style |= TBS_VERT;
        }
        if (snapshot.tick_style == 3) {
            style |= TBS_NOTICKS;
        } else if (snapshot.tick_style == 2) {
            style |= TBS_BOTH;
        } else if (snapshot.tick_style == 1) {
            style |= TBS_TOP;
        }
        if (snapshot.select_range) {
            style |= TBS_ENABLESELRANGE;
        }
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);
        SendMessageW(hwnd, TBM_SETRANGEMIN, FALSE, snapshot.min);
        SendMessageW(hwnd, TBM_SETRANGEMAX, FALSE, snapshot.max);
        SendMessageW(hwnd, TBM_SETPOS, TRUE, snapshot.value);
        SendMessageW(hwnd, TBM_SETLINESIZE, 0,
                     std::max<long>(1, snapshot.small_change));
        SendMessageW(hwnd, TBM_SETPAGESIZE, 0,
                     std::max<long>(1, snapshot.large_change));
        SendMessageW(hwnd, TBM_SETTICFREQ,
                     std::max<long>(1, snapshot.tick_frequency), 0);
        if (snapshot.select_range) {
            SendMessageW(hwnd, TBM_SETSEL, TRUE,
                         MAKELPARAM(snapshot.sel_start,
                                    snapshot.sel_start + snapshot.sel_length));
        }
    } else if (kind == "animation") {
        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        style &= ~static_cast<LONG_PTR>(ACS_CENTER | ACS_TRANSPARENT);
        if (snapshot.center) {
            style |= ACS_CENTER;
        }
        if (snapshot.transparent) {
            style |= ACS_TRANSPARENT;
        }
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);
    } else if (kind == "dtpicker") {
        // A changed callback answer needs the control to format again.
        bool answers_changed = entry->callbacks.size() != snapshot.items.size();
        for (std::size_t at = 0; !answers_changed && at < snapshot.items.size();
             ++at) {
            answers_changed =
                entry->callbacks[at].key != snapshot.items[at].key ||
                entry->callbacks[at].width != snapshot.items[at].width;
        }
        entry->callbacks = snapshot.items;
        if (answers_changed) {
            SendMessageW(hwnd, DTM_SETFORMATW, 0,
                         reinterpret_cast<LPARAM>(L" "));
        }
        const auto to_system = [](const double serial) {
            SYSTEMTIME time{};
            VariantTimeToSystemTime(serial, &time);
            return time;
        };
        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        style &= ~static_cast<LONG_PTR>(DTS_LONGDATEFORMAT |
                                        DTS_SHORTDATEFORMAT | DTS_TIMEFORMAT);
        // DTS_LONGDATEFORMAT and DTS_SHORTDATEFORMAT are separate bits.
        style |= snapshot.style == 1   ? DTS_SHORTDATEFORMAT
                 : snapshot.style == 2 ? DTS_TIMEFORMAT
                                       : DTS_LONGDATEFORMAT;
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);
        if (snapshot.style == 3) {
            // Each run of X is a callback field: ask the program (once)
            // how wide it is.
            for (std::size_t at = 0; at < snapshot.text.size();) {
                if (snapshot.text[at] != 'X') {
                    ++at;
                    continue;
                }
                const std::size_t end =
                    snapshot.text.find_first_not_of('X', at);
                const std::string run = snapshot.text.substr(at, end - at);
                if (entry->callback_sized.insert(run).second) {
                    queue(UiEvent{window, "ComctlInput", {"formatsize", run}});
                }
                SYSTEMTIME now = to_system(snapshot.date_value);
                char when[32];
                std::snprintf(when, sizeof(when),
                              "%04d-%02d-%02d %02d:%02d:%02d", now.wYear,
                              now.wMonth, now.wDay, now.wHour, now.wMinute,
                              now.wSecond);
                const std::string asked = run + " " + when;
                if (asked != entry->callback_asked) {
                    entry->callback_asked = asked;
                    queue(UiEvent{window, "ComctlInput", {"format", asked}});
                }
                at = end == std::string::npos ? snapshot.text.size() : end;
            }
            std::wstring format = wide_text(snapshot.text);
            SendMessageW(hwnd, DTM_SETFORMATW, 0,
                         reinterpret_cast<LPARAM>(format.c_str()));
        } else {
            SendMessageW(hwnd, DTM_SETFORMATW, 0, 0);
        }
        SYSTEMTIME limits[2] = {to_system(snapshot.date_min),
                                to_system(snapshot.date_max)};
        SendMessageW(hwnd, DTM_SETRANGE, GDTR_MIN | GDTR_MAX,
                     reinterpret_cast<LPARAM>(limits));
        if (snapshot.select_range) {
            SendMessageW(hwnd, DTM_SETSYSTEMTIME, GDT_NONE, 0);
        } else {
            SYSTEMTIME current = to_system(snapshot.date_value);
            SendMessageW(hwnd, DTM_SETSYSTEMTIME, GDT_VALID,
                         reinterpret_cast<LPARAM>(&current));
        }
        if (answers_changed) {
            InvalidateRect(hwnd, nullptr, TRUE);
            UpdateWindow(hwnd);
        }
    } else if (kind == "monthview") {
        const auto to_system = [](const double serial) {
            SYSTEMTIME time{};
            VariantTimeToSystemTime(serial, &time);
            return time;
        };
        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        style &= ~static_cast<LONG_PTR>(MCS_NOTODAY | MCS_NOTODAYCIRCLE |
                                        MCS_WEEKNUMBERS);
        if (!snapshot.show_today) {
            style |= MCS_NOTODAY | MCS_NOTODAYCIRCLE;
        }
        if (snapshot.week_numbers) {
            style |= MCS_WEEKNUMBERS;
        }
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);
        SYSTEMTIME limits[2] = {to_system(snapshot.date_min),
                                to_system(snapshot.date_max)};
        SendMessageW(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX,
                     reinterpret_cast<LPARAM>(limits));
        SendMessageW(
            hwnd, MCM_SETMAXSELCOUNT,
            static_cast<WPARAM>(std::max<long>(1, snapshot.max_select)), 0);
        // MCM_SETFIRSTDAYOFWEEK counts from Monday (0) to Sunday (6).
        SendMessageW(hwnd, MCM_SETFIRSTDAYOFWEEK, 0,
                     (snapshot.first_day + 5) % 7);
        SYSTEMTIME selection[2] = {to_system(snapshot.date_start),
                                   to_system(snapshot.date_end)};
        SendMessageW(hwnd, MCM_SETSELRANGE, 0,
                     reinterpret_cast<LPARAM>(selection));
        entry->bold_days = snapshot.bold_days;
        SYSTEMTIME shown[2] = {};
        const int months =
            static_cast<int>(SendMessageW(hwnd, MCM_GETMONTHRANGE, GMR_DAYSTATE,
                                          reinterpret_cast<LPARAM>(shown)));
        if (months > 0 && months <= 12) {
            MONTHDAYSTATE states[12] = {};
            month_day_states(entry->bold_days, shown[0], months, states);
            SendMessageW(hwnd, MCM_SETDAYSTATE, static_cast<WPARAM>(months),
                         reinterpret_cast<LPARAM>(states));
            // Ask the program (GetDayBold) once for each stretch shown.
            const std::string asked = month_range_text(shown[0], months);
            if (asked != entry->bold_asked) {
                entry->bold_asked = asked;
                queue(UiEvent{window, "ComctlInput", {"getdaybold", asked}});
            }
        }
    } else if (kind == "updown") {
        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        style &= ~static_cast<LONG_PTR>(UDS_HORZ | UDS_WRAP);
        if (!snapshot.vertical) {
            style |= UDS_HORZ;
        }
        if (snapshot.wrap) {
            style |= UDS_WRAP;
        }
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);
        SendMessageW(hwnd, UDM_SETRANGE32, snapshot.min, snapshot.max);
        SendMessageW(hwnd, UDM_SETPOS32, 0, snapshot.value);
        UDACCEL step{};
        step.nSec = 0;
        step.nInc = static_cast<UINT>(std::max<long>(1, snapshot.small_change));
        SendMessageW(hwnd, UDM_SETACCEL, 1, reinterpret_cast<LPARAM>(&step));
    } else if (kind == "tabstrip") {
        SendMessageW(hwnd, TCM_DELETEALLITEMS, 0, 0);
        entry->images = replace_images(
            hwnd, TCM_SETIMAGELIST, 0, entry->images,
            build_image_list(snapshot.images, snapshot.image_width,
                             snapshot.image_height));
        long at = 0;
        for (const auto& item : snapshot.items) {
            std::wstring text = wide_text(item.text);
            TCITEMW tab{};
            tab.mask = TCIF_TEXT | TCIF_IMAGE;
            tab.iImage =
                item.image >= 0 ? static_cast<int>(item.image) : I_IMAGENONE;
            tab.pszText = text.data();
            SendMessageW(hwnd, TCM_INSERTITEMW, static_cast<WPARAM>(at),
                         reinterpret_cast<LPARAM>(&tab));
            ++at;
        }
        if (snapshot.selected >= 0) {
            SendMessageW(hwnd, TCM_SETCURSEL,
                         static_cast<WPARAM>(snapshot.selected), 0);
        }
    } else if (kind == "statusbar") {
        SendMessageW(hwnd, SB_SIMPLE, snapshot.style == 1 ? TRUE : FALSE, 0);
        if (snapshot.style == 1) {
            std::wstring text = wide_text(snapshot.simple_text);
            SendMessageW(hwnd, SB_SETTEXTW, SB_SIMPLEID,
                         reinterpret_cast<LPARAM>(text.c_str()));
        } else {
            std::vector<int> edges;
            int right = 0;
            RECT area;
            GetClientRect(hwnd, &area);
            for (std::size_t i = 0; i < snapshot.items.size(); ++i) {
                const long width = snapshot.items[i].width;
                right +=
                    width > 0 ? static_cast<int>(width * dpi_x_ / 1440) : 100;
                edges.push_back(i + 1U == snapshot.items.size() ? -1 : right);
            }
            if (edges.empty()) {
                edges.push_back(-1);
            }
            SendMessageW(hwnd, SB_SETPARTS, edges.size(),
                         reinterpret_cast<LPARAM>(edges.data()));
            for (std::size_t i = 0; i < snapshot.items.size(); ++i) {
                std::wstring text = wide_text(snapshot.items[i].text);
                SendMessageW(hwnd, SB_SETTEXTW, i,
                             reinterpret_cast<LPARAM>(text.c_str()));
            }
        }
    } else if (kind == "toolbar") {
        while (SendMessageW(hwnd, TB_BUTTONCOUNT, 0, 0) > 0) {
            SendMessageW(hwnd, TB_DELETEBUTTON, 0, 0);
        }
        SendMessageW(hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        entry->images = replace_images(
            hwnd, TB_SETIMAGELIST, 0, entry->images,
            build_image_list(snapshot.images, snapshot.image_width,
                             snapshot.image_height));
        long at = 0;
        for (const auto& item : snapshot.items) {
            std::wstring text = wide_text(item.text) + L'\0';
            const INT_PTR text_index = SendMessageW(
                hwnd, TB_ADDSTRINGW, 0, reinterpret_cast<LPARAM>(text.c_str()));
            TBBUTTON button{};
            button.idCommand = static_cast<int>(at) + 1;
            button.iBitmap =
                item.image >= 0 ? static_cast<int>(item.image) : I_IMAGENONE;
            button.iString = static_cast<INT_PTR>(text_index);
            button.fsState =
                static_cast<BYTE>((item.enabled ? TBSTATE_ENABLED : 0) |
                                  (item.value == 1 ? TBSTATE_CHECKED : 0) |
                                  (item.visible ? 0 : TBSTATE_HIDDEN));
            switch (item.style) {
                case 1:
                    button.fsStyle = BTNS_CHECK | BTNS_AUTOSIZE;
                    break;
                case 2:
                    button.fsStyle = BTNS_CHECKGROUP | BTNS_AUTOSIZE;
                    break;
                case 3:
                    button.fsStyle = BTNS_SEP;
                    break;
                case 4:
                    button.fsStyle = BTNS_SEP;
                    button.iBitmap =
                        std::max<long>(8, item.width * dpi_x_ / 1440);
                    break;
                case 5:
                    button.fsStyle = BTNS_DROPDOWN | BTNS_AUTOSIZE;
                    break;
                default:
                    button.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
                    break;
            }
            button.fsStyle = static_cast<BYTE>(button.fsStyle | BTNS_SHOWTEXT);
            SendMessageW(hwnd, TB_INSERTBUTTONW, static_cast<WPARAM>(at),
                         reinterpret_cast<LPARAM>(&button));
            ++at;
        }
        SendMessageW(hwnd, TB_SETEXTENDEDSTYLE, 0,
                     TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
        SendMessageW(hwnd, TB_AUTOSIZE, 0, 0);
    } else if (kind == "treeview") {
        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        style |=
            TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS;
        style = snapshot.checkboxes ? (style | TVS_CHECKBOXES)
                                    : (style & ~TVS_CHECKBOXES);
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);
        SendMessageW(hwnd, TVM_DELETEITEM, 0,
                     reinterpret_cast<LPARAM>(TVI_ROOT));
        entry->images = replace_images(
            hwnd, TVM_SETIMAGELIST, TVSIL_NORMAL, entry->images,
            build_image_list(snapshot.images, snapshot.image_width,
                             snapshot.image_height));
        std::vector<HTREEITEM> handles;
        for (const auto& item : snapshot.items) {
            std::wstring text = wide_text(item.text);
            TVINSERTSTRUCTW insert{};
            insert.hParent =
                item.parent < 0
                    ? TVI_ROOT
                    : handles[static_cast<std::size_t>(item.parent)];
            insert.hInsertAfter = TVI_LAST;
            insert.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE |
                               TVIF_IMAGE | TVIF_SELECTEDIMAGE;
            insert.item.iImage =
                item.image >= 0 ? static_cast<int>(item.image) : I_IMAGENONE;
            insert.item.iSelectedImage =
                item.image2 >= 0
                    ? static_cast<int>(item.image2)
                    : (item.image >= 0 ? static_cast<int>(item.image)
                                       : I_IMAGENONE);
            insert.item.pszText = text.data();
            insert.item.lParam = item.index;
            insert.item.stateMask = TVIS_BOLD;
            insert.item.state = item.bold ? TVIS_BOLD : 0;
            handles.push_back(reinterpret_cast<HTREEITEM>(SendMessageW(
                hwnd, TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&insert))));
        }
        for (std::size_t i = 0; i < snapshot.items.size(); ++i) {
            if (snapshot.items[i].checked && snapshot.checkboxes) {
                TVITEMW check{};
                check.mask = TVIF_STATE | TVIF_HANDLE;
                check.hItem = handles[i];
                check.stateMask = TVIS_STATEIMAGEMASK;
                check.state = INDEXTOSTATEIMAGEMASK(2);
                SendMessageW(hwnd, TVM_SETITEMW, 0,
                             reinterpret_cast<LPARAM>(&check));
            }
            if (snapshot.items[i].expanded) {
                SendMessageW(hwnd, TVM_EXPAND, TVE_EXPAND,
                             reinterpret_cast<LPARAM>(handles[i]));
            }
        }
        if (snapshot.selected >= 0) {
            SendMessageW(
                hwnd, TVM_SELECTITEM, TVGN_CARET,
                reinterpret_cast<LPARAM>(
                    handles[static_cast<std::size_t>(snapshot.selected)]));
        }
    } else if (kind == "listview") {
        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        style &= ~static_cast<LONG_PTR>(LVS_TYPEMASK);
        style |= snapshot.view == 0   ? LVS_ICON
                 : snapshot.view == 1 ? LVS_SMALLICON
                 : snapshot.view == 2 ? LVS_LIST
                                      : LVS_REPORT;
        style |= LVS_SHOWSELALWAYS;
        style = snapshot.hide_headers ? (style | LVS_NOCOLUMNHEADER)
                                      : (style & ~LVS_NOCOLUMNHEADER);
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);
        DWORD extended = 0;
        extended |= snapshot.checkboxes ? LVS_EX_CHECKBOXES : 0;
        extended |= snapshot.gridlines ? LVS_EX_GRIDLINES : 0;
        extended |= snapshot.full_row ? LVS_EX_FULLROWSELECT : 0;
        SendMessageW(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, ~0U, extended);
        SendMessageW(hwnd, LVM_DELETEALLITEMS, 0, 0);
        while (SendMessageW(hwnd, LVM_DELETECOLUMN, 0, 0) != FALSE) {
        }
        entry->images = replace_images(
            hwnd, LVM_SETIMAGELIST, LVSIL_NORMAL, entry->images,
            build_image_list(snapshot.images, snapshot.image_width,
                             snapshot.image_height));
        entry->small_images = replace_images(
            hwnd, LVM_SETIMAGELIST, LVSIL_SMALL, entry->small_images,
            build_image_list(snapshot.small_images, snapshot.small_width,
                             snapshot.small_height));
        for (std::size_t i = 0; i < snapshot.columns.size(); ++i) {
            std::wstring text = wide_text(snapshot.columns[i].text);
            LVCOLUMNW column{};
            column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;
            column.pszText = text.data();
            column.cx =
                static_cast<int>(snapshot.columns[i].width * dpi_x_ / 1440);
            column.fmt = snapshot.columns[i].alignment == 1   ? LVCFMT_RIGHT
                         : snapshot.columns[i].alignment == 2 ? LVCFMT_CENTER
                                                              : LVCFMT_LEFT;
            column.iSubItem = static_cast<int>(i);
            SendMessageW(hwnd, LVM_INSERTCOLUMNW, i,
                         reinterpret_cast<LPARAM>(&column));
        }
        for (std::size_t i = 0; i < snapshot.items.size(); ++i) {
            std::wstring text = wide_text(snapshot.items[i].text);
            LVITEMW row{};
            row.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
            const long shown = snapshot.view == 0 ? snapshot.items[i].image
                                                  : snapshot.items[i].image2;
            row.iImage = shown >= 0 ? static_cast<int>(shown) : I_IMAGENONE;
            row.iItem = static_cast<int>(i);
            row.pszText = text.data();
            row.lParam = snapshot.items[i].index;
            SendMessageW(hwnd, LVM_INSERTITEMW, 0,
                         reinterpret_cast<LPARAM>(&row));
            for (std::size_t c = 0; c < snapshot.items[i].cells.size(); ++c) {
                std::wstring cell = wide_text(snapshot.items[i].cells[c]);
                LVITEMW sub{};
                sub.iSubItem = static_cast<int>(c) + 1;
                sub.pszText = cell.data();
                SendMessageW(hwnd, LVM_SETITEMTEXTW, i,
                             reinterpret_cast<LPARAM>(&sub));
            }
            if (snapshot.items[i].checked && snapshot.checkboxes) {
                LVITEMW state{};
                state.stateMask = LVIS_STATEIMAGEMASK;
                state.state = INDEXTOSTATEIMAGEMASK(2);
                SendMessageW(hwnd, LVM_SETITEMSTATE, i,
                             reinterpret_cast<LPARAM>(&state));
            }
        }
        if (snapshot.selected >= 0) {
            LVITEMW state{};
            state.stateMask = LVIS_SELECTED;
            state.state = LVIS_SELECTED;
            SendMessageW(hwnd, LVM_SETITEMSTATE,
                         static_cast<WPARAM>(snapshot.selected),
                         reinterpret_cast<LPARAM>(&state));
        }
    } else if (kind == "imagecombo") {
        SendMessageW(hwnd, CB_RESETCONTENT, 0, 0);
        entry->images = replace_images(
            hwnd, CBEM_SETIMAGELIST, 0, entry->images,
            build_image_list(snapshot.images, snapshot.image_width,
                             snapshot.image_height));
        for (std::size_t i = 0; i < snapshot.items.size(); ++i) {
            std::wstring text = wide_text(snapshot.items[i].text);
            COMBOBOXEXITEMW item{};
            item.mask =
                CBEIF_TEXT | CBEIF_INDENT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
            item.iImage = snapshot.items[i].image >= 0
                              ? static_cast<int>(snapshot.items[i].image)
                              : I_IMAGENONE;
            item.iSelectedImage =
                snapshot.items[i].image2 >= 0
                    ? static_cast<int>(snapshot.items[i].image2)
                    : (snapshot.items[i].image >= 0
                           ? static_cast<int>(snapshot.items[i].image)
                           : I_IMAGENONE);
            item.iItem = -1;
            item.pszText = text.data();
            item.iIndent = static_cast<int>(snapshot.items[i].indent);
            SendMessageW(hwnd, CBEM_INSERTITEMW, 0,
                         reinterpret_cast<LPARAM>(&item));
        }
        SendMessageW(hwnd, CB_SETCURSEL, static_cast<WPARAM>(snapshot.selected),
                     0);
        if (snapshot.selected < 0) {
            std::wstring text = wide_text(snapshot.text);
            SetWindowTextW(hwnd, text.c_str());
        }
        SendMessageW(hwnd, EM_SETREADONLY, snapshot.locked ? TRUE : FALSE, 0);
    }
    entry->applying = false;
}

// A notification of a common control: becomes a ComctlInput event.
// Bit n of a month's state is day n + 1 of it; `first` is the month the
// calendar starts at (any day of it).
static void month_day_states(const std::vector<double>& bold,
                             const SYSTEMTIME& first, const int months,
                             MONTHDAYSTATE* states) {
    for (int at = 0; at < months; ++at) {
        states[at] = 0;
        SYSTEMTIME month = first;
        const int index = month.wMonth - 1 + at;
        month.wYear = static_cast<WORD>(month.wYear + index / 12);
        month.wMonth = static_cast<WORD>(index % 12 + 1);
        for (int day = 1; day <= 31; ++day) {
            month.wDay = static_cast<WORD>(day);
            double serial = 0;
            if (SystemTimeToVariantTime(&month, &serial) == 0) {
                break;
            }
            if (std::find(bold.begin(), bold.end(), std::floor(serial)) !=
                bold.end()) {
                states[at] |= 1U << (day - 1);
            }
        }
    }
}

// `yyyy-mm-dd days`: the first day of the month `first` is in and the days of
// the `months` months from it.
static std::string month_range_text(const SYSTEMTIME& first, const int months) {
    static const int lengths[12] = {31, 28, 31, 30, 31, 30,
                                    31, 31, 30, 31, 30, 31};
    long days = 0;
    for (int at = 0; at < months; ++at) {
        const int index = first.wMonth - 1 + at;
        const int year = first.wYear + index / 12;
        const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
        days += lengths[index % 12] + (index % 12 == 1 && leap ? 1 : 0);
    }
    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-01 %ld", first.wYear,
                  first.wMonth, days);
    return buffer;
}

bool Win32Host::common_notify(const void* raw_header,
                              const std::intptr_t lparam) {
    const NMHDR& header = *static_cast<const NMHDR*>(raw_header);
    const WindowId control = Win32Support::id_of(header.hwndFrom);
    auto* entry = find(control);
    // The callback fields of a date picker are answered while a snapshot is
    // applied too: the control formats then.
    const bool callback_code =
        header.code == DTN_FORMATW || header.code == DTN_FORMATQUERYW;
    if (entry == nullptr || (entry->applying && !callback_code)) {
        return false;
    }
    const auto post = [&](const std::string& action, const std::string& rest) {
        if (entry->applying) {
            return;
        }
        queue(UiEvent{control, "ComctlInput", {action, rest}});
    };
    const std::string& kind = entry->kind;
    if (kind.rfind("DTPicker", 0) == 0) {
        const auto text = [](const SYSTEMTIME& time) {
            char buffer[48];
            std::snprintf(buffer, sizeof(buffer),
                          "%04d-%02d-%02d %02d:%02d:%02d", time.wYear,
                          time.wMonth, time.wDay, time.wHour, time.wMinute,
                          time.wSecond);
            return std::string(buffer);
        };
        const auto callback = [&](const std::wstring& field) {
            for (const auto& item : entry->callbacks) {
                if (item.text == narrow(field)) {
                    return &item;
                }
            }
            return static_cast<const CommonItem*>(nullptr);
        };
        if (header.code == DTN_FORMATW) {
            // A callback field (a run of X in the custom format): the
            // program's Format event gives its text.
            auto* format = reinterpret_cast<NMDATETIMEFORMATW*>(lparam);
            const std::string field = narrow(format->pszFormat);
            const std::string asked = field + " " + text(format->st);
            if (const auto* item = callback(format->pszFormat)) {
                const std::wstring shown = wide_text(item->key);
                wcsncpy_s(format->szDisplay, shown.c_str(), _TRUNCATE);
            }
            format->pszDisplay = format->szDisplay;
            if (!entry->applying && asked != entry->callback_asked) {
                entry->callback_asked = asked;
                post("format", asked);
            }
            return true;
        }
        if (header.code == DTN_FORMATQUERYW) {
            auto* query = reinterpret_cast<NMDATETIMEFORMATQUERYW*>(lparam);
            if (const auto* item = callback(query->pszFormat)) {
                query->szMax.cx = item->width;
            }
            post("formatsize", narrow(query->pszFormat));
            return true;
        }
        if (header.code == DTN_WMKEYDOWNW) {
            auto* key = reinterpret_cast<NMDATETIMEWMKEYDOWNW*>(lparam);
            const int shift = (GetKeyState(VK_SHIFT) < 0 ? 1 : 0) |
                              (GetKeyState(VK_CONTROL) < 0 ? 2 : 0) |
                              (GetKeyState(VK_MENU) < 0 ? 4 : 0);
            post("keydown", narrow(key->pszFormat) + " " +
                                std::to_string(key->nVirtKey) + " " +
                                std::to_string(shift) + " " + text(key->st));
            return true;
        }
        if (header.code == DTN_DATETIMECHANGE) {
            const auto* change =
                reinterpret_cast<const NMDATETIMECHANGE*>(lparam);
            if (change->dwFlags == GDT_NONE) {
                post("check", "off");
            } else {
                post("value", text(change->st));
            }
            return true;
        }
        if (header.code == DTN_DROPDOWN) {
            post("dropdown", "");
            return true;
        }
        if (header.code == DTN_CLOSEUP) {
            post("closeup", "");
            return true;
        }
    }
    if (kind == "MonthView" && header.code == MCN_GETDAYSTATE) {
        // The calendar asks which days are bold: answer from the last
        // snapshot, and let the program's GetDayBold event refresh it.
        auto* request = reinterpret_cast<NMDAYSTATE*>(lparam);
        month_day_states(entry->bold_days, request->stStart, request->cDayState,
                         request->prgDayState);
        post("getdaybold",
             month_range_text(request->stStart, request->cDayState));
        return true;
    }
    if (kind == "MonthView" && header.code == MCN_SELECT) {
        // The user chose a date or a range: `start end` as yyyy-mm-dd.
        const auto* chosen = reinterpret_cast<const NMSELCHANGE*>(lparam);
        const auto text = [](const SYSTEMTIME& time) {
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", time.wYear,
                          time.wMonth, time.wDay);
            return std::string(buffer);
        };
        post("select", text(chosen->stSelStart) + " " + text(chosen->stSelEnd));
        return true;
    }
    if (kind == "TabStrip" && header.code == TCN_SELCHANGE) {
        post("select",
             std::to_string(SendMessageW(header.hwndFrom, TCM_GETCURSEL, 0, 0) +
                            1));
        return true;
    }
    if (kind == "TreeView") {
        const auto* tree = reinterpret_cast<const NMTREEVIEWW*>(lparam);
        if (header.code == TVN_SELCHANGEDW) {
            post("click", std::to_string(tree->itemNew.lParam));
            return true;
        }
        if (header.code == TVN_ITEMEXPANDEDW) {
            post((tree->action & TVE_EXPAND) != 0 ? "expand" : "collapse",
                 std::to_string(tree->itemNew.lParam));
            return true;
        }
        if (header.code == NM_CLICK) {
            TVHITTESTINFO hit{};
            DWORD position = GetMessagePos();
            hit.pt = {static_cast<short>(LOWORD(position)),
                      static_cast<short>(HIWORD(position))};
            ScreenToClient(header.hwndFrom, &hit.pt);
            if (HTREEITEM item = reinterpret_cast<HTREEITEM>(
                    SendMessageW(header.hwndFrom, TVM_HITTEST, 0,
                                 reinterpret_cast<LPARAM>(&hit)));
                item != nullptr && (hit.flags & TVHT_ONITEMSTATEICON) != 0) {
                TVITEMW data{};
                data.mask = TVIF_PARAM | TVIF_HANDLE;
                data.hItem = item;
                SendMessageW(header.hwndFrom, TVM_GETITEMW, 0,
                             reinterpret_cast<LPARAM>(&data));
                post("check", std::to_string(data.lParam));
                return true;
            }
        }
    }
    if (kind == "ListView") {
        const auto* list = reinterpret_cast<const NMLISTVIEW*>(lparam);
        if (header.code == LVN_ITEMCHANGED &&
            (list->uChanged & LVIF_STATE) != 0 &&
            (list->uNewState & LVIS_SELECTED) != 0 &&
            (list->uOldState & LVIS_SELECTED) == 0) {
            post("click", std::to_string(list->lParam));
            return true;
        }
        if (header.code == LVN_ITEMCHANGED &&
            (list->uChanged & LVIF_STATE) != 0 &&
            ((list->uNewState ^ list->uOldState) & LVIS_STATEIMAGEMASK) != 0 &&
            list->uOldState != 0) {
            post("check", std::to_string(list->lParam));
            return true;
        }
        if (header.code == LVN_COLUMNCLICK) {
            post("columnclick", std::to_string(list->iSubItem + 1));
            return true;
        }
    }
    if (kind == "StatusBar" &&
        (header.code == NM_CLICK || header.code == NM_DBLCLK)) {
        const auto* mouse = reinterpret_cast<const NMMOUSE*>(lparam);
        post(header.code == NM_CLICK ? "panel" : "paneldbl",
             std::to_string(mouse->dwItemSpec + 1));
        return true;
    }
    if (kind == "Toolbar" && header.code == TBN_DROPDOWN) {
        const auto* drop = reinterpret_cast<const NMTOOLBARW*>(lparam);
        post("dropdown", std::to_string(drop->iItem));
        return true;
    }
    return false;
}
