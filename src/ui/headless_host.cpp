// The deterministic in-memory UiHost.
// Internal to the WFC runtime; not part of the public API.

#include "headless_host.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace wfc::ui {

const HeadlessHost::Window* HeadlessHost::find(const WindowId window) const {
    const auto found = windows_.find(window);
    return found == windows_.end() ? nullptr : &found->second;
}

HeadlessHost::Window* HeadlessHost::find(const WindowId window) {
    const auto found = windows_.find(window);
    return found == windows_.end() ? nullptr : &found->second;
}

WindowId HeadlessHost::create_window(const std::string& kind,
                                     const WindowId parent) {
    const WindowId id = next_id_++;
    Window created;
    created.kind = kind;
    created.parent = parent;
    windows_.emplace(id, std::move(created));
    trace_.push_back("create " + kind + "#" + std::to_string(id));
    return id;
}

void HeadlessHost::destroy_window(const WindowId window) {
    const auto* existing = find(window);
    if (existing == nullptr) {
        return;
    }
    trace_.push_back("destroy " + existing->kind + "#" +
                     std::to_string(window));
    // Children go with their parent.
    std::vector<WindowId> children;
    for (const auto& [id, child] : windows_) {
        if (child.parent == window) {
            children.push_back(id);
        }
    }
    for (const WindowId child : children) {
        destroy_window(child);
    }
    timers_.erase(window);
    if (focus_ == window) {
        focus_ = kNoWindow;
    }
    windows_.erase(window);
    std::erase_if(queue_, [window](const UiEvent& event) {
        return event.window == window;
    });
}

void HeadlessHost::set_bounds(const WindowId window, const Rect& bounds) {
    if (auto* target = find(window)) {
        target->bounds = bounds;
        trace_.push_back("bounds #" + std::to_string(window));
    }
}

Rect HeadlessHost::bounds(const WindowId window) const {
    const auto* target = find(window);
    return target != nullptr ? target->bounds : Rect{};
}

void HeadlessHost::set_visible(const WindowId window, const bool visible) {
    if (auto* target = find(window)) {
        target->visible = visible;
        trace_.push_back(std::string(visible ? "show #" : "hide #") +
                         std::to_string(window));
    }
}

bool HeadlessHost::visible(const WindowId window) const {
    const auto* target = find(window);
    return target != nullptr && target->visible;
}

void HeadlessHost::set_enabled(const WindowId window, const bool enabled) {
    if (auto* target = find(window)) {
        target->enabled = enabled;
        trace_.push_back(std::string(enabled ? "enable #" : "disable #") +
                         std::to_string(window));
    }
}

bool HeadlessHost::enabled(const WindowId window) const {
    const auto* target = find(window);
    return target != nullptr && target->enabled;
}

void HeadlessHost::set_text(const WindowId window, const std::string& text) {
    if (auto* target = find(window)) {
        target->text = text;
        trace_.push_back("text #" + std::to_string(window) + " " + text);
    }
}

std::string HeadlessHost::text(const WindowId window) const {
    const auto* target = find(window);
    return target != nullptr ? target->text : std::string{};
}

void HeadlessHost::set_items(const WindowId window,
                             const std::vector<std::string>& items) {
    if (auto* target = find(window)) {
        target->items = items;
        trace_.push_back("items #" + std::to_string(window) + " " +
                         std::to_string(items.size()));
    }
}

void HeadlessHost::set_selection(const WindowId window, const long index) {
    if (auto* target = find(window)) {
        target->selection = index;
        trace_.push_back("select #" + std::to_string(window) + " " +
                         std::to_string(index));
    }
}

long HeadlessHost::selection(const WindowId window) const {
    const auto* target = find(window);
    return target != nullptr ? target->selection : -1;
}

std::vector<std::string> HeadlessHost::items_of(const WindowId window) const {
    const auto* target = find(window);
    return target != nullptr ? target->items : std::vector<std::string>{};
}

namespace {

constexpr double kTwipsPerPixel = 15.0;

[[nodiscard]] std::string number_text(const double value) {
    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "%.2f", value);
    std::string text = buffer;
    while (!text.empty() && text.back() == '0') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.') {
        text.pop_back();
    }
    return text.empty() || text == "-0" ? "0" : text;
}

[[nodiscard]] std::string color_text(const long color) {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%06lX",
                  static_cast<unsigned long>(color) & 0xFFFFFFUL);
    return buffer;
}

}  // namespace

