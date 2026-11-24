#include "text_host.hpp"

#include "text_host_support.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <optional>

#include "../interpreter/pds_screen.hpp"
#include "../interpreter/vb_text.hpp"
#include "wfc/brun71/cp437.hpp"

namespace wfc::ui {
namespace {

using namespace text;  // NOLINT(google-build-using-namespace)

namespace pds = wfc::detail::pds;
using brun71::Region;

}  // namespace

TextHost::TextHost() {
    auto& screen = pds::screen();
    screen.view_print();
    background_ = screen.cells();
    pds::enable_mouse();
}

const TextHost::Window* TextHost::find(const WindowId window) const {
    const auto found = windows_.find(window);
    return found == windows_.end() ? nullptr : &found->second;
}

TextHost::Window* TextHost::find(const WindowId window) {
    const auto found = windows_.find(window);
    return found == windows_.end() ? nullptr : &found->second;
}

WindowId TextHost::form_of(WindowId window) const {
    while (const Window* entry = find(window)) {
        if (entry->parent == kNoWindow) {
            return window;
        }
        window = entry->parent;
    }
    return kNoWindow;
}

WindowId TextHost::active_form() const {
    if (const WindowId form = form_of(focus_);
        form != kNoWindow && shown(form)) {
        return form;
    }
    for (auto it = order_.rbegin(); it != order_.rend(); ++it) {
        if (shown(*it)) {
            return *it;
        }
    }
    return kNoWindow;
}

// A window and every window it is in are visible.
bool TextHost::shown(WindowId window) const {
    while (const Window* entry = find(window)) {
        if (!entry->visible) {
            return false;
        }
        if (entry->parent == kNoWindow) {
            return true;
        }
        window = entry->parent;
    }
    return false;
}

void TextHost::changed() {
    dirty_ = true;
}

WindowId TextHost::create_window(const std::string& kind,
                                 const WindowId parent) {
    const WindowId id = next_id_++;
    Window entry;
    entry.kind = kind;
    entry.parent = parent;
    windows_.emplace(id, std::move(entry));
    if (parent == kNoWindow) {
        order_.push_back(id);
    }
    changed();
    return id;
}

void TextHost::destroy_window(const WindowId window) {
    std::vector<WindowId> children;
    for (const auto& [id, entry] : windows_) {
        if (entry.parent == window) {
            children.push_back(id);
        }
    }
    for (const WindowId child : children) {
        destroy_window(child);
    }
    const bool form =
        find(window) != nullptr && find(window)->parent == kNoWindow;
    const bool had_focus = form && form_of(focus_) == kNoWindow;
    windows_.erase(window);
    timers_.erase(window);
    std::erase(order_, window);
    last_focus_.erase(window);
    if (focus_ == window) {
        focus_ = kNoWindow;
    }
    if (std::find(menu_path_.begin(), menu_path_.end(), window) !=
        menu_path_.end()) {
        close_menus();
    }
    if (had_focus) {
        focus_back(window);
    }
    changed();
}

// The active form went away (unloaded or hidden): the form now on top gets
// the focus back on the control that had it last, as in VB6.
void TextHost::focus_back(const WindowId leaving) {
    for (auto it = order_.rbegin(); it != order_.rend(); ++it) {
        if (*it == leaving || !shown(*it)) {
            continue;
        }
        const auto last = last_focus_.find(*it);
        const WindowId target =
            last != last_focus_.end() && find(last->second) != nullptr
                ? last->second
                : *it;
        queue_.push_back(UiEvent{target, "UserFocus", {}});
        return;
    }
}

void TextHost::set_bounds(const WindowId window, const Rect& bounds) {
    if (Window* entry = find(window)) {
        entry->bounds = bounds;
        changed();
    }
}

Rect TextHost::bounds(const WindowId window) const {
    const Window* entry = find(window);
    return entry != nullptr ? entry->bounds : Rect{};
}

void TextHost::set_visible(const WindowId window, const bool visible) {
    if (Window* entry = find(window)) {
        if (visible && !entry->visible && entry->parent == kNoWindow) {
            // A form that is shown becomes the active one, as in VB6.
            raise(window);
            queue_.push_back(UiEvent{window, "UserFocus", {}});
        }
        const bool hiding = !visible && entry->visible &&
                            entry->parent == kNoWindow &&
                            form_of(focus_) == window;
        entry->visible = visible;
        if (hiding) {
            focus_back(window);
        }
        changed();
    }
}

bool TextHost::visible(const WindowId window) const {
    const Window* entry = find(window);
    return entry != nullptr && entry->visible;
}

void TextHost::set_enabled(const WindowId window, const bool enabled) {
    if (Window* entry = find(window)) {
        entry->enabled = enabled;
        changed();
    }
}

bool TextHost::enabled(const WindowId window) const {
    const Window* entry = find(window);
    return entry != nullptr && entry->enabled;
}

void TextHost::set_text(const WindowId window, const std::string& text) {
    if (Window* entry = find(window)) {
        if (entry->text != text) {
            // A text the program sets puts the caret at its start, as
            // SelStart = 0 does.
            entry->text = text;
            entry->caret = 0;
            entry->scroll = 0;
        }
        changed();
    }
}

std::string TextHost::text(const WindowId window) const {
    const Window* entry = find(window);
    return entry != nullptr ? entry->text : std::string{};
}

void TextHost::set_items(const WindowId window,
                         const std::vector<std::string>& items) {
    if (Window* entry = find(window)) {
        entry->items = items;
        changed();
    }
}

void TextHost::set_selection(const WindowId window, const long index) {
    if (Window* entry = find(window)) {
        entry->selection = index;
        changed();
    }
}

long TextHost::selection(const WindowId window) const {
    const Window* entry = find(window);
    return entry != nullptr ? entry->selection : -1;
}

void TextHost::set_value(const WindowId window, const long value) {
    if (Window* entry = find(window)) {
        entry->value = value;
        changed();
    }
}

UiHost::KeyResult TextHost::control_key(const WindowId window, const long code,
                                        const long shift, const long ascii) {
    Window* entry = find(window);
    if (entry == nullptr || !entry->enabled) {
        return KeyResult::ignored;
    }
    if (entry->kind == "OptionButton") {
        if (ascii == 0 && code >= 37 && code <= 40) {
            next_option(window, code == 39 || code == 40);
            return KeyResult::handled;
        }
        return KeyResult::ignored;
    }
    if (entry->kind == "ListBox" || entry->kind == "ComboBox") {
        return list_key(window, *entry, code, shift, ascii);
    }
    if (entry->kind == "HScrollBar" || entry->kind == "VScrollBar") {
        return ascii == 0 ? scroll_key(window, *entry, code)
                          : KeyResult::ignored;
    }
    if (entry->kind != "TextBox") {
        return KeyResult::ignored;
    }
    std::u32string characters = decode(entry->text);
    switch (edit_line(characters, entry->caret, code, ascii)) {
        case Edit::ignored:
            return KeyResult::ignored;
        case Edit::moved:
            changed();
            return KeyResult::handled;
        case Edit::changed:
            break;
    }
    changed();
    entry->text = encode(characters);
    return KeyResult::text_changed;
}

// A list box or the list of a combo box: Up, Down, Page Up, Page Down,
// Home and End move the selection, and a letter selects the next item that
// starts with it (the model raises Click). F4 or Alt+Down opens and closes
// a combo box's list; Enter and Esc close it.
UiHost::KeyResult TextHost::list_key(const WindowId window, Window& list,
                                     const long code, const long shift,
                                     const long ascii) {
    const bool combo = list.kind == "ComboBox";
    const long count = static_cast<long>(list.items.size());
    if (combo && ascii == 0 && (code == 115 || (code == 40 && shift == 4))) {
        list.open = !list.open;
        changed();
        return KeyResult::handled;
    }
    if (combo && list.open && (ascii == 13 || ascii == 27)) {
        list.open = false;
        changed();
        return KeyResult::handled;
    }
    if (combo && list.open && ascii == 0 && (code == 13 || code == 27)) {
        return KeyResult::handled;  // the KeyDown of the Enter that closes it
    }
    if (count == 0) {
        return KeyResult::ignored;
    }
    const long current = list.selection;
    long next = current;
    // A page is the rows of the list (of an open combo box's list).
    const long page =
        std::max<long>(combo ? dropped_region_.height - 2
                             : cells(list.bounds.height, kCellHeight),
                       1);
    if (ascii > 32) {
        const auto wanted =
            static_cast<char>(std::tolower(static_cast<unsigned char>(ascii)));
        for (long step = 1; step <= count; ++step) {
            const long at = (std::max(current, -1L) + step) % count;
            const std::string& item = list.items[static_cast<std::size_t>(at)];
            if (!item.empty() && std::tolower(static_cast<unsigned char>(
                                     item.front())) == wanted) {
                next = at;
                break;
            }
        }
    } else if (ascii != 0 || shift != 0) {
        return KeyResult::ignored;
    } else if (code == 38) {
        next = std::max(current - 1, 0L);
    } else if (code == 40) {
        next = std::min(current + 1, count - 1);
    } else if (code == 33) {
        next = std::max(current - page, 0L);
    } else if (code == 34) {
        next = std::min(std::max(current, 0L) + page, count - 1);
    } else if (code == 36) {
        next = 0;
    } else if (code == 35) {
        next = count - 1;
    } else {
        return KeyResult::ignored;
    }
    if (next != current) {
        queue_.push_back(UiEvent{window, "UserSelect", {next}});
    }
    return KeyResult::handled;
}

// A scroll bar: Left and Up a small change down, Right and Down one up,
// Page Up and Page Down a large change, Home and End its ends (the model
// raises Change).
UiHost::KeyResult TextHost::scroll_key(const WindowId window, const Window& bar,
                                       const long code) {
    const auto by = [&](const long amount) {
        queue_.push_back(
            UiEvent{window, "UserScrollBy", {static_cast<double>(amount)}});
        return KeyResult::handled;
    };
    const auto to = [&](const long value) {
        queue_.push_back(
            UiEvent{window, "UserScroll", {static_cast<double>(value)}});
        return KeyResult::handled;
    };
    // Max may be below Min; a change moves toward Max.
    const long direction = bar.maximum >= bar.minimum ? 1 : -1;
    switch (code) {
        case 37:
        case 38:
            return by(-direction * bar.small_change);
        case 39:
        case 40:
            return by(direction * bar.small_change);
        case 33:
            return by(-direction * bar.large_change);
        case 34:
            return by(direction * bar.large_change);
        case 36:
            return to(bar.minimum);
        case 35:
            return to(bar.maximum);
        default:
            return KeyResult::ignored;
    }
}

void TextHost::set_scroll(const WindowId window, const long minimum,
                          const long maximum, const long value,
                          const long small_change, const long large_change) {
    if (Window* entry = find(window)) {
        entry->minimum = minimum;
        entry->maximum = maximum;
        entry->value = value;
        entry->small_change = small_change;
        entry->large_change = large_change;
        changed();
    }
}

// The arrow keys on an option button select the next one of its group (the
// option buttons of its container, top to bottom, then left to right).
void TextHost::next_option(const WindowId window, const bool forward) {
    const Window* self = find(window);
    std::vector<std::pair<const Window*, WindowId>> group;
    for (const auto& [id, entry] : windows_) {
        if (entry.parent == self->parent && entry.kind == "OptionButton" &&
            entry.visible && entry.enabled) {
            group.emplace_back(&entry, id);
        }
    }
    std::stable_sort(
        group.begin(), group.end(), [](const auto& a, const auto& b) {
            const Rect& x = a.first->bounds;
            const Rect& y = b.first->bounds;
            return x.top != y.top ? x.top < y.top : x.left < y.left;
        });
    for (std::size_t i = 0; i < group.size(); ++i) {
        if (group[i].second == window && group.size() > 1U) {
            const std::size_t next =
                forward ? (i + 1U) % group.size()
                        : (i + group.size() - 1U) % group.size();
            queue_.push_back(UiEvent{group[next].second, "Click", {}});
            return;
        }
    }
}

void TextHost::set_alignment(const WindowId window, const int alignment) {
    if (Window* entry = find(window)) {
        entry->alignment = alignment;
        changed();
    }
}

void TextHost::set_form_style(const WindowId window, const int border_style) {
    if (Window* entry = find(window)) {
        entry->border_style = border_style;
        changed();
    }
}

bool TextHost::screen_metric(const long index, long& value) const {
    // The screen in pixels of 15 twips, so Screen.Width is its cells in
    // twips.
    const auto& screen = pds::screen();
    if (index == 0) {
        value = static_cast<long>(screen.columns() * kCellWidth / 15.0);
        return true;
    }
    if (index == 1) {
        value = static_cast<long>(screen.rows() * kCellHeight / 15.0);
        return true;
    }
    return false;
}

void TextHost::raise(const WindowId form) {
    std::erase(order_, form);
    order_.push_back(form);
    changed();
}

void TextHost::set_focus(const WindowId window) {
    if (find(window) == nullptr) {
        return;
    }
    if (Window* previous = find(focus_); previous != nullptr) {
        previous->open = false;  // a combo box's list closes
    }
    focus_ = window;
    last_focus_[form_of(window)] = window;
    if (const WindowId form = form_of(window);
        form != kNoWindow && (order_.empty() || order_.back() != form)) {
        raise(form);
    }
    changed();
}

void TextHost::set_modal(const WindowId form, const bool modal) {
    if (modal) {
        modal_.push_back(form);
    } else if (!modal_.empty()) {
        modal_.pop_back();
    }
}

// Ctrl+F6 makes the next visible form (in the order they were created) the
// active one, unless a modal form runs.
void TextHost::next_form() {
    if (!modal_.empty()) {
        return;
    }
    std::vector<WindowId> forms;
    for (const auto& [id, entry] : windows_) {
        if (entry.parent == kNoWindow && entry.visible && entry.enabled) {
            forms.push_back(id);
        }
    }
    const auto at = std::find(forms.begin(), forms.end(), active_form());
    if (forms.size() < 2U || at == forms.end()) {
        return;
    }
    const WindowId next =
        std::next(at) == forms.end() ? forms.front() : *std::next(at);
    raise(next);
    queue_.push_back(UiEvent{next, "UserFocus", {}});
}

WindowId TextHost::focus() const {
    return focus_;
}

void TextHost::set_timer(const WindowId window, const long interval_ms) {
    if (interval_ms <= 0) {
        kill_timer(window);
        return;
    }
    timers_[window] = Timer{
        interval_ms, Clock::now() + std::chrono::milliseconds(interval_ms),
        virtual_now_ + interval_ms};
}

void TextHost::kill_timer(const WindowId window) {
    timers_.erase(window);
}

// With keys from the standard input, time passes only when a key of scan
// code 0 (the bytes 0, 0) is read: the virtual clock moves to the next
// timer that is due, which fires (with every other one due then). Programs
// with timers then run the same on every machine.
void TextHost::tick() {
    if (timers_.empty()) {
        return;
    }
    long next = timers_.begin()->second.virtual_due;
    for (const auto& [window, timer] : timers_) {
        next = std::min(next, timer.virtual_due);
    }
    virtual_now_ = next;
    for (auto& [window, timer] : timers_) {
        if (timer.virtual_due <= virtual_now_) {
            queue_.push_back(UiEvent{window, "Timer", {}});
            timer.virtual_due = virtual_now_ + timer.interval;
        }
    }
}

void TextHost::queue_timers() {
    if (!pds::keys_from_console()) {
        return;  // the virtual clock (tick)
    }
    const auto now = Clock::now();
    for (auto& [window, timer] : timers_) {
        if (timer.due <= now) {
            queue_.push_back(UiEvent{window, "Timer", {}});
            timer.due = now + std::chrono::milliseconds(timer.interval);
        }
    }
}

void TextHost::pump() {
    if (dirty_ && !frozen_) {
        repaint();
    }
}

void TextHost::refresh_window(const WindowId window) {
    static_cast<void>(window);
    changed();
    pump();
}

// A key from the keyboard: to the focused control (or the active form) as
// KeyDown, KeyPress and KeyUp. Alt+F4 closes the active form.
void TextHost::key(const int character, const int code) {
    if (!menu_path_.empty()) {
        menu_key(character, code);
        return;
    }
    if (menu_opener(character, code)) {
        return;
    }
    const WindowId form = active_form();
    WindowId target = focus_;
    if (target == kNoWindow || form_of(target) != form) {
        target = form;
    }
    if (target == kNoWindow) {
        return;
    }
    long virtual_key = 0;
    long shift = 0;
    long ascii = 0;
    if (character == 0) {
        if (code == 0) {
            tick();
            return;
        }
        if (code == 107) {
            queue_.push_back(UiEvent{form, "Close", {}});
            return;
        }
        if (code == 99) {  // Ctrl+F6
            next_form();
            return;
        }
        virtual_key = extended_key(code, shift);
    } else {
        // KeyAscii is ANSI: a code page 437 character above 127 becomes
        // its Windows-1252 byte.
        ascii = character < 128
                    ? character
                    : static_cast<long>(
                          wfc::detail::unicode_to_ansi(brun71::cp437_to_unicode(
                              static_cast<std::uint8_t>(character))));
        if (character == 8 || character == 9 || character == 13 ||
            character == 27 || character == 32) {
            virtual_key = character;
        } else if (character >= 1 && character <= 26) {
            virtual_key = 'A' + character - 1;  // Ctrl and a letter
            shift = 2;
        } else if (character < 128 && std::isalnum(character) != 0) {
            virtual_key = std::toupper(character);
            shift = std::isupper(character) != 0 ? 1 : 0;
        }
    }
    if (virtual_key != 0) {
        queue_.push_back(UiEvent{target, "KeyDown", {virtual_key, shift}});
    }
    if (ascii != 0 && (shift & 4) == 0) {
        queue_.push_back(UiEvent{target, "KeyPress", {ascii}});
    }
    if (virtual_key != 0) {
        queue_.push_back(UiEvent{target, "KeyUp", {virtual_key, shift}});
    }
}

std::optional<UiEvent> TextHost::next_event(const bool wait) {
    while (true) {
        pump();
        queue_timers();
        if (!queue_.empty()) {
            UiEvent event = std::move(queue_.front());
            queue_.pop_front();
            return event;
        }
        if (frozen_) {
            return std::nullopt;
        }
        if (pds::keys_ended()) {
            // The program's input is over: once it waits for input, what it
            // shows stays on the screen while the forms are closed. (DoEvents
            // does not wait: the program goes on and the screen with it.)
            if (wait) {
                frozen_ = true;
            }
            return std::nullopt;
        }
        int slice = wait ? kWaitSlice : 0;
        const auto now = Clock::now();
        for (const auto& [window, timer] : timers_) {
            const auto left =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    timer.due - now);
            slice = std::min(
                slice, static_cast<int>(std::max<long long>(left.count(), 0)));
        }
        if (const auto pressed = pds::next_key(slice)) {
            if (pressed->character == 0 && pressed->code == brun71::kMouseKey) {
                // A mouse event, in its place among the keys.
                if (const auto event = pds::next_mouse()) {
                    mouse(*event);
                }
                continue;
            }
            key(pressed->character, pressed->code);
            continue;
        }
        if (!wait) {
            queue_timers();
            if (queue_.empty()) {
                return std::nullopt;
            }
        }
    }
}

