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
    [[nodiscard]] bool control_key(WindowId window, long code, long shift,
                                   long ascii) override;
    void set_alignment(WindowId window, int alignment) override;
    void set_form_style(WindowId window, int border_style) override;
    [[nodiscard]] bool screen_metric(long index, long& value) const override;
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
    };
    struct Timer {
        long interval{};
        Clock::time_point due;
    };
    // The cells a window covers on the screen, and the cells its children
    // are clipped to.
    struct Placed {
        brun71::Region outer;
        brun71::Region client;
    };

    [[nodiscard]] const Window* find(WindowId window) const;
    [[nodiscard]] Window* find(WindowId window);
    [[nodiscard]] WindowId form_of(WindowId window) const;
    [[nodiscard]] WindowId active_form() const;
    [[nodiscard]] bool shown(WindowId window) const;
    [[nodiscard]] Placed place_form(const Window& form) const;
    void raise(WindowId form);
    void next_option(WindowId window, bool forward);
    void key(int character, int code);
    void queue_timers();
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
    int cursor_row_{0};
    int cursor_column_{0};
};

}  // namespace wfc::ui

#endif
