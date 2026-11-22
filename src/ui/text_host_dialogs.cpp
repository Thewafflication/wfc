// The menus and dialogs of the text host (ADR-0012): the menu bar of a form
// and its pull-down menus, MsgBox and InputBox.

#include <algorithm>
#include <cctype>
#include <iterator>

#include "../interpreter/pds_screen.hpp"
#include "text_host.hpp"
#include "text_host_support.hpp"
#include "wfc/brun71/cp437.hpp"

namespace wfc::ui {
namespace {

namespace pds = wfc::detail::pds;
using text::Region;

constexpr std::uint8_t kDisabled = 0x78;  // dark grey on grey
constexpr std::uint8_t kAccess = 0x7F;    // white on grey

// A caption as the screen shows it, with its access key.
brun71::Caption screen_caption(const std::string& text) {
    return brun71::caption(pds::to_screen(text));
}

char access_letter(const brun71::Caption& caption) {
    if (caption.access < 0 ||
        caption.access >= static_cast<int>(caption.text.size())) {
        return 0;
    }
    return static_cast<char>(std::tolower(static_cast<unsigned char>(
        caption.text[static_cast<std::size_t>(caption.access)])));
}

// The ANSI code of a key's character (code page 437 above 127).
long ansi_of(const int character) {
    if (character < 128) {
        return character;
    }
    return static_cast<long>(wfc::detail::unicode_to_ansi(
        brun71::cp437_to_unicode(static_cast<std::uint8_t>(character))));
}

// Text cut into lines of at most `width` characters, at blanks where it can.
std::vector<std::string> wrap(const std::vector<std::string>& paragraphs,
                              const int width) {
    std::vector<std::string> lines;
    const auto room = static_cast<std::size_t>(std::max(width, 1));
    for (const std::string& paragraph : paragraphs) {
        std::string rest = paragraph;
        do {
            std::string line = rest.substr(0, room);
            if (rest.size() > room) {
                const auto space = line.rfind(' ');
                if (space != std::string::npos && space > 0) {
                    line.resize(space);
                }
            }
            rest.erase(0, line.size());
            while (!rest.empty() && rest.front() == ' ') {
                rest.erase(0, 1);
            }
            lines.push_back(line);
        } while (!rest.empty());
    }
    return lines;
}

}  // namespace

// ----- Menus --------------------------------------------------------------

std::vector<WindowId> TextHost::menu_items(const WindowId owner) const {
    std::vector<WindowId> items;
    for (const auto& [id, entry] : windows_) {
        if (entry.kind == "Menu" && entry.parent == owner && entry.visible) {
            items.push_back(id);
        }
    }
    return items;
}

// F10, or Alt and the access key of a title on the active form's menu bar,
// opens that menu.
bool TextHost::menu_opener(const int character, const int code) {
    if (character != 0) {
        return false;
    }
    const auto titles = menu_items(active_form());
    if (titles.empty()) {
        return false;
    }
    if (code == 68) {  // F10
        for (const WindowId title : titles) {
            if (find(title)->enabled) {
                open_menu(title);
                return true;
            }
        }
        return false;
    }
    long shift = 0;
    const long letter = text::extended_key(code, shift);
    if (shift != 4 || letter < 'A' || letter > 'Z') {
        return false;
    }
    for (const WindowId title : titles) {
        const Window* entry = find(title);
        if (entry->enabled &&
            access_letter(screen_caption(entry->text)) ==
                static_cast<char>(std::tolower(static_cast<int>(letter)))) {
            open_menu(title);
            return true;
        }
    }
    return false;
}

void TextHost::open_menu(const WindowId top) {
    menu_path_.assign(1, top);
    menu_index_.assign(1, -1);
    const auto items = menu_items(top);
    for (std::size_t i = 0; i < items.size(); ++i) {
        const Window* item = find(items[i]);
        if (item->enabled && item->text != "-") {
            menu_index_[0] = static_cast<int>(i);
            break;
        }
    }
    // VB raises a menu's Click as it opens, so that its items can be set.
    queue_.push_back(UiEvent{top, "Click", {}});
    changed();
}

void TextHost::close_menus() {
    menu_path_.clear();
    menu_index_.clear();
    changed();
}

// A key while a menu is open: Up and Down move in it, Right opens an item's
// menu (or the next menu on the bar), Left goes back (or to the previous
// menu), Enter or an item's access key chooses it, Esc closes one menu and
// F10 all of them.
void TextHost::menu_key(const int character, const int code) {
    changed();
    const WindowId owner = menu_path_.back();
    const auto items = menu_items(owner);
    int& index = menu_index_.back();
    const auto selectable = [&](const int at) {
        const Window* item = find(items[static_cast<std::size_t>(at)]);
        return item != nullptr && item->enabled && item->text != "-";
    };
    const auto move = [&](const int step) {
        const int count = static_cast<int>(items.size());
        for (int tries = 0, at = index; tries < count; ++tries) {
            at = ((at + step) % count + count) % count;
            if (selectable(at)) {
                index = at;
                return;
            }
        }
    };
    const auto switch_title = [&](const int step) {
        const auto titles = menu_items(active_form());
        const auto at =
            std::find(titles.begin(), titles.end(), menu_path_.front());
        if (titles.empty() || at == titles.end()) {
            close_menus();
            return;
        }
        const int count = static_cast<int>(titles.size());
        int position = static_cast<int>(at - titles.begin());
        for (int tries = 0; tries < count; ++tries) {
            position = ((position + step) % count + count) % count;
            if (find(titles[static_cast<std::size_t>(position)])->enabled) {
                open_menu(titles[static_cast<std::size_t>(position)]);
                return;
            }
        }
    };
    const auto choose = [&](const int at) {
        if (at < 0 || !selectable(at)) {
            return;
        }
        const WindowId item = items[static_cast<std::size_t>(at)];
        index = at;
        if (!menu_items(item).empty()) {
            menu_path_.push_back(item);
            menu_index_.push_back(-1);
            const auto inner = menu_items(item);
            for (std::size_t i = 0; i < inner.size(); ++i) {
                const Window* entry = find(inner[i]);
                if (entry->enabled && entry->text != "-") {
                    menu_index_.back() = static_cast<int>(i);
                    break;
                }
            }
            queue_.push_back(UiEvent{item, "Click", {}});
            return;
        }
        close_menus();
        queue_.push_back(UiEvent{item, "Click", {}});
    };
    if (character == 27) {
        menu_path_.pop_back();
        menu_index_.pop_back();
        return;
    }
    if (character == 13) {
        choose(index);
        return;
    }
    if (character > 32) {
        const auto wanted = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (access_letter(screen_caption(find(items[i])->text)) == wanted) {
                choose(static_cast<int>(i));
                return;
            }
        }
        return;
    }
    if (character != 0 || items.empty()) {
        return;
    }
    switch (code) {
        case 72:  // Up
            move(-1);
            break;
        case 80:  // Down
            move(1);
            break;
        case 75:  // Left
            if (menu_path_.size() > 1U) {
                menu_path_.pop_back();
                menu_index_.pop_back();
            } else {
                switch_title(-1);
            }
            break;
        case 77:  // Right
            if (index >= 0 &&
                !menu_items(items[static_cast<std::size_t>(index)]).empty()) {
                choose(index);
            } else {
                switch_title(1);
            }
            break;
        case 68:  // F10
            close_menus();
            break;
        default:
            break;
    }
}