void TextHost::post_event(UiEvent event) {
    if (find(event.window) != nullptr) {
        queue_.push_back(std::move(event));
    }
}

bool TextHost::exhausted() const {
    if (!queue_.empty()) {
        return false;
    }
    if (frozen_) {
        return true;
    }
    for (const WindowId form : order_) {
        if (find(form) != nullptr) {
            return false;
        }
    }
    return true;
}

// ----- Drawing ----------------------------------------------------------

TextHost::Placed TextHost::place_form(const WindowId id,
                                      const Window& form) const {
    const auto& screen = pds::screen();
    const bool bordered = form.border_style != 0;
    const int edge = bordered ? 1 : 0;
    // A menu bar is a row of its own above the client area, as in VB6.
    const int bar = menu_items(id).empty() ? 0 : 1;
    const int width =
        std::max(cells(form.bounds.width, kCellWidth), 1) + 2 * edge;
    const int height =
        std::max(cells(form.bounds.height, kCellHeight), 1) + 2 * edge + bar;
    int left = cells(form.bounds.left, kCellWidth) + 1;
    int top = cells(form.bounds.top, kCellHeight) + 1;
    if (form.bounds.left == 0.0 && form.bounds.top == 0.0) {
        // A form the program has not placed is centred.
        left = (screen.columns() - width) / 2 + 1;
        top = (screen.rows() - height) / 2 + 1;
    }
    // A form that would leave the screen is moved back onto it.
    left = std::max(std::min(left, screen.columns() - width + 1), 1);
    top = std::max(std::min(top, screen.rows() - height + 1), 1);
    const Region outer{top, left, height, width};
    return Placed{outer,
                  Region{top + edge + bar, left + edge, height - 2 * edge - bar,
                         width - 2 * edge},
                  bar != 0 ? top + edge : 0};
}

