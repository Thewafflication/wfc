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
struct HDC__;
struct HBRUSH__;
struct HBITMAP__;

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
    void set_border(WindowId window, bool border) override;
    void set_form_style(WindowId window, int border_style) override;
    void set_label_style(WindowId window, long fore, long back,
                         bool transparent) override;
    // The brush for a Label drawn by its parent (WM_CTLCOLORSTATIC), or null.
    [[nodiscard]] HBRUSH__* label_brush(HWND__* label, HDC__* dc);
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
    [[nodiscard]] std::optional<long> message_box(WindowId owner,
                                                  const std::string& text,
                                                  const std::string& title,
                                                  long style) override;
    [[nodiscard]] std::optional<FileDialogResult> file_dialog(
        WindowId owner, const FileDialogRequest& request,
        bool& supported) override;
    [[nodiscard]] std::optional<long> color_dialog(WindowId owner, long initial,
                                                   long flags,
                                                   bool& supported) override;
    [[nodiscard]] std::optional<FontDialogRequest> font_dialog(
        WindowId owner, const FontDialogRequest& request,
        bool& supported) override;
    void wake_timer(int slot, long interval_ms) override;
    [[nodiscard]] std::intptr_t surface_dc(WindowId window) override;
    void refresh_surfaces() override;
    [[nodiscard]] std::optional<std::string> input_box(
        WindowId owner, const std::string& prompt, const std::string& title,
        const std::string& initial, bool& supported) override;
    [[nodiscard]] bool send_key(long code, long shift, long ascii) override;
    void pump() override;
    void arrange(WindowId mdi_form, long mode) override;
    void set_button_image(WindowId window, long picture) override;
    void begin_drag(WindowId window) override;
    void sync_common(WindowId window, const CommonSnapshot& snapshot) override;
    // A WM_NOTIFY of a common control (true when it was handled).
    bool common_notify(const void* header, std::intptr_t lparam);
    // An image list (HIMAGELIST) of the pictures `ids`, each scaled to the
    // size in pixels; nullptr when there are none.
    [[nodiscard]] void* build_image_list(const std::vector<long>& ids,
                                         long width, long height);
    [[nodiscard]] void* activex_object(WindowId window) override;
    [[nodiscard]] bool capture(WindowId window,
                               const std::string& path) override;
    [[nodiscard]] long load_picture(const std::string& bytes) override;
    void draw(WindowId window, const DrawOp& operation) override;
    [[nodiscard]] std::optional<long> pixel(WindowId window, double x,
                                            double y) const override;
    [[nodiscard]] TextMetrics measure_text(WindowId window,
                                           const std::string& text,
                                           const FontSpec& font) const override;
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
    WindowId drag_source_{kNoWindow};
    friend struct Win32Support;

    [[nodiscard]] Entry* find(WindowId window);
    [[nodiscard]] const Entry* find(WindowId window) const;
    void queue(UiEvent event);
    void refresh_menu_bar(WindowId form);
    std::map<long, HBRUSH__*> label_brushes_;
    // Paints the persistent surface of the window `hwnd` onto `target`;
    // false when the window has none.
    bool paint_surface(HWND__* hwnd, HDC__* target);
    // The window's drawing surface, sized to its client area.
    Entry* ensure_surface(WindowId window);

    std::map<WindowId, std::unique_ptr<Entry>> windows_;
    std::deque<UiEvent> queue_;
    WindowId next_id_{1};
    int next_command_{1000};
    std::uintptr_t wake_timers_[2]{0, 0};
    bool usable_{false};
    long dpi_x_{96};
    long dpi_y_{96};
    HFONT__* font_{nullptr};
    // Decoded pictures (IPicture*) by id, and the id of each byte string.
    std::map<long, void*> pictures_;
    std::map<std::string, long> picture_ids_;
};

}  // namespace wfc::ui

#endif

#endif