void HeadlessHost::rasterize(Window& window, const DrawOp& operation) {
    if (window.raster.empty()) {
        window.raster_width =
            std::max(1, static_cast<int>(window.bounds.width / kTwipsPerPixel));
        window.raster_height = std::max(
            1, static_cast<int>(window.bounds.height / kTwipsPerPixel));
        window.raster.assign(static_cast<std::size_t>(window.raster_width) *
                                 static_cast<std::size_t>(window.raster_height),
                             0xFFFFFF);
    }
    const int width = window.raster_width;
    const int height = window.raster_height;
    const auto set = [&](const int x, const int y, const long color) {
        if (x >= 0 && y >= 0 && x < width && y < height) {
            window.raster[static_cast<std::size_t>(y) *
                              static_cast<std::size_t>(width) +
                          static_cast<std::size_t>(x)] = color;
        }
    };
    const auto px = [](const double twips) {
        return static_cast<int>(std::floor(twips / kTwipsPerPixel));
    };
    const auto line = [&](int x0, int y0, const int x1, const int y1,
                          const long color) {
        const int dx = std::abs(x1 - x0);
        const int dy = -std::abs(y1 - y0);
        const int sx = x0 < x1 ? 1 : -1;
        const int sy = y0 < y1 ? 1 : -1;
        int error = dx + dy;
        for (;;) {
            set(x0, y0, color);
            if (x0 == x1 && y0 == y1) {
                break;
            }
            const int doubled = 2 * error;
            if (doubled >= dy) {
                error += dy;
                x0 += sx;
            }
            if (doubled <= dx) {
                error += dx;
                y0 += sy;
            }
        }
    };
    const auto fill = [&](int left, int top, int right, int bottom,
                          const long color) {
        for (int y = std::max(top, 0); y <= std::min(bottom, height - 1); ++y) {
            for (int x = std::max(left, 0); x <= std::min(right, width - 1);
                 ++x) {
                set(x, y, color);
            }
        }
    };
    const auto ellipse = [&](const int cx, const int cy, const int rx,
                             const int ry, const bool filled,
                             const long color) {
        for (int y = -ry; y <= ry; ++y) {
            for (int x = -rx; x <= rx; ++x) {
                const double value =
                    (static_cast<double>(x) * x) /
                        std::max(1.0, static_cast<double>(rx) * rx) +
                    (static_cast<double>(y) * y) /
                        std::max(1.0, static_cast<double>(ry) * ry);
                if (filled ? value <= 1.0 : (value <= 1.0 && value > 0.8)) {
                    set(cx + x, cy + y, color);
                }
            }
        }
    };
    if (operation.kind == "cls") {
        std::fill(window.raster.begin(), window.raster.end(), operation.color);
    } else if (operation.kind == "pset") {
        set(px(operation.x1), px(operation.y1), operation.color);
    } else if (operation.kind == "line") {
        const int x1 = px(operation.x1);
        const int y1 = px(operation.y1);
        const int x2 = px(operation.x2);
        const int y2 = px(operation.y2);
        if (operation.box && operation.filled) {
            fill(std::min(x1, x2), std::min(y1, y2), std::max(x1, x2),
                 std::max(y1, y2), operation.color);
        } else if (operation.box) {
            if (operation.fill_style != 1) {
                fill(std::min(x1, x2), std::min(y1, y2), std::max(x1, x2),
                     std::max(y1, y2), operation.fill_color);
            }
            line(x1, y1, x2, y1, operation.color);
            line(x2, y1, x2, y2, operation.color);
            line(x2, y2, x1, y2, operation.color);
            line(x1, y2, x1, y1, operation.color);
        } else {
            line(x1, y1, x2, y2, operation.color);
        }
    } else if (operation.kind == "circle") {
        const int rx = std::max(1, px(operation.radius));
        const int ry = std::max(1, static_cast<int>(rx * operation.aspect));
        if (operation.fill_style != 1 && !operation.arc) {
            ellipse(px(operation.x1), px(operation.y1), rx, ry, true,
                    operation.fill_color);
        }
        ellipse(px(operation.x1), px(operation.y1), rx, ry, false,
                operation.color);
    } else if (operation.kind == "shape") {
        const int left = px(operation.x1);
        const int top = px(operation.y1);
        const int right = left + std::max(1, px(operation.x2)) - 1;
        const int bottom = top + std::max(1, px(operation.y2)) - 1;
        const int type = static_cast<int>(operation.radius);
        const bool filled = operation.fill_style != 1;
        if (type == 2 || type == 3) {
            ellipse((left + right) / 2, (top + bottom) / 2,
                    std::max(1, (right - left) / 2),
                    std::max(1, (bottom - top) / 2), filled,
                    filled ? operation.fill_color : operation.color);
        } else if (filled) {
            fill(left, top, right, bottom, operation.fill_color);
        }
        if (type != 2 && type != 3) {
            line(left, top, right, top, operation.color);
            line(right, top, right, bottom, operation.color);
            line(right, bottom, left, bottom, operation.color);
            line(left, bottom, left, top, operation.color);
        }
    }
}

std::optional<long> HeadlessHost::pixel(const WindowId window, const double x,
                                        const double y) const {
    const auto* target = find(window);
    if (target == nullptr) {
        return std::nullopt;
    }
    const int px = static_cast<int>(std::floor(x / kTwipsPerPixel));
    const int py = static_cast<int>(std::floor(y / kTwipsPerPixel));
    if (target->raster.empty()) {
        // Nothing drawn yet: the surface is white inside its bounds.
        return px >= 0 && py >= 0 &&
                       px < static_cast<int>(target->bounds.width /
                                             kTwipsPerPixel) &&
                       py < static_cast<int>(target->bounds.height /
                                             kTwipsPerPixel)
                   ? std::optional<long>{0xFFFFFF}
                   : std::nullopt;
    }
    if (px < 0 || py < 0 || px >= target->raster_width ||
        py >= target->raster_height) {
        return std::nullopt;
    }
    return target->raster[static_cast<std::size_t>(py) *
                              static_cast<std::size_t>(target->raster_width) +
                          static_cast<std::size_t>(px)];
}

