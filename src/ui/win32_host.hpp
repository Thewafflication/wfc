// The Win32 UiHost (ADR-0005): real windows, standard controls and a message
// loop. Windows only. Internal to the WFC runtime; not part of the public API.

#ifndef WFC_UI_WIN32_HOST_HPP
#define WFC_UI_WIN32_HOST_HPP

#ifdef _WIN32

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ui_host.hpp"

struct HWND__;
struct HMENU__;
struct HFONT__;

namespace wfc::ui {

class Win32Host final : public UiHost {
public:
    Win32Host();
    ~Win32Host() override;

    // False when the process has no usable desktop (a service session).
    [[nodiscard]] bool usable() const noexcept { return usable_; }

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
    void set_scroll(WindowId window, long minimum, long maximum, long value,
                    long small_change, long large_change) override;
    void set_focus(WindowId window) override;
    [[nodiscard]] WindowId focus() const override;
    void set_timer(WindowId window, long interval_ms) override;
    void kill_timer(WindowId window) override;
    [[nodiscard]] std::optional<UiEvent> next_event(bool wait) override;
    void post_event(UiEvent event) override;
    [[nodiscard]] bool exhausted() const override;

    // The native window handle of a host window (tests).
    [[nodiscard]] HWND__* handle_of(WindowId window) const;

private:
    struct Entry;
    friend struct Win32Support;

    [[nodiscard]] Entry* find(WindowId window);
    [[nodiscard]] const Entry* find(WindowId window) const;
    void queue(UiEvent event);
    void refresh_menu_bar(WindowId form);

    std::map<WindowId, std::unique_ptr<Entry>> windows_;
    std::deque<UiEvent> queue_;
    WindowId next_id_{1};
    int next_command_{1000};
    bool usable_{false};
    long dpi_x_{96};
    long dpi_y_{96};
    HFONT__* font_{nullptr};
};

}  // namespace wfc::ui

#endif

#endif
