// A deterministic in-memory UiHost for tests and non-interactive runs.
// Internal to the WFC runtime; not part of the public API.

#ifndef WFC_UI_HEADLESS_HOST_HPP
#define WFC_UI_HEADLESS_HOST_HPP

#include <deque>
#include <map>
#include <string>
#include <vector>

#include "ui_host.hpp"

namespace wfc::ui {

class HeadlessHost final : public UiHost {
public:
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
    [[nodiscard]] long load_picture(const std::string& bytes) override;
    void draw(WindowId window, const DrawOp& operation) override;
    void set_focus(WindowId window) override;
    [[nodiscard]] WindowId focus() const override;
    void set_timer(WindowId window, long interval_ms) override;
    void kill_timer(WindowId window) override;
    [[nodiscard]] std::optional<UiEvent> next_event(bool wait) override;
    void post_event(UiEvent event) override;
    [[nodiscard]] bool exhausted() const override;

    // Advances the virtual clock, queueing a "Timer" event for each interval
    // that elapses (in time order).
    void advance_time(long milliseconds);

    // Every host operation in order, for tests ("create Form#1", ...).
    [[nodiscard]] const std::vector<std::string>& trace() const noexcept {
        return trace_;
    }

    // The drawing operations since the last call, one line each (see
    // draw()), and forgets them.
    [[nodiscard]] std::vector<std::string> take_drawing();

    [[nodiscard]] WindowId parent_of(WindowId window) const;
    [[nodiscard]] std::vector<std::string> items_of(WindowId window) const;
    [[nodiscard]] std::string kind_of(WindowId window) const;

private:
    struct Window {
        std::string kind;
        WindowId parent{kNoWindow};
        Rect bounds;
        bool visible{false};
        bool enabled{true};
        std::string text;
        std::vector<std::string> items;
        long selection{-1};
    };
    struct TimerState {
        long interval{};
        long due{};
    };

    [[nodiscard]] const Window* find(WindowId window) const;
    [[nodiscard]] Window* find(WindowId window);

    std::map<WindowId, Window> windows_;
    std::map<WindowId, TimerState> timers_;
    std::deque<UiEvent> queue_;
    std::vector<std::string> trace_;
    std::vector<std::string> drawing_;
    std::map<std::string, long> pictures_;
    WindowId next_id_{1};
    WindowId focus_{kNoWindow};
    long now_{0};
};

}  // namespace wfc::ui

#endif