void TextHost::write(const int row, const int column, const std::string& bytes,
                     const std::uint8_t attribute, const Region& clip) {
    auto& screen = pds::screen();
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        const int at = column + static_cast<int>(i);
        if (inside(clip, row, at)) {
            brun71::put_clipped(
                screen, row, at,
                brun71::Cell{static_cast<std::uint8_t>(bytes[i]), attribute});
        }
    }
}

void TextHost::repaint() {
    auto& screen = pds::screen();
    dirty_ = false;
    for (int row = 1; row <= screen.rows(); ++row) {
        for (int column = 1; column <= screen.columns(); ++column) {
            const auto index = static_cast<std::size_t>(
                (row - 1) * screen.columns() + (column - 1));
            if (index < background_.size()) {
                screen.put(row, column, background_[index]);
            }
        }
    }
    cursor_row_ = 0;
    cursor_column_ = 0;
    dropped_ = kNoWindow;
    hits_.clear();
    menu_bar_row_ = 0;
    menu_columns_.clear();
    const WindowId active = active_form();
    for (const WindowId id : order_) {
        const Window* form = find(id);
        if (form == nullptr || !form->visible) {
            continue;
        }
        const Placed place = place_form(id, *form);
        hits_.push_back(Hit{id, place.outer, place.client});
        brun71::fill(screen, place.outer, brun71::kWindowAttribute);
        if (form->border_style != 0) {
            const bool is_active = id == active;
            const auto attribute =
                is_active ? brun71::kActiveFrame : brun71::kWindowAttribute;
            brun71::frame(screen, place.outer,
                          is_active ? brun71::Border::double_line
                                    : brun71::Border::single,
                          attribute);
            brun71::title(screen, place.outer, pds::to_screen(form->text),
                          attribute);
            brun71::shadow(screen, place.outer);
        }
        if (place.bar_row != 0) {
            paint_menu_bar(id, place);
        }
        paint_children(id, place.client, place.client);
    }
    if (!menu_path_.empty()) {
        paint_menus();
        cursor_row_ = 0;
    }
    if (dialog_ != nullptr) {
        paint_dialog(*dialog_);
    }
    if (Window* combo = find(dropped_); combo != nullptr) {
        // The open list of a combo box, over everything else.
        const Region all{1, 1, screen.rows(), screen.columns()};
        brun71::frame(screen, dropped_region_, brun71::Border::single,
                      brun71::kWindowAttribute);
        const Region inside_box{
            dropped_region_.top + 1, dropped_region_.left + 1,
            dropped_region_.height - 2, dropped_region_.width - 2};
        paint_list(*combo, inside_box, all, true);
    }
    if (cursor_row_ > 0) {
        screen.locate(cursor_row_, cursor_column_, 1);
    } else {
        screen.locate(std::nullopt, std::nullopt, 0);
    }
    pds::present();
}

