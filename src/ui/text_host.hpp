// The text host (ADR-0012): VB6 forms and controls drawn on the text screen
// of wfc-brun71, with keys from the console or the standard input. The form
// model above it is the one the Win32 and headless hosts serve; this host
// turns keys into the KeyDown, KeyPress and KeyUp events of the focused
// control, and the model moves the focus and presses buttons. Internal to
// the WFC runtime; not part of the public API.

#ifndef WFC_UI_TEXT_HOST_HPP
#define WFC_UI_TEXT_HOST_HPP

#include <chrono>
#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <vector>

#include "ui_host.hpp"
#include "wfc/brun71/forms.hpp"
#include "wfc/brun71/mouse.hpp"

namespace wfc::ui {

class TextHost final : public UiHost {
public:
    TextHost();

    [[nodiscard]] WindowId create_window(const std::string& kind,
                                         WindowId parent) override;
    void destroy_window(WindowId window) override;
    void set_bounds(WindowId window, const Rect& bounds) override;
    [[nodiscard]] Rect bounds(WindowId window) const override;
    void set_visible(WindowId window, bool visible) override;
    [[nodiscard]] bool visible(WindowId window) const override;
    void set_enabled(WindowId window, bool enabled) override;
    [[nodiscard]] bool enabled(WindowId window) const override;
    void set_text(WindowId window, const std::string& text) override;
    [[nodiscard]] std::string text(WindowId window) const override;
    void set_items(WindowId window,
                   const std::vector<std::string>& items) override;
    void set_selection(WindowId window, long index) override;
    [[nodiscard]] long selection(WindowId window) const override;
    void set_value(WindowId window, long value) override;
    [[nodiscard]] KeyResult control_key(WindowId window, long code, long shift,
                                        long ascii) override;
    void set_scroll(WindowId window, long minimum, long maximum, long value,
                    long small_change, long large_change) override;
    void set_alignment(WindowId window, int alignment) override;
    void set_form_style(WindowId window, int border_style) override;
    [[nodiscard]] bool screen_metric(long index, long& value) const override;
    [[nodiscard]] std::optional<long> message_box(WindowId owner,
                                                  const std::string& text,
                                                  const std::string& title,
                                                  long style) override;
    [[nodiscard]] std::optional<std::string> input_box(
        WindowId owner, const std::string& prompt, const std::string& title,
        const std::string& initial, bool& supported) override;
    void set_focus(WindowId window) override;
    [[nodiscard]] WindowId focus() const override;
    void set_timer(WindowId window, long interval_ms) override;
    void kill_timer(WindowId window) override;
    void pump() override;
    void refresh_window(WindowId window) override;
    [[nodiscard]] std::optional<UiEvent> next_event(bool wait) override;
    void post_event(UiEvent event) override;
    [[nodiscard]] bool exhausted() const override;

private:
    using Clock = std::chrono::steady_clock;

    struct Window {
        std::string kind;
        WindowId parent{kNoWindow};
        Rect bounds;
        bool visible{false};
        bool enabled{true};
        std::string text;
        std::vector<std::string> items;
        long selection{-1};
        long value{0};
        int alignment{0};
        int border_style{2};
        // A text box: the caret and the first character shown, counted in
        // characters.
        std::size_t caret{0};
        std::size_t scroll{0};
        // A list: the first item shown; a combo box: its list is open.
        std::size_t top{0};
        bool open{false};
        // A scroll bar.
        long minimum{0};
        long maximum{32767};
        long small_change{1};
        long large_change{1};
    };
    // A timer is due on the clock (keys from the console) or, with keys
    // from the standard input, on the virtual clock that a key of scan code
    // 0 advances.
    struct Timer {
        long interval{};
        Clock::time_point due;
        long virtual_due{};
    };
    // The cells a window covers on the screen, and the cells its children
    // are clipped to; a form with menus has its menu bar on `bar_row`.
    struct Placed {
        brun71::Region outer;
        brun71::Region client;
        int bar_row{0};
    };
    // A MsgBox or InputBox on the screen: its text in lines, its buttons
    // (captions with access keys), the one that has the focus (-1: the
    // input field), and the input field's text and caret.
    struct Dialog {
        std::string title;
        std::vector<std::string> lines;
        std::vector<std::string> buttons;
        int focus{0};
        bool input{false};
        std::u32string text;
        std::size_t caret{0};
        std::size_t scroll{0};
    };