void TextHost::paint_menu_bar(const WindowId form, const Placed& place) {
    auto& screen = pds::screen();
    const bool active = form == active_form();
    const Region bar{place.bar_row, place.client.left, 1, place.client.width};
    brun71::fill(screen, bar, brun71::kWindowAttribute);
    if (active) {
        menu_bar_row_ = place.bar_row;
        menu_columns_.clear();
    }
    int column = bar.left + 1;
    for (const WindowId id : menu_items(form)) {
        const Window* title = find(id);
        const auto caption = screen_caption(title->text);
        const bool open =
            active && !menu_path_.empty() && menu_path_.front() == id;
        const std::uint8_t attribute = open ? brun71::kFocusAttribute
                                       : title->enabled
                                           ? brun71::kWindowAttribute
                                           : kDisabled;
        write(bar.top, column, " " + caption.text + " ", attribute, bar);
        if (attribute == brun71::kWindowAttribute && caption.access >= 0) {
            write(bar.top, column + 1 + caption.access,
                  caption.text.substr(static_cast<std::size_t>(caption.access),
                                      1),
                  kAccess, bar);
        }
        if (active) {
            menu_columns_[id] = column;
        }
        column += static_cast<int>(caption.text.size()) + 2;
    }
}

// The open menus, each a box below its title or beside its item.
void TextHost::paint_menus() {
    auto& screen = pds::screen();
    menu_boxes_.clear();
    const Region all{1, 1, screen.rows(), screen.columns()};
    Region previous{};
    for (std::size_t level = 0; level < menu_path_.size(); ++level) {
        const auto items = menu_items(menu_path_[level]);
        if (items.empty()) {
            continue;
        }
        int longest = 0;
        for (const WindowId id : items) {
            longest = std::max(
                longest,
                static_cast<int>(screen_caption(find(id)->text).text.size()));
        }
        // A check mark, a blank, the caption, two blanks and an arrow.
        const int width = longest + 7;
        const int height = static_cast<int>(items.size()) + 2;
        int top = 0;
        int left = 0;
        if (level == 0) {
            const auto column = menu_columns_.find(menu_path_[0]);
            top = menu_bar_row_ + 1;
            left = column != menu_columns_.end() ? column->second : 1;
        } else {
            top = previous.top + 1 + menu_index_[level - 1];
            left = previous.right() + 1;
            if (left + width - 1 > screen.columns()) {
                left = previous.left - width;
            }
        }
        left = std::max(std::min(left, screen.columns() - width + 1), 1);
        top = std::max(std::min(top, screen.rows() - height + 1), 1);
        const Region box{top, left, height, width};
        menu_boxes_.push_back(box);
        brun71::fill(screen, box, brun71::kWindowAttribute);
        brun71::frame(screen, box, brun71::Border::single,
                      brun71::kWindowAttribute);
        brun71::shadow(screen, box);
        for (std::size_t i = 0; i < items.size(); ++i) {
            const Window* item = find(items[i]);
            const int row = top + 1 + static_cast<int>(i);
            if (item->text == "-") {
                std::string line(static_cast<std::size_t>(width), '\xC4');
                line.front() = '\xC3';
                line.back() = '\xB4';
                write(row, left, line, brun71::kWindowAttribute, all);
                continue;
            }
            const auto caption = screen_caption(item->text);
            const bool highlighted = static_cast<int>(i) == menu_index_[level];
            const std::uint8_t attribute = highlighted ? brun71::kFocusAttribute
                                           : item->enabled
                                               ? brun71::kWindowAttribute
                                               : kDisabled;
            std::string line(static_cast<std::size_t>(width - 2), ' ');
            if (item->value != 0) {
                line[0] = '\xFB';  // the check mark
            }
            line.replace(2, caption.text.size(), caption.text);
            if (!menu_items(items[i]).empty()) {
                line.back() = '\x10';  // an arrow: the item opens a menu
            }
            write(row, left + 1, line, attribute, all);
            if (attribute == brun71::kWindowAttribute && caption.access >= 0) {
                write(row, left + 3 + caption.access,
                      caption.text.substr(
                          static_cast<std::size_t>(caption.access), 1),
                      kAccess, all);
            }
        }
        previous = box;
    }
}

