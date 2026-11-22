#include "text_host.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <optional>

#include "../interpreter/pds_screen.hpp"

namespace wfc::ui {
namespace {

namespace pds = wfc::detail::pds;
using brun71::Region;

// A character cell in twips (VB6's ScaleMode 4, vbCharacters).
constexpr double kCellWidth = 120.0;
constexpr double kCellHeight = 240.0;
// How long a wait for a key lasts before the timers are checked again.
constexpr int kWaitSlice = 50;

constexpr std::uint8_t kDisabledAttribute = 0x78;  // dark grey on grey
constexpr std::uint8_t kAccessAttribute = 0x7F;    // white on grey

int cells(const double twips, const double size) {
    return static_cast<int>(std::lround(twips / size));
}

Region intersect(const Region& a, const Region& b) {
    const int top = std::max(a.top, b.top);
    const int left = std::max(a.left, b.left);
    const int bottom = std::min(a.bottom(), b.bottom());
    const int right = std::min(a.right(), b.right());
    return Region{top, left, std::max(bottom - top + 1, 0),
                  std::max(right - left + 1, 0)};
}

bool inside(const Region& region, const int row, const int column) {
    return row >= region.top && row <= region.bottom() &&
           column >= region.left && column <= region.right();
}

// The virtual-key code of a BIOS scan code of an extended key, with the
// shift mask (1 Shift, 2 Ctrl, 4 Alt), or 0.
long extended_key(const int code, long& shift) {
    shift = 0;
    switch (code) {
        case 15:
            shift = 1;
            return 9;  // Shift+Tab
        case 71:
            return 36;  // Home
        case 72:
            return 38;  // Up
        case 73:
            return 33;  // Page Up
        case 75:
            return 37;  // Left
        case 77:
            return 39;  // Right
        case 79:
            return 35;  // End
        case 80:
            return 40;  // Down
        case 81:
            return 34;  // Page Down
        case 82:
            return 45;  // Insert
        case 83:
            return 46;  // Delete
        case 133:
            return 122;  // F11
        case 134:
            return 123;  // F12
        default:
            break;
    }
    if (code >= 59 && code <= 68) {
        return 112 + (code - 59);  // F1-F10
    }
    // Alt and a letter: the scan codes of the three letter rows.
    static constexpr const char* kRows[] = {"QWERTYUIOP", "ASDFGHJKL",
                                            "ZXCVBNM"};
    static constexpr int kFirst[] = {16, 30, 44};
    for (int row = 0; row < 3; ++row) {
        const std::string_view letters = kRows[row];
        if (code >= kFirst[row] &&
            code < kFirst[row] + static_cast<int>(letters.size())) {
            shift = 4;
            return letters[static_cast<std::size_t>(code - kFirst[row])];
        }
    }
    if (code >= 120 && code <= 129) {
        shift = 4;
        return code == 129 ? '0' : '1' + (code - 120);  // Alt and a digit
    }
    return 0;
}

}  // namespace

TextHost::TextHost() {
    auto& screen = pds::screen();
    screen.view_print();
    background_ = screen.cells();
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
    windows_.erase(window);
    timers_.erase(window);
    std::erase(order_, window);
    if (focus_ == window) {
        focus_ = kNoWindow;
    }
    changed();
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
            raise(window);
        }
        entry->visible = visible;
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
        entry->text = text;
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
    focus_ = window;
    if (const WindowId form = form_of(window);
        form != kNoWindow && (order_.empty() || order_.back() != form)) {
        raise(form);
    }
    changed();
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
        interval_ms, Clock::now() + std::chrono::milliseconds(interval_ms)};
}

void TextHost::kill_timer(const WindowId window) {
    timers_.erase(window);
}

void TextHost::queue_timers() {
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
        if (code == 107) {
            queue_.push_back(UiEvent{form, "Close", {}});
            return;
        }
        virtual_key = extended_key(code, shift);
    } else {
        ascii = character;
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
            // The program's input is over: what it shows now stays on the
            // screen while the forms are closed.
            frozen_ = true;
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

TextHost::Placed TextHost::place_form(const Window& form) const {
    const auto& screen = pds::screen();
    const bool bordered = form.border_style != 0;
    const int edge = bordered ? 1 : 0;
    const int width =
        std::max(cells(form.bounds.width, kCellWidth), 1) + 2 * edge;
    const int height =
        std::max(cells(form.bounds.height, kCellHeight), 1) + 2 * edge;
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
    return Placed{outer, Region{top + edge, left + edge, height - 2 * edge,
                                width - 2 * edge}};
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
    const WindowId active = active_form();
    for (const WindowId id : order_) {
        const Window* form = find(id);
        if (form == nullptr || !form->visible) {
            continue;
        }
        const Placed place = place_form(*form);
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
        paint_children(id, place.client, place.client);
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
    } else if (kind == "TextBox" || kind == "ComboBox") {
        const Region field{region.top, region.left,
                           kind == "ComboBox" ? 1 : region.height,
                           region.width};
        const Region cut = intersect(field, clip);
        brun71::fill(screen, cut, brun71::kFieldAttribute);
        const std::string text = pds::to_screen(control.text);
        write(field.top, field.left,
              text.substr(0, static_cast<std::size_t>(field.width)),
              brun71::kFieldAttribute, clip);
        if (kind == "ComboBox") {
            write(field.top, field.right(), "\x19", normal, clip);
        }
        focus_at(field.top,
                 field.left + std::min(static_cast<int>(text.size()),
                                       std::max(field.width - 1, 0)));
    } else if (kind == "ListBox") {
        brun71::fill(screen, intersect(region, clip), brun71::kFieldAttribute);
        for (int i = 0;
             i < region.height && i < static_cast<int>(control.items.size());
             ++i) {
            const bool chosen = i == control.selection;
            write(region.top + i, region.left,
                  pds::to_screen(control.items[static_cast<std::size_t>(i)])
                      .substr(0, static_cast<std::size_t>(region.width)),
                  chosen ? brun71::kFocusAttribute : brun71::kFieldAttribute,
                  clip);
        }
        focus_at(region.top + std::max<int>(control.selection, 0), region.left);
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

}  // namespace wfc::ui