void HeadlessHost::draw(const WindowId window, const DrawOp& operation) {
    if (find(window) == nullptr) {
        return;
    }
    rasterize(*find(window), operation);
    std::string line = operation.kind + " #" + std::to_string(window);
    const auto add = [&](const double value) {
        line += " " + number_text(value);
    };
    if (operation.kind == "pset") {
        add(operation.x1);
        add(operation.y1);
        line += " " + color_text(operation.color);
    } else if (operation.kind == "line") {
        add(operation.x1);
        add(operation.y1);
        add(operation.x2);
        add(operation.y2);
        line += " " + color_text(operation.color);
        if (operation.box) {
            line += operation.filled ? " BF" : " B";
        }
    } else if (operation.kind == "circle") {
        add(operation.x1);
        add(operation.y1);
        add(operation.radius);
        line += " " + color_text(operation.color);
        if (operation.arc) {
            add(operation.start);
            add(operation.end);
        }
        if (operation.aspect != 1.0) {
            line += " aspect";
            add(operation.aspect);
        }
    } else if (operation.kind == "shape") {
        line += " type" + number_text(operation.radius);
        add(operation.x1);
        add(operation.y1);
        add(operation.x2);
        add(operation.y2);
        line += " " + color_text(operation.color) + " fill " +
                color_text(operation.fill_color) + " style" +
                std::to_string(operation.fill_style);
    } else if (operation.kind == "picture") {
        line += " pic" + std::to_string(operation.picture);
        add(operation.x1);
        add(operation.y1);
        add(operation.x2);
        add(operation.y2);
        if (operation.has_source) {
            line += " src";
            add(operation.source_x);
            add(operation.source_y);
            add(operation.source_width);
            add(operation.source_height);
        }
    } else if (operation.kind == "text") {
        add(operation.x1);
        add(operation.y1);
        line +=
            " " + color_text(operation.color) + " \"" + operation.text + "\"";
    }
    drawing_.push_back(line);
    trace_.push_back(line);
}

void HeadlessHost::arrange(const WindowId mdi_form, const long mode) {
    trace_.push_back("arrange #" + std::to_string(mdi_form) + " " +
                     std::to_string(mode));
}

long HeadlessHost::load_picture(const std::string& bytes) {
    const auto found = pictures_.find(bytes);
    if (found != pictures_.end()) {
        return found->second;
    }
    const long id = static_cast<long>(pictures_.size()) + 1;
    pictures_.emplace(bytes, id);
    trace_.push_back("picture " + std::to_string(id) + " " +
                     std::to_string(bytes.size()) + " bytes");
    return id;
}

std::vector<std::string> HeadlessHost::take_drawing() {
    std::vector<std::string> taken;
    taken.swap(drawing_);
    return taken;
}

void HeadlessHost::set_focus(const WindowId window) {
    if (find(window) == nullptr || focus_ == window) {
        return;
    }
    focus_ = window;
    trace_.push_back("focus #" + std::to_string(window));
}

WindowId HeadlessHost::focus() const {
    return focus_;
}

void HeadlessHost::set_timer(const WindowId window, const long interval_ms) {
    if (interval_ms <= 0) {
        kill_timer(window);
        return;
    }
    timers_[window] = TimerState{interval_ms, now_ + interval_ms};
}

void HeadlessHost::kill_timer(const WindowId window) {
    timers_.erase(window);
}

std::optional<UiEvent> HeadlessHost::next_event(const bool wait) {
    static_cast<void>(wait);
    if (queue_.empty()) {
        return std::nullopt;
    }
    UiEvent event = std::move(queue_.front());
    queue_.pop_front();
    return event;
}

void HeadlessHost::post_event(UiEvent event) {
    if (find(event.window) != nullptr) {
        queue_.push_back(std::move(event));
    }
}

bool HeadlessHost::exhausted() const {
    return queue_.empty() && timers_.empty();
}

void HeadlessHost::advance_time(const long milliseconds) {
    const long target = now_ + milliseconds;
    while (true) {
        // The earliest timer due at or before the target.
        WindowId next = kNoWindow;
        long earliest = target + 1;
        for (const auto& [window, state] : timers_) {
            if (state.due <= target && state.due < earliest) {
                earliest = state.due;
                next = window;
            }
        }
        if (next == kNoWindow) {
            break;
        }
        now_ = earliest;
        timers_[next].due += timers_[next].interval;
        queue_.push_back(UiEvent{next, "Timer", {}});
    }
    now_ = target;
}

WindowId HeadlessHost::parent_of(const WindowId window) const {
    const auto* target = find(window);
    return target != nullptr ? target->parent : kNoWindow;
}

std::string HeadlessHost::kind_of(const WindowId window) const {
    const auto* target = find(window);
    return target != nullptr ? target->kind : std::string{};
}

}  // namespace wfc::ui