// ----- Dialogs ------------------------------------------------------------

void TextHost::paint_dialog(const Dialog& dialog) {
    auto& screen = pds::screen();
    const Region all{1, 1, screen.rows(), screen.columns()};
    std::vector<std::string> faces;
    int buttons_width = 0;
    for (const std::string& button : dialog.buttons) {
        faces.push_back(screen_caption(button).text);
        buttons_width += static_cast<int>(faces.back().size()) + 4;
    }
    buttons_width += 2 * (static_cast<int>(faces.size()) - 1);
    std::vector<std::string> paragraphs;
    int content = std::max(buttons_width, 20);
    content = std::max(
        content, static_cast<int>(pds::to_screen(dialog.title).size()) + 4);
    for (const std::string& line : dialog.lines) {
        paragraphs.push_back(pds::to_screen(line));
        content = std::max(content, static_cast<int>(paragraphs.back().size()));
    }
    if (dialog.input) {
        content = std::max(content, 30);
    }
    content = std::min(content, screen.columns() - 8);
    const auto lines = wrap(paragraphs, content);
    const int height = static_cast<int>(lines.size()) + (dialog.input ? 8 : 6);
    const int width = content + 4;
    const Region box{std::max((screen.rows() - height) / 2 + 1, 1),
                     std::max((screen.columns() - width) / 2 + 1, 1), height,
                     width};
    brun71::fill(screen, box, brun71::kWindowAttribute);
    brun71::frame(screen, box, brun71::Border::double_line,
                  brun71::kActiveFrame);
    brun71::title(screen, box, pds::to_screen(dialog.title),
                  brun71::kActiveFrame);
    brun71::shadow(screen, box);
    int row = box.top + 2;
    for (const std::string& line : lines) {
        write(row++, box.left + 2, line, brun71::kWindowAttribute, all);
    }
    ++row;
    if (dialog.input) {
        const Region field{row, box.left + 2, 1, content};
        brun71::fill(screen, field, brun71::kFieldAttribute);
        const std::string shown = pds::to_screen(text::encode(dialog.text));
        const auto room = static_cast<std::size_t>(content);
        auto& scroll = const_cast<Dialog&>(dialog).scroll;
        if (dialog.caret < scroll) {
            scroll = dialog.caret;
        } else if (dialog.caret >= scroll + room) {
            scroll = dialog.caret - room + 1U;
        }
        write(row, field.left,
              shown.substr(std::min(scroll, shown.size()), room),
              brun71::kFieldAttribute, all);
        if (dialog.focus < 0) {
            cursor_row_ = row;
            cursor_column_ =
                field.left + static_cast<int>(dialog.caret - scroll);
        }
        row += 2;
    }
    int column = box.left + 2 + (content - buttons_width) / 2;
    dialog_buttons_.clear();
    for (std::size_t i = 0; i < faces.size(); ++i) {
        const auto caption = screen_caption(dialog.buttons[i]);
        const bool focused = static_cast<int>(i) == dialog.focus;
        dialog_buttons_.push_back(
            Region{row, column, 1, static_cast<int>(caption.text.size()) + 4});
        write(row, column, "< " + caption.text + " >",
              focused ? brun71::kFocusAttribute : brun71::kWindowAttribute,
              all);
        if (!focused && caption.access >= 0) {
            write(row, column + 2 + caption.access,
                  caption.text.substr(static_cast<std::size_t>(caption.access),
                                      1),
                  kAccess, all);
        }
        if (focused) {
            cursor_row_ = row;
            cursor_column_ = column + 2;
        }
        column += static_cast<int>(caption.text.size()) + 6;
    }
}

