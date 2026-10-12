// The deterministic in-memory UiHost.
// Internal to the WFC runtime; not part of the public API.

#include "headless_host.hpp"

#include <algorithm>
#include <cstdio>

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

void HeadlessHost::draw(const WindowId window, const DrawOp& operation) {
    if (find(window) == nullptr) {
        return;
    }
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
