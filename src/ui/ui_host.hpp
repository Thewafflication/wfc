// The window-system abstraction behind forms and intrinsic controls
// (ADR-0005). The form model and the interpreter see only this interface;
// the Win32 host and the scripted headless host implement it. Internal to the
// WFC runtime; not part of the public API.

#ifndef WFC_UI_UI_HOST_HPP
#define WFC_UI_UI_HOST_HPP

#include <algorithm>
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

// A font as a form or control describes it.
struct FontSpec {
    std::string name{"MS Sans Serif"};
    double size{8.25};  // points
    bool bold{false};
    bool italic{false};
    bool underline{false};
    bool strikethrough{false};
};

struct TextMetrics {
    double width{0};   // twips
    double height{0};  // twips
};

// One drawing operation on a window surface. Coordinates are twips from the
// surface's top-left corner; colors are 0x00BBGGRR.
struct DrawOp {
    // "cls", "pset", "line", "circle" or "text".
    std::string kind;
    double x1{0};
    double y1{0};
    double x2{0};
    double y2{0};
    double radius{0};
    double start{0};   // circle arc start, radians (0 when absent)
    double end{0};     // circle arc end, radians
    bool arc{false};   // a partial circle (start/end given)
    double aspect{1};  // circle aspect ratio
    long color{0};
    long fill_color{0};
    int fill_style{1};  // 1 = transparent
    int draw_width{1};
    int draw_style{0};
    bool box{false};
    bool filled{false};
    std::string text;
    FontSpec font;
    long back_color{0};
    // "picture": a host picture id (see UiHost::load_picture) drawn into the
    // rectangle (x1, y1, x2, y2 = left, top, width, height); `source` selects
    // a part of the image in twips when `has_source`.
    long picture{0};
    bool has_source{false};
    double source_x{0};
    double source_y{0};
    double source_width{0};
    double source_height{0};
};

// One element of a common control (tab, panel, button, node, list item,
// combo item or column) as the host needs it to draw the control.
// Requests and results of the Common Dialog control.
struct FileDialogRequest {
    bool save{false};
    std::string title;
    std::string initial_directory;
    std::string file_name;
    std::string filter;  // "Text|*.txt|All|*.*"
    std::string default_extension;
    long filter_index{1};
    long flags{0};
};
struct FileDialogResult {
    std::string file_name;  // the full path
    long filter_index{1};
};
struct FontDialogRequest {
    std::string name{"MS Sans Serif"};
    double size{8.0};
    bool bold{false};
    bool italic{false};
    bool strikethrough{false};
    bool underline{false};
    long color{0};
    long flags{0};
};

struct CommonItem {
    std::string text;
    std::string key;
    bool selected{false};
    bool checked{false};
    bool bold{false};
    bool expanded{false};
    bool visible{true};
    bool enabled{true};
    long image{-1};   // 0-based index in the image list, or -1
    long image2{-1};  // selected / small image
    long style{0};
    long value{0};
    long width{0};
    long alignment{0};
    long indent{0};
    long parent{-1};  // tree: index of the parent in the snapshot
    long index{0};    // the 1-based index the model uses for the item
    std::vector<std::string> cells;  // sub items; drop-down menu texts
};

// The state of an MSComctlLib control (a snapshot of its model).
struct CommonSnapshot {
    std::string kind;  // lower-case class name: "tabstrip", "treeview", ...
    std::vector<CommonItem> items;
    std::vector<CommonItem> columns;
    long selected{-1};
    long min{0};
    long max{100};
    long value{0};
    long view{0};
    long style{0};
    bool vertical{false};
    bool smooth{false};
    bool hide_headers{false};
    bool checkboxes{false};
    bool gridlines{false};
    bool full_row{false};
    long large_change{5};
    long small_change{1};
    long tick_frequency{1};
    long tick_style{0};
    long sel_start{0};
    long sel_length{0};
    bool select_range{false};
    bool locked{false};
    std::string text;
    std::string simple_text;
    // Host picture ids of the images of the control's ImageList (and of the
    // small-icon list of a ListView), and their size in pixels.
    std::vector<long> images;
    std::vector<long> small_images;
    long image_width{16};
    long image_height{16};
    long small_width{16};
    long small_height{16};
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
    // Colors of a Label (-1: the default) and whether it is transparent.
    virtual void set_label_style(WindowId window, long fore, long back,
                                 bool transparent) {
        static_cast<void>(window);
        static_cast<void>(fore);
        static_cast<void>(back);
        static_cast<void>(transparent);
    }
    // Shows or hides the sunken edge of a picture box (BorderStyle).
    virtual void set_border(WindowId window, bool border) {
        static_cast<void>(window);
        static_cast<void>(border);
    }
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