void TextHost::paint_children(const WindowId parent, const Region& origin,
                              const Region& clip) {
    for (const auto& [id, entry] : windows_) {
        if (entry.parent != parent || !entry.visible) {
            continue;
        }
        const Region region{
            origin.top + cells(entry.bounds.top, kCellHeight),
            origin.left + cells(entry.bounds.left, kCellWidth),
            std::max(cells(entry.bounds.height, kCellHeight), 1),
            std::max(cells(entry.bounds.width, kCellWidth), 1)};
        hits_.push_back(Hit{id, intersect(region, clip), region});
        paint_control(id, entry, region, clip);
        // A container's children are placed from its top-left cell.
        paint_children(id, region, intersect(region, clip));
    }
}

void TextHost::paint_control(const WindowId id, const Window& control,
                             const Region& region, const Region& clip) {
    auto& screen = pds::screen();
    const std::string& kind = control.kind;
    const bool has_focus = id == focus_;
    const std::uint8_t normal =
        control.enabled ? brun71::kWindowAttribute : kDisabledAttribute;
    const auto caption = brun71::caption(pds::to_screen(control.text));
    // The caption with its access key in white.
    const auto write_caption = [&](const int row, const int column,
                                   const int width,
                                   const std::uint8_t attribute) {
        const std::string text = caption.text.substr(
            0, static_cast<std::size_t>(std::max(width, 0)));
        write(row, column, text, attribute, clip);
        if (caption.access >= 0 &&
            caption.access < static_cast<int>(text.size()) && control.enabled &&
            attribute == brun71::kWindowAttribute) {
            write(row, column + caption.access,
                  text.substr(static_cast<std::size_t>(caption.access), 1),
                  kAccessAttribute, clip);
        }
    };
    const auto focus_at = [&](const int row, const int column) {
        if (has_focus && inside(clip, row, column)) {
            cursor_row_ = row;
            cursor_column_ = column;
        }
    };
    const int middle = region.top + (region.height - 1) / 2;
    if (kind == "Label") {
        // The caption wrapped on the label's rows, aligned in its width.
        std::string rest = caption.text;
        for (int row = region.top; row <= region.bottom() && !rest.empty();
             ++row) {
            std::string line =
                rest.substr(0, static_cast<std::size_t>(region.width));
            if (rest.size() > line.size()) {
                const auto space = line.rfind(' ');
                if (space != std::string::npos && space > 0) {
                    line.resize(space);
                }
            }
            rest.erase(0, line.size());
            while (!rest.empty() && rest.front() == ' ') {
                rest.erase(0, 1);
            }
            const int spare = region.width - static_cast<int>(line.size());
            const int shift = control.alignment == 1   ? spare
                              : control.alignment == 2 ? spare / 2
                                                       : 0;
            write(row, region.left + shift, line, normal, clip);
        }
    } else if (kind == "CommandButton") {
        const std::string face =
            brun71::button_face(caption.text, region.width);
        const auto start = face.find('<');
        const int column =
            region.left +
            static_cast<int>(start == std::string::npos ? 0 : start);
        write(middle, region.left, face,
              has_focus ? brun71::kFocusAttribute : normal, clip);
        if (caption.access >= 0 && !has_focus && control.enabled &&
            start != std::string::npos) {
            write(middle, column + 2 + caption.access,
                  caption.text.substr(static_cast<std::size_t>(caption.access),
                                      1),
                  kAccessAttribute, clip);
        }
        focus_at(middle, column + 2);
    } else if (kind == "CheckBox" || kind == "OptionButton") {
        const bool check = kind == "CheckBox";
        std::string mark = check ? "[ ] " : "( ) ";
        if (control.value != 0) {
            mark[1] = check ? 'X' : '*';
        }
        write(middle, region.left, mark, normal, clip);
        write_caption(middle, region.left + 4, region.width - 4, normal);
        focus_at(middle, region.left + 1);
    } else if (kind == "TextBox") {
        // One line, scrolled so that the caret is in the field.
        brun71::fill(screen, intersect(region, clip), brun71::kFieldAttribute);
        auto& entry = windows_.at(id);
        const std::string text = pds::to_screen(control.text);
        const auto width = static_cast<std::size_t>(region.width);
        entry.caret = std::min(entry.caret, text.size());
        if (entry.caret < entry.scroll) {
            entry.scroll = entry.caret;
        } else if (entry.caret >= entry.scroll + width) {
            entry.scroll = entry.caret - width + 1U;
        }
        entry.scroll = std::min(entry.scroll, text.size());
        write(region.top, region.left, text.substr(entry.scroll, width),
              brun71::kFieldAttribute, clip);
        focus_at(region.top,
                 region.left + static_cast<int>(entry.caret - entry.scroll));
    } else if (kind == "ComboBox") {
        const Region field{region.top, region.left, 1, region.width};
        brun71::fill(screen, intersect(field, clip), brun71::kFieldAttribute);
        const std::string text = pds::to_screen(control.text);
        write(field.top, field.left,
              text.substr(0, static_cast<std::size_t>(field.width)),
              brun71::kFieldAttribute, clip);
        write(field.top, field.right(), "\x19", normal, clip);
        focus_at(field.top, field.left);
        if (control.open && has_focus) {
            // Its list opens below it (above it when the screen ends).
            const int rows =
                std::min(static_cast<int>(control.items.size()), 8) + 2;
            int top = field.top + 1;
            if (top + rows - 1 > screen.rows()) {
                top = std::max(field.top - rows, 1);
            }
            dropped_ = id;
            dropped_region_ = Region{top, field.left, rows, field.width};
        }
    } else if (kind == "ListBox") {
        paint_list(windows_.at(id), region, clip, has_focus);
    } else if (kind == "HScrollBar" || kind == "VScrollBar") {
        paint_scroll(control, region, clip);
        if (has_focus) {
            cursor_row_ = 0;  // the thumb shows where it is
        }
    } else if (kind == "Frame" || kind == "PictureBox") {
        const Region box = intersect(region, clip);
        if (box.width == region.width && box.height == region.height) {
            brun71::frame(screen, region, brun71::Border::single, normal);
            if (kind == "Frame") {
                brun71::title(screen, region, caption.text, normal);
            }
        }
    }
}

