// The mouse of the text host (ADR-0012): what is under the pointer, and the
// events a click, a release and a move give it.

#include <algorithm>

#include "../interpreter/pds_screen.hpp"
#include "text_host.hpp"
#include "text_host_support.hpp"

namespace wfc::ui {
namespace {

namespace pds = wfc::detail::pds;
using text::inside;
using text::Region;

using Kind = brun71::MouseEvent::Kind;

}  // namespace

// A click on the menus while one is open: an item of an open menu chooses
// it (as Enter does), a title on the menu bar opens its menu, anywhere else
// closes them. Returns true when the click was on the menus.
bool TextHost::menu_click(const int row, const int column) {
    for (std::size_t level = menu_boxes_.size(); level-- > 0;) {
        const Region& box = menu_boxes_[level];
        if (!inside(box, row, column) || level >= menu_path_.size()) {
            continue;
        }
        // Close the menus below this one, then choose the item.
        menu_path_.resize(level + 1U);
        menu_index_.resize(level + 1U);
        const int item = row - box.top - 1;
        if (item >= 0 && item < box.height - 2) {
            menu_index_.back() = item;
            menu_key(13, 0);
        }
        return true;
    }
    if (row == menu_bar_row_) {
        for (const auto& [title, start] : menu_columns_) {
            const Window* entry = find(title);
            const int width =
                static_cast<int>(
                    brun71::caption(pds::to_screen(entry->text)).text.size()) +
                2;
            if (column >= start && column < start + width && entry->enabled) {
                if (!menu_path_.empty() && menu_path_.front() == title) {
                    close_menus();
                } else {
                    open_menu(title);
                }
                return true;
            }
        }
    }
    if (!menu_path_.empty()) {
        close_menus();
        return true;
    }
    return false;
}

// The dialog button at a cell, or -1.
int TextHost::dialog_click(const int row, const int column) const {
    for (std::size_t i = 0; i < dialog_buttons_.size(); ++i) {
        if (inside(dialog_buttons_[i], row, column)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void TextHost::mouse(const brun71::MouseEvent& event) {
    const bool left = (event.buttons & brun71::kLeftButton) != 0;
    const bool pressed =
        event.kind == Kind::down || event.kind == Kind::double_click;
    const long shift = (event.shift ? 1L : 0L) | (event.control ? 2L : 0L) |
                       (event.alt ? 4L : 0L);
    // Open menus and an open combo box list take the click first.
    if (pressed && left && menu_click(event.row, event.column)) {
        changed();
        return;
    }
    if (!menu_path_.empty()) {
        return;
    }
    if (Window* combo = find(dropped_); combo != nullptr && pressed) {
        const Region list{dropped_region_.top + 1, dropped_region_.left + 1,
                          dropped_region_.height - 2,
                          dropped_region_.width - 2};
        if (inside(list, event.row, event.column) && left) {
            const long index =
                static_cast<long>(combo->top) + (event.row - list.top);
            if (index < static_cast<long>(combo->items.size())) {
                queue_.push_back(UiEvent{dropped_, "UserSelect", {index}});
            }
        }
        combo->open = false;
        changed();
        if (inside(dropped_region_, event.row, event.column)) {
            return;
        }
    }
    // The window under the pointer: the last one drawn there.
    const Hit* hit = nullptr;
    for (auto it = hits_.rbegin(); it != hits_.rend(); ++it) {
        if (inside(it->area, event.row, event.column)) {
            hit = &*it;
            break;
        }
    }
    // A release goes to the window the button went down on.
    const Hit* target = hit;
    if ((event.kind == Kind::up ||
         (event.kind == Kind::move && event.buttons != 0)) &&
        captured_ != kNoWindow) {
        target = nullptr;
        for (const Hit& candidate : hits_) {
            if (candidate.window == captured_) {
                target = &candidate;
            }
        }
    }
    if (target == nullptr) {
        if (event.kind == Kind::up) {
            captured_ = kNoWindow;
        }
        return;
    }
    const WindowId window = target->window;
    Window* entry = find(window);
    if (entry == nullptr) {
        return;
    }
    // While a modal form runs, the mouse does nothing on the other forms;
    // a press on another form makes it the active one.
    const WindowId owner = form_of(window);
    if (!modal_.empty() && owner != modal_.back()) {
        return;
    }
    if (pressed && owner != active_form()) {
        raise(owner);
        queue_.push_back(UiEvent{owner, "UserFocus", {}});
    }
    // The centre of the cell, in twips from the window's origin.
    const double x = (event.column - target->origin.left) * text::kCellWidth +
                     text::kCellWidth / 2.0;
    const double y = (event.row - target->origin.top) * text::kCellHeight +
                     text::kCellHeight / 2.0;
    const std::string& kind = entry->kind;
    if (event.kind == Kind::move) {
        if (event.row == mouse_row_ && event.column == mouse_column_) {
            return;
        }
        mouse_row_ = event.row;
        mouse_column_ = event.column;
        queue_.push_back(
            UiEvent{window,
                    "MouseMove",
                    {static_cast<long>(event.buttons), shift, x, y}});
        return;
    }
    if (event.kind == Kind::up) {
        queue_.push_back(
            UiEvent{window,
                    "MouseUp",
                    {static_cast<long>(event.buttons), shift, x, y}});
        // A left click released over where it went down clicks the window
        // (lists, combo boxes and scroll bars act as it goes down).
        if (left && !double_clicked_ && hit != nullptr &&
            hit->window == window && entry->enabled && kind != "ListBox" &&
            kind != "ComboBox" && kind != "HScrollBar" &&
            kind != "VScrollBar") {
            queue_.push_back(UiEvent{window, "Click", {}});
        }
        captured_ = kNoWindow;
        double_clicked_ = false;
        return;
    }
    // A button goes down.
    captured_ = window;
    double_clicked_ = event.kind == Kind::double_click;
    const Region& area = target->origin;
    if (left && entry->enabled) {
        if (kind == "ListBox") {
            const long count = static_cast<long>(entry->items.size());
            const bool bar =
                count > area.height && area.width > 1 && area.height >= 2;
            long index = static_cast<long>(entry->top) + (event.row - area.top);
            if (bar && event.column == area.right()) {
                // The scroll bar's arrows move the selection.
                index = entry->selection + (event.row == area.top        ? -1
                                            : event.row == area.bottom() ? 1
                                                                         : 0);
            }
            index = std::clamp(index, 0L, std::max(count - 1, 0L));
            if (count > 0) {
                queue_.push_back(UiEvent{window, "UserSelect", {index}});
            }
        } else if (kind == "ComboBox") {
            queue_.push_back(UiEvent{window, "UserFocus", {}});
            entry->open = !entry->open;
        } else if (kind == "HScrollBar" || kind == "VScrollBar") {
            const bool horizontal = kind == "HScrollBar";
            const int at =
                horizontal ? event.column - area.left : event.row - area.top;
            const int length = horizontal ? area.width : area.height;
            const long direction = entry->maximum >= entry->minimum ? 1 : -1;
            long amount = 0;
            if (at == 0) {
                amount = -direction * entry->small_change;
            } else if (at == length - 1) {
                amount = direction * entry->small_change;
            } else {
                // The track before or after the thumb.
                const long span = entry->maximum - entry->minimum;
                const int track = length - 2;
                const int thumb =
                    span == 0
                        ? 0
                        : static_cast<int>((entry->value - entry->minimum) *
                                           (track - 1) / span);
                amount = (at - 1 < thumb   ? -1
                          : at - 1 > thumb ? 1
                                           : 0) *
                         direction * entry->large_change;
            }
            queue_.push_back(UiEvent{window, "UserFocus", {}});
            if (amount != 0) {
                queue_.push_back(UiEvent{
                    window, "UserScrollBy", {static_cast<double>(amount)}});
            }
        } else if (kind == "TextBox") {
            // The caret goes where the text was clicked.
            const auto length = text::decode(entry->text).size();
            entry->caret =
                std::min(entry->scroll + static_cast<std::size_t>(std::max(
                                             event.column - area.left, 0)),
                         length);
        }
    }
    queue_.push_back(UiEvent{
        window, "MouseDown", {static_cast<long>(event.buttons), shift, x, y}});
    if (double_clicked_) {
        queue_.push_back(UiEvent{window, "DblClick", {}});
    }
    changed();
}

}  // namespace wfc::ui