// Runs a dialog over the forms until a button is chosen: Tab and Shift+Tab
// (and Left and Right among the buttons) move the focus, Enter chooses the
// focused button (the first one from the input field), a button's access
// key chooses it, Esc gives `cancel` (-1 when the dialog has no answer to
// Esc). Returns the button's index, -1 for Esc, or -2 when the keys ended
// (the screen then keeps showing the dialog).
int TextHost::run_dialog(Dialog& dialog, const int cancel) {
    close_menus();
    dialog_ = &dialog;
    const int buttons = static_cast<int>(dialog.buttons.size());
    const int first = dialog.input ? -1 : 0;
    const auto step = [&](const int by) {
        const int count = buttons - first;
        dialog.focus =
            ((dialog.focus - first + by) % count + count) % count + first;
    };
    int result = -2;
    while (true) {
        if (!frozen_) {
            changed();
            repaint();
        }
        if (const auto click = pds::next_mouse()) {
            // A button released over a button chooses it.
            if (click->kind == brun71::MouseEvent::Kind::up &&
                (click->buttons & brun71::kLeftButton) != 0) {
                const int button = dialog_click(click->row, click->column);
                if (button >= 0) {
                    result = button;
                    break;
                }
            }
            continue;
        }
        const auto pressed = pds::next_key(text::kWaitSlice);
        if (!pressed) {
            if (frozen_ || pds::keys_ended()) {
                frozen_ = true;
                break;
            }
            continue;
        }
        const int character = pressed->character;
        const int code = pressed->code;
        if (character == 0 && code == 255) {
            continue;  // a mouse record
        }
        if (character == 9 || (character == 0 && code == 15)) {
            step(character == 9 ? 1 : -1);
        } else if (character == 13) {
            result = std::max(dialog.focus, 0);
            break;
        } else if (character == 27) {
            if (cancel != -2) {
                result = -1;
                break;
            }
        } else if (dialog.focus < 0) {
            // The input field.
            long key = 0;
            long ascii = 0;
            if (character != 0) {
                ascii = ansi_of(character);
            } else {
                key = code == 75   ? 37
                      : code == 77 ? 39
                      : code == 71 ? 36
                      : code == 79 ? 35
                      : code == 83 ? 46
                                   : 0;
            }
            static_cast<void>(
                text::edit_line(dialog.text, dialog.caret, key, ascii));
        } else if (character == 0 && (code == 75 || code == 77)) {
            if (buttons > 0) {
                dialog.focus =
                    ((dialog.focus + (code == 77 ? 1 : -1)) % buttons +
                     buttons) %
                    buttons;
            }
        } else if (character > 32) {
            const auto wanted = static_cast<char>(
                std::tolower(static_cast<unsigned char>(character)));
            bool chosen = false;
            for (int i = 0; i < buttons; ++i) {
                if (access_letter(screen_caption(
                        dialog.buttons[static_cast<std::size_t>(i)])) ==
                    wanted) {
                    result = i;
                    chosen = true;
                    break;
                }
            }
            if (chosen) {
                break;
            }
        }
    }
    if (!frozen_) {
        dialog_ = nullptr;
        changed();
    }
    return result;
}