// The items of a list from its first one shown, scrolled so that the
// selection is in view, with a scroll bar in its last column when they do
// not all fit. The selection is white on blue while the list has the focus.
void TextHost::paint_list(Window& list, const Region& region,
                          const Region& clip, const bool focused) {
    auto& screen = pds::screen();
    if (region.height < 1 || region.width < 1) {
        return;
    }
    brun71::fill(screen, intersect(region, clip), brun71::kFieldAttribute);
    const auto count = static_cast<long>(list.items.size());
    const long rows = region.height;
    const bool bar = count > rows && region.width > 1 && rows >= 2;
    const int width = region.width - (bar ? 1 : 0);
    auto top = static_cast<long>(list.top);
    if (list.selection >= 0) {
        if (list.selection < top) {
            top = list.selection;
        } else if (list.selection >= top + rows) {
            top = list.selection - rows + 1;
        }
    }
    top = std::max(std::min(top, count - rows), 0L);
    list.top = static_cast<std::size_t>(top);
    for (long row = 0; row < rows && top + row < count; ++row) {
        const long index = top + row;
        const bool chosen = index == list.selection;
        std::string text =
            pds::to_screen(list.items[static_cast<std::size_t>(index)])
                .substr(0, static_cast<std::size_t>(width));
        text.resize(static_cast<std::size_t>(width), ' ');
        const int at = region.top + static_cast<int>(row);
        write(at, region.left, text,
              chosen ? (focused ? brun71::kFocusAttribute
                                : brun71::kWindowAttribute)
                     : brun71::kFieldAttribute,
              clip);
        if (chosen && focused && inside(clip, at, region.left)) {
            cursor_row_ = at;
            cursor_column_ = region.left;
        }
    }
    if (focused && list.selection < 0 &&
        inside(clip, region.top, region.left)) {
        cursor_row_ = region.top;
        cursor_column_ = region.left;
    }
    if (bar) {
        const int column = region.right();
        const int track = region.height - 2;
        write(region.top, column, "\x1E", brun71::kWindowAttribute, clip);
        write(region.bottom(), column, "\x1F", brun71::kWindowAttribute, clip);
        const long room = std::max(count - rows, 1L);
        const int thumb =
            track > 0 ? static_cast<int>(top * (track - 1) / room) : 0;
        for (int i = 0; i < track; ++i) {
            write(region.top + 1 + i, column, i == thumb ? "\xDB" : "\xB0",
                  brun71::kWindowAttribute, clip);
        }
    }
}