    // A message box with VB MsgBox button/icon `style` bits; returns the VB
    // button result, or nullopt when the host shows no dialogs.
    [[nodiscard]] virtual std::optional<long> message_box(
        WindowId owner, const std::string& text, const std::string& title,
        long style) {
        static_cast<void>(owner);
        static_cast<void>(text);
        static_cast<void>(title);
        static_cast<void>(style);
        return std::nullopt;
    }
    // The Common Dialog control. Each returns an empty optional when the user
    // cancels; `supported` is false when the host shows no dialogs.
    [[nodiscard]] virtual std::optional<FileDialogResult> file_dialog(
        WindowId owner, const FileDialogRequest& request, bool& supported) {
        static_cast<void>(owner);
        static_cast<void>(request);
        supported = false;
        return std::nullopt;
    }
    [[nodiscard]] virtual std::optional<long> color_dialog(WindowId owner,
                                                           long initial,
                                                           long flags,
                                                           bool& supported) {
        static_cast<void>(owner);
        static_cast<void>(initial);
        static_cast<void>(flags);
        supported = false;
        return std::nullopt;
    }
    [[nodiscard]] virtual std::optional<FontDialogRequest> font_dialog(
        WindowId owner, const FontDialogRequest& request, bool& supported) {
        static_cast<void>(owner);
        static_cast<void>(request);
        supported = false;
        return std::nullopt;
    }
    // An input box; returns the text, or an empty optional when cancelled.
    // `supported` is false when the host shows no dialogs.
    [[nodiscard]] virtual std::optional<std::string> input_box(
        WindowId owner, const std::string& prompt, const std::string& title,
        const std::string& initial, bool& supported) {
        static_cast<void>(owner);
        static_cast<void>(prompt);
        static_cast<void>(title);
        static_cast<void>(initial);
        supported = false;
        return std::nullopt;
    }
    // Injects one real keystroke into the system input queue (virtual-key
    // `code`, shift mask 1/2/4, `ascii` for a character key). Returns false
    // when the host cannot, and the caller delivers key events itself.
    [[nodiscard]] virtual bool send_key(long code, long shift, long ascii) {
        static_cast<void>(code);
        static_cast<void>(shift);
        static_cast<void>(ascii);
        return false;
    }
    // Arranges the child windows of an MDI form: 0 cascade, 1 tile
    // horizontally, 2 tile vertically, 3 arrange icons.
    virtual void arrange(WindowId mdi_form, long mode) {
        static_cast<void>(mdi_form);
        static_cast<void>(mode);
    }
    // Dispatches queued system messages without delivering events to the
    // caller (they stay queued), so painting is up to date.
    virtual void pump() {}
    // Saves an image of the window (with its child controls) as a PNG file;
    // false when the host cannot (the headless host has no pixels).
    [[nodiscard]] virtual bool capture(WindowId window,
                                       const std::string& path) {
        static_cast<void>(window);
        static_cast<void>(path);
        return false;
    }
    // The color (0x00BBGGRR) of the surface pixel at (x, y) twips, or nullopt
    // outside the surface or when the host keeps no pixels.
    [[nodiscard]] virtual std::optional<long> pixel(WindowId window, double x,
                                                    double y) const {
        static_cast<void>(window);
        static_cast<void>(x);
        static_cast<void>(y);
        return std::nullopt;
    }
    // Decodes image bytes for drawing; returns a picture id (0 on failure).
    // The same bytes give the same id.
    [[nodiscard]] virtual long load_picture(const std::string& bytes) {
        static_cast<void>(bytes);
        return 0;
    }
    // The ActiveX control hosted by a window created with the kind
    // "ActiveX:{CLSID}": its IUnknown with one reference added, or nullptr
    // when the control could not be created. The caller releases it.
    [[nodiscard]] virtual void* activex_object(WindowId window) {
        static_cast<void>(window);
        return nullptr;
    }
    // Draws a common control from its model. User input comes back as
    // "ComctlInput" events (an action word and its argument).
    virtual void sync_common(WindowId window, const CommonSnapshot& snapshot) {
        static_cast<void>(window);
        static_cast<void>(snapshot);
    }
    // Starts a mouse drag from `window` (DragMode = Automatic or `Drag`): the
    // host captures the mouse and, on release, posts a "DropOn" event to the
    // window under the pointer: arguments (source window, x, y twips).
    virtual void begin_drag(WindowId window) { static_cast<void>(window); }
    // Shows a picture on a graphical button (Style = 1) instead of its text.
    virtual void set_button_image(WindowId window, long picture) {
        static_cast<void>(window);
        static_cast<void>(picture);
    }
    // Draws on the window's persistent surface.
    virtual void draw(WindowId window, const DrawOp& operation) {
        static_cast<void>(window);
        static_cast<void>(operation);
    }
    // The size `text` occupies in `font` on this host. The default is a
    // deterministic estimate (a proportional average-width model) so layouts
    // computed headless do not depend on installed fonts.
    [[nodiscard]] virtual TextMetrics measure_text(WindowId window,
                                                   const std::string& text,
                                                   const FontSpec& font) const {
        static_cast<void>(window);
        const double em = font.size * 20.0;  // twips per point
        std::size_t longest = 0;
        std::size_t current = 0;
        std::size_t lines = 1;
        for (const char c : text) {
            if (c == '\n') {
                longest = std::max(longest, current);
                current = 0;
                ++lines;
            } else if ((static_cast<unsigned char>(c) & 0xC0U) != 0x80U) {
                ++current;
            }
        }
        longest = std::max(longest, current);
        return TextMetrics{static_cast<double>(longest) * em * 0.55,
                           static_cast<double>(lines) * em * 1.18};
    }

    // Keyboard focus.
    virtual void set_focus(WindowId window) = 0;
    [[nodiscard]] virtual WindowId focus() const = 0;

    // Timers deliver a "Timer" event to `window` every `interval_ms`.
    virtual void set_timer(WindowId window, long interval_ms) = 0;
    virtual void kill_timer(WindowId window) = 0;

    // The device context of the drawing surface of a form or picture box
    // (0 when the host has none), so GDI calls made through `Declare` draw on
    // it; `refresh_surfaces` then shows what such calls drew.
    [[nodiscard]] virtual std::intptr_t surface_dc(WindowId window) {
        static_cast<void>(window);
        return 0;
    }
    virtual void refresh_surfaces() {}

    // A host-wide wake-up timer (not tied to a window) so a blocking message
    // loop can poll sockets or end after a deadline. Slot 0 delivers
    // "WfcWake" every `interval_ms`, slot 1 delivers "WfcExit" once;
    // `interval_ms` 0 stops the timer.
    virtual void wake_timer(int slot, long interval_ms) {
        static_cast<void>(slot);
        static_cast<void>(interval_ms);
    }

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