std::optional<long> TextHost::message_box(const WindowId owner,
                                          const std::string& text,
                                          const std::string& title,
                                          const long style) {
    static_cast<void>(owner);
    struct Set {
        std::vector<std::string> buttons;
        std::vector<long> results;
        int cancel;  // the button Esc gives, -2 for none
    };
    static const Set kSets[] = {
        {{"OK"}, {1}, 0},
        {{"OK", "Cancel"}, {1, 2}, 1},
        {{"&Abort", "&Retry", "&Ignore"}, {3, 4, 5}, -2},
        {{"&Yes", "&No", "Cancel"}, {6, 7, 2}, 2},
        {{"&Yes", "&No"}, {6, 7}, -2},
        {{"&Retry", "Cancel"}, {4, 2}, 1},
    };
    const auto kind = static_cast<std::size_t>(style & 7);
    const Set& set = kSets[kind < std::size(kSets) ? kind : 0];
    Dialog dialog;
    dialog.title = title;
    std::string line;
    for (const char c : text) {
        if (c == '\n') {
            dialog.lines.push_back(line);
            line.clear();
        } else if (c != '\r') {
            line.push_back(c);
        }
    }
    dialog.lines.push_back(line);
    dialog.buttons = set.buttons;
    const int count = static_cast<int>(set.buttons.size());
    const int default_button = static_cast<int>((style >> 8) & 3);
    dialog.focus = default_button < count ? default_button : 0;
    const int chosen = run_dialog(dialog, set.cancel);
    int index = chosen;
    if (chosen == -1) {
        index = set.cancel;
    } else if (chosen < 0) {
        index = set.cancel >= 0 ? set.cancel : dialog.focus;
    }
    return set.results[static_cast<std::size_t>(index)];
}

std::optional<std::string> TextHost::input_box(const WindowId owner,
                                               const std::string& prompt,
                                               const std::string& title,
                                               const std::string& initial,
                                               bool& supported) {
    static_cast<void>(owner);
    supported = true;
    Dialog dialog;
    dialog.title = title;
    dialog.lines.push_back(prompt);
    dialog.buttons = {"OK", "Cancel"};
    dialog.input = true;
    dialog.focus = -1;
    dialog.text = text::decode(initial);
    dialog.caret = dialog.text.size();
    if (run_dialog(dialog, 1) != 0) {
        return std::nullopt;
    }
    return text::encode(dialog.text);
}

}  // namespace wfc::ui
