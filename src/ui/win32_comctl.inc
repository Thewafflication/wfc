// Win32 rendering of the MSComctlLib controls (MP-0006): each control is a
// system common control rebuilt from a snapshot of the model, and user input
// becomes "ComctlInput" events (an action word and its argument) that the
// interpreter runs like the scripted `ui` actions. Included by win32_host.cpp
// inside namespace wfc::ui. Internal to the WFC runtime.

namespace {

[[nodiscard]] std::wstring wide_text(const std::string& text) {
    return widen(text);
}

}  // namespace

void Win32Host::sync_common(const WindowId window, const CommonSnapshot& snapshot) {
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
        style = snapshot.vertical ? (style | PBS_VERTICAL) : (style & ~PBS_VERTICAL);
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);
        SendMessageW(hwnd, PBM_SETRANGE32, snapshot.min, snapshot.max);
        SendMessageW(hwnd, PBM_SETPOS, snapshot.value, 0);
    } else if (kind == "slider") {
        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        style &= ~static_cast<LONG_PTR>(TBS_VERT | TBS_NOTICKS | TBS_BOTH | TBS_TOP |
                                        TBS_ENABLESELRANGE);
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
        SendMessageW(hwnd, TBM_SETLINESIZE, 0, std::max<long>(1, snapshot.small_change));
        SendMessageW(hwnd, TBM_SETPAGESIZE, 0, std::max<long>(1, snapshot.large_change));
        SendMessageW(hwnd, TBM_SETTICFREQ, std::max<long>(1, snapshot.tick_frequency), 0);
        if (snapshot.select_range) {
            SendMessageW(hwnd, TBM_SETSEL, TRUE,
                         MAKELPARAM(snapshot.sel_start,
                                    snapshot.sel_start + snapshot.sel_length));
        }
    } else if (kind == "tabstrip") {
        SendMessageW(hwnd, TCM_DELETEALLITEMS, 0, 0);
        long at = 0;
        for (const auto& item : snapshot.items) {
            std::wstring text = wide_text(item.text);
            TCITEMW tab{};
            tab.mask = TCIF_TEXT;
            tab.pszText = text.data();
            SendMessageW(hwnd, TCM_INSERTITEMW, static_cast<WPARAM>(at), reinterpret_cast<LPARAM>(&tab));
            ++at;
        }
        if (snapshot.selected >= 0) {
            SendMessageW(hwnd, TCM_SETCURSEL, static_cast<WPARAM>(snapshot.selected), 0);
        }
    } else if (kind == "statusbar") {
        SendMessageW(hwnd, SB_SIMPLE, snapshot.style == 1 ? TRUE : FALSE, 0);
        if (snapshot.style == 1) {
            std::wstring text = wide_text(snapshot.simple_text);
            SendMessageW(hwnd, SB_SETTEXTW, SB_SIMPLEID, reinterpret_cast<LPARAM>(text.c_str()));
        } else {
            std::vector<int> edges;
            int right = 0;
            RECT area;
            GetClientRect(hwnd, &area);
            for (std::size_t i = 0; i < snapshot.items.size(); ++i) {
                const long width = snapshot.items[i].width;
                right += width > 0 ? static_cast<int>(width * dpi_x_ / 1440) : 100;
                edges.push_back(i + 1U == snapshot.items.size() ? -1 : right);
            }
            if (edges.empty()) {
                edges.push_back(-1);
            }
            SendMessageW(hwnd, SB_SETPARTS, edges.size(), reinterpret_cast<LPARAM>(edges.data()));
            for (std::size_t i = 0; i < snapshot.items.size(); ++i) {
                std::wstring text = wide_text(snapshot.items[i].text);
                SendMessageW(hwnd, SB_SETTEXTW, i, reinterpret_cast<LPARAM>(text.c_str()));
            }
        }
    } else if (kind == "toolbar") {
        while (SendMessageW(hwnd, TB_BUTTONCOUNT, 0, 0) > 0) {
            SendMessageW(hwnd, TB_DELETEBUTTON, 0, 0);
        }
        SendMessageW(hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        long at = 0;
        for (const auto& item : snapshot.items) {
            std::wstring text = wide_text(item.text) + L'\0';
            const INT_PTR text_index = SendMessageW(hwnd, TB_ADDSTRINGW, 0, reinterpret_cast<LPARAM>(text.c_str()));
            TBBUTTON button{};
            button.idCommand = static_cast<int>(at) + 1;
            button.iBitmap = I_IMAGENONE;
            button.iString = static_cast<INT_PTR>(text_index);
            button.fsState = static_cast<BYTE>((item.enabled ? TBSTATE_ENABLED : 0) |
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
                    button.iBitmap = std::max<long>(8, item.width * dpi_x_ / 1440);
                    break;
                case 5:
                    button.fsStyle = BTNS_DROPDOWN | BTNS_AUTOSIZE;
                    break;
                default:
                    button.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
                    break;
            }
            button.fsStyle = static_cast<BYTE>(button.fsStyle | BTNS_SHOWTEXT);
            SendMessageW(hwnd, TB_INSERTBUTTONW, static_cast<WPARAM>(at), reinterpret_cast<LPARAM>(&button));
            ++at;
        }
        SendMessageW(hwnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
        SendMessageW(hwnd, TB_AUTOSIZE, 0, 0);
    } else if (kind == "treeview") {
        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        style |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS;
        style = snapshot.checkboxes ? (style | TVS_CHECKBOXES) : (style & ~TVS_CHECKBOXES);
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);
        SendMessageW(hwnd, TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(TVI_ROOT));
        std::vector<HTREEITEM> handles;
        for (const auto& item : snapshot.items) {
            std::wstring text = wide_text(item.text);
            TVINSERTSTRUCTW insert{};
            insert.hParent = item.parent < 0 ? TVI_ROOT : handles[static_cast<std::size_t>(item.parent)];
            insert.hInsertAfter = TVI_LAST;
            insert.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
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
                SendMessageW(hwnd, TVM_SETITEMW, 0, reinterpret_cast<LPARAM>(&check));
            }
            if (snapshot.items[i].expanded) {
                SendMessageW(hwnd, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(handles[i]));
            }
        }
        if (snapshot.selected >= 0) {
            SendMessageW(hwnd, TVM_SELECTITEM, TVGN_CARET,
                         reinterpret_cast<LPARAM>(handles[static_cast<std::size_t>(snapshot.selected)]));
        }
    } else if (kind == "listview") {
        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        style &= ~static_cast<LONG_PTR>(LVS_TYPEMASK);
        style |= snapshot.view == 0   ? LVS_ICON
                 : snapshot.view == 1 ? LVS_SMALLICON
                 : snapshot.view == 2 ? LVS_LIST
                                      : LVS_REPORT;
        style |= LVS_SHOWSELALWAYS;
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);
        DWORD extended = 0;
        extended |= snapshot.checkboxes ? LVS_EX_CHECKBOXES : 0;
        extended |= snapshot.gridlines ? LVS_EX_GRIDLINES : 0;
        extended |= snapshot.full_row ? LVS_EX_FULLROWSELECT : 0;
        SendMessageW(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, ~0U, extended);
        SendMessageW(hwnd, LVM_DELETEALLITEMS, 0, 0);
        while (SendMessageW(hwnd, LVM_DELETECOLUMN, 0, 0) != FALSE) {
        }
        for (std::size_t i = 0; i < snapshot.columns.size(); ++i) {
            std::wstring text = wide_text(snapshot.columns[i].text);
            LVCOLUMNW column{};
            column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;
            column.pszText = text.data();
            column.cx = static_cast<int>(snapshot.columns[i].width * dpi_x_ / 1440);
            column.fmt = snapshot.columns[i].alignment == 1   ? LVCFMT_RIGHT
                         : snapshot.columns[i].alignment == 2 ? LVCFMT_CENTER
                                                              : LVCFMT_LEFT;
            column.iSubItem = static_cast<int>(i);
            SendMessageW(hwnd, LVM_INSERTCOLUMNW, i, reinterpret_cast<LPARAM>(&column));
        }
        for (std::size_t i = 0; i < snapshot.items.size(); ++i) {
            std::wstring text = wide_text(snapshot.items[i].text);
            LVITEMW row{};
            row.mask = LVIF_TEXT | LVIF_PARAM;
            row.iItem = static_cast<int>(i);
            row.pszText = text.data();
            row.lParam = snapshot.items[i].index;
            SendMessageW(hwnd, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&row));
            for (std::size_t c = 0; c < snapshot.items[i].cells.size(); ++c) {
                std::wstring cell = wide_text(snapshot.items[i].cells[c]);
                LVITEMW sub{};
                sub.iSubItem = static_cast<int>(c) + 1;
                sub.pszText = cell.data();
                SendMessageW(hwnd, LVM_SETITEMTEXTW, i, reinterpret_cast<LPARAM>(&sub));
            }
            if (snapshot.items[i].checked && snapshot.checkboxes) {
                LVITEMW state{};
                state.stateMask = LVIS_STATEIMAGEMASK;
                state.state = INDEXTOSTATEIMAGEMASK(2);
                SendMessageW(hwnd, LVM_SETITEMSTATE, i, reinterpret_cast<LPARAM>(&state));
            }
        }
        if (snapshot.selected >= 0) {
            LVITEMW state{};
            state.stateMask = LVIS_SELECTED;
            state.state = LVIS_SELECTED;
            SendMessageW(hwnd, LVM_SETITEMSTATE, static_cast<WPARAM>(snapshot.selected),
                         reinterpret_cast<LPARAM>(&state));
        }
    } else if (kind == "imagecombo") {
        SendMessageW(hwnd, CB_RESETCONTENT, 0, 0);
        for (std::size_t i = 0; i < snapshot.items.size(); ++i) {
            std::wstring text = wide_text(snapshot.items[i].text);
            COMBOBOXEXITEMW item{};
            item.mask = CBEIF_TEXT | CBEIF_INDENT;
            item.iItem = -1;
            item.pszText = text.data();
            item.iIndent = static_cast<int>(snapshot.items[i].indent);
            SendMessageW(hwnd, CBEM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));
        }
        SendMessageW(hwnd, CB_SETCURSEL, static_cast<WPARAM>(snapshot.selected), 0);
        if (snapshot.selected < 0) {
            std::wstring text = wide_text(snapshot.text);
            SetWindowTextW(hwnd, text.c_str());
        }
        SendMessageW(hwnd, EM_SETREADONLY, snapshot.locked ? TRUE : FALSE, 0);
    }
    entry->applying = false;
}