// A scroll bar: arrows at its ends and a thumb on a track between them,
// placed by its Value between Min and Max.
void TextHost::paint_scroll(const Window& bar, const Region& region,
                            const Region& clip) {
    const bool horizontal = bar.kind == "HScrollBar";
    const int length = horizontal ? region.width : region.height;
    if (length < 2) {
        return;
    }
    const auto cell = [&](const int i, const char* glyph) {
        if (horizontal) {
            write(region.top + (region.height - 1) / 2, region.left + i, glyph,
                  brun71::kWindowAttribute, clip);
        } else {
            write(region.top + i, region.left, glyph, brun71::kWindowAttribute,
                  clip);
        }
    };
    cell(0, horizontal ? "\x11" : "\x1E");
    cell(length - 1, horizontal ? "\x10" : "\x1F");
    const int track = length - 2;
    if (track <= 0) {
        return;
    }
    const long span = bar.maximum - bar.minimum;
    const long offset = bar.value - bar.minimum;
    int thumb = 0;
    if (span != 0) {
        thumb = static_cast<int>(
            std::lround(static_cast<double>(offset) * (track - 1) /
                        static_cast<double>(span)));
    }
    thumb = std::max(std::min(thumb, track - 1), 0);
    for (int i = 0; i < track; ++i) {
        cell(1 + i, i == thumb ? "\xDB" : "\xB0");
    }
}

}  // namespace wfc::ui