    [[nodiscard]] const Window* find(WindowId window) const;
    [[nodiscard]] Window* find(WindowId window);
    [[nodiscard]] WindowId form_of(WindowId window) const;
    [[nodiscard]] WindowId active_form() const;
    [[nodiscard]] bool shown(WindowId window) const;
    [[nodiscard]] Placed place_form(WindowId id, const Window& form) const;
    void raise(WindowId form);
    void next_option(WindowId window, bool forward);
    [[nodiscard]] KeyResult list_key(WindowId window, Window& list, long code,
                                     long shift, long ascii);
    [[nodiscard]] KeyResult scroll_key(WindowId window, const Window& bar,
                                       long code);
    void key(int character, int code);
    // The mouse (text_host_mouse.cpp).
    void mouse(const brun71::MouseEvent& event);
    [[nodiscard]] bool menu_click(int row, int column);
    [[nodiscard]] int dialog_click(int row, int column) const;
    // Menus (text_host_dialogs.cpp).
    [[nodiscard]] std::vector<WindowId> menu_items(WindowId owner) const;
    [[nodiscard]] bool menu_opener(int character, int code);
    void menu_key(int character, int code);
    void open_menu(WindowId top);
    void close_menus();
    void paint_menu_bar(WindowId form, const Placed& place);
    void paint_menus();
    // Dialogs: runs the dialog until a button is chosen; the button's index,
    // or -1 for Esc (-2 when the keys ended).
    [[nodiscard]] int run_dialog(Dialog& dialog, int cancel);
    void paint_dialog(const Dialog& dialog);
    void queue_timers();
    void tick();
    void paint_list(Window& list, const brun71::Region& region,
                    const brun71::Region& clip, bool focused);
    void paint_scroll(const Window& bar, const brun71::Region& region,
                      const brun71::Region& clip);
    void changed();
    void repaint();
    void paint_children(WindowId parent, const brun71::Region& origin,
                        const brun71::Region& clip);
    void paint_control(WindowId id, const Window& control,
                       const brun71::Region& region,
                       const brun71::Region& clip);
    void write(int row, int column, const std::string& bytes,
               std::uint8_t attribute, const brun71::Region& clip);

    std::map<WindowId, Window> windows_;
    std::vector<WindowId> order_;  // forms, the top one last
    std::map<WindowId, Timer> timers_;
    std::deque<UiEvent> queue_;
    std::vector<brun71::Cell> background_;
    WindowId next_id_{1};
    WindowId focus_{kNoWindow};
    bool dirty_{true};
    bool frozen_{false};  // the keys have ended: the screen stays as it is
    long virtual_now_{0};
    // The open combo box, whose list is drawn over everything else.
    WindowId dropped_{kNoWindow};
    brun71::Region dropped_region_;
    // The open menus, from the one on the menu bar down, and the item
    // highlighted in each; the row of the active form's menu bar and the
    // column of each title on it.
    std::vector<WindowId> menu_path_;
    std::vector<int> menu_index_;
    int menu_bar_row_{0};
    std::map<WindowId, int> menu_columns_;
    // The dialog on the screen, drawn over everything else.
    const Dialog* dialog_{nullptr};
    // Where the last repaint drew each window, in the order it drew them
    // (the one on top last): the cells it covers and the cell its
    // coordinates start from. The boxes of the open menus, by level; the
    // buttons of the dialog.
    struct Hit {
        WindowId window{kNoWindow};
        brun71::Region area;
        brun71::Region origin;
    };
    std::vector<Hit> hits_;
    std::vector<brun71::Region> menu_boxes_;
    std::vector<brun71::Region> dialog_buttons_;
    // The window a button went down on gets its MouseUp (the capture); a
    // double click's release raises no Click.
    WindowId captured_{kNoWindow};
    bool double_clicked_{false};
    int mouse_row_{0};
    int mouse_column_{0};
    int cursor_row_{0};
    int cursor_column_{0};
};

}  // namespace wfc::ui

#endif