// A notification of a common control: becomes a ComctlInput event.
bool Win32Host::common_notify(const void* raw_header,
                              const std::intptr_t lparam) {
    const NMHDR& header = *static_cast<const NMHDR*>(raw_header);
    const WindowId control = Win32Support::id_of(header.hwndFrom);
    auto* entry = find(control);
    if (entry == nullptr || entry->applying) {
        return false;
    }
    const auto post = [&](const std::string& action, const std::string& rest) {
        queue(UiEvent{control, "ComctlInput", {action, rest}});
    };
    const std::string& kind = entry->kind;
    if (kind == "TabStrip" && header.code == TCN_SELCHANGE) {
        post("select", std::to_string(SendMessageW(header.hwndFrom, TCM_GETCURSEL, 0, 0) + 1));
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
            hit.pt = {static_cast<short>(LOWORD(position)), static_cast<short>(HIWORD(position))};
            ScreenToClient(header.hwndFrom, &hit.pt);
            if (HTREEITEM item = reinterpret_cast<HTREEITEM>(SendMessageW(
                    header.hwndFrom, TVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hit)));
                item != nullptr && (hit.flags & TVHT_ONITEMSTATEICON) != 0) {
                TVITEMW data{};
                data.mask = TVIF_PARAM | TVIF_HANDLE;
                data.hItem = item;
                SendMessageW(header.hwndFrom, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&data));
                post("check", std::to_string(data.lParam));
                return true;
            }
        }
    }
    if (kind == "ListView") {
        const auto* list = reinterpret_cast<const NMLISTVIEW*>(lparam);
        if (header.code == LVN_ITEMCHANGED && (list->uChanged & LVIF_STATE) != 0 &&
            (list->uNewState & LVIS_SELECTED) != 0 && (list->uOldState & LVIS_SELECTED) == 0) {
            post("click", std::to_string(list->lParam));
            return true;
        }
        if (header.code == LVN_ITEMCHANGED && (list->uChanged & LVIF_STATE) != 0 &&
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
    if (kind == "StatusBar" && (header.code == NM_CLICK || header.code == NM_DBLCLK)) {
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
