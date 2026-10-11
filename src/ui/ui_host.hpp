// The window-system abstraction behind forms and intrinsic controls
// (ADR-0005). The form model and the interpreter see only this interface;
// the Win32 host and the scripted headless host implement it. Internal to the
// WFC runtime; not part of the public API.

#ifndef WFC_UI_UI_HOST_HPP
#define WFC_UI_UI_HOST_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace wfc::ui {

using WindowId = std::uint32_t;
inline constexpr WindowId kNoWindow = 0;

// Geometry is in twips relative to the parent's client area.
struct Rect {
    double left{};
    double top{};
    double width{};
    double height{};
};

using EventArgument = std::variant<long, double, std::string>;

// One event a host delivers: `name` is the type-library event name ("Click",
// "KeyDown", "Change") and `arguments` follow the event's parameter order.
struct UiEvent {
    WindowId window{kNoWindow};
    std::string name;
    std::vector<EventArgument> arguments;
};

class UiHost {
public:
    UiHost() = default;
    UiHost(const UiHost&) = delete;
    UiHost& operator=(const UiHost&) = delete;
    virtual ~UiHost() = default;

    // `kind` is the control class name ("Form", "CommandButton", ...).
    [[nodiscard]] virtual WindowId create_window(const std::string& kind,
                                                 WindowId parent) = 0;
    virtual void destroy_window(WindowId window) = 0;

    virtual void set_bounds(WindowId window, const Rect& bounds) = 0;
    [[nodiscard]] virtual Rect bounds(WindowId window) const = 0;
    virtual void set_visible(WindowId window, bool visible) = 0;
    [[nodiscard]] virtual bool visible(WindowId window) const = 0;
    virtual void set_enabled(WindowId window, bool enabled) = 0;
    [[nodiscard]] virtual bool enabled(WindowId window) const = 0;
    virtual void set_text(WindowId window, const std::string& text) = 0;
    [[nodiscard]] virtual std::string text(WindowId window) const = 0;

    // List-like windows (ListBox, ComboBox): the items and the selected index
    // (-1 for none).
    virtual void set_items(WindowId window,
                           const std::vector<std::string>& items) = 0;
    virtual void set_selection(WindowId window, long index) = 0;
    [[nodiscard]] virtual long selection(WindowId window) const = 0;

    // A check box, option button or menu item's state (0 or 1; -1 unused).
    virtual void set_value(WindowId window, long value) {
        static_cast<void>(window);
        static_cast<void>(value);
    }
    // A scroll bar's range, position and step sizes.
    virtual void set_scroll(WindowId window, long minimum, long maximum,
                            long value, long small_change, long large_change) {
        static_cast<void>(window);
        static_cast<void>(minimum);
        static_cast<void>(maximum);
        static_cast<void>(value);
        static_cast<void>(small_change);
        static_cast<void>(large_change);
    }

    // Keyboard focus.
    virtual void set_focus(WindowId window) = 0;
    [[nodiscard]] virtual WindowId focus() const = 0;

    // Timers deliver a "Timer" event to `window` every `interval_ms`.
    virtual void set_timer(WindowId window, long interval_ms) = 0;
    virtual void kill_timer(WindowId window) = 0;

    // The next pending event, or nullopt when none is queued. A host that can
    // block (Win32) waits when `wait` is true; the headless host never waits.
    [[nodiscard]] virtual std::optional<UiEvent> next_event(bool wait) = 0;
    // Queues an event as if the user or system had produced it.
    virtual void post_event(UiEvent event) = 0;
    // True when nothing can produce another event (no windows, timers or
    // queued events), so a message loop may end.
    [[nodiscard]] virtual bool exhausted() const = 0;
};

}  // namespace wfc::ui

#endif
