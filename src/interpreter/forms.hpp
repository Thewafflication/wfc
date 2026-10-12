// Runtime state behind forms and intrinsic controls (REQ-0289): the UI host,
// the window-to-object bindings, and the scripted input of the headless host.
// Internal to the WFC evaluator; not part of the public API.

#ifndef WFC_INTERPRETER_FORMS_HPP
#define WFC_INTERPRETER_FORMS_HPP

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../ui/headless_host.hpp"
#include "interpreter.hpp"

namespace wfc::detail {

// A StdFont that belongs to no control (`New StdFont`, `Font.Clone`).
[[nodiscard]] Value make_standalone_font();

// Which form, and which control of it, a host window stands for.
struct WindowBinding {
    std::weak_ptr<InstanceData> form;
    std::string control;  // lower-cased; empty for the form itself
    long index{-1};       // control-array index, or -1
};

// A SendKeys keystroke waiting to be delivered to whatever has the focus.
struct PendingKey {
    long code{0};
    long shift{0};
    long ascii{0};
};

struct FormRuntime {
    std::unique_ptr<ui::UiHost> host;
    ui::HeadlessHost* headless{nullptr};
    std::unordered_map<ui::WindowId, WindowBinding> bindings;
    // Forms in the order they were loaded.
    std::vector<std::weak_ptr<InstanceData>> loaded;
    std::vector<std::string> script;
    std::size_t script_next{0};
    bool script_loaded{false};
    // `End` ran: no more events are delivered and no forms are unloaded.
    bool ended{false};
    // The control being dragged with `Drag vbBeginDrag`, if any.
    ui::WindowId drag_source{ui::kNoWindow};
    std::deque<PendingKey> keys;
    // Answers the script queued for MsgBox / InputBox: (kind, text).
    std::deque<std::pair<std::string, std::string>> answers;
};

}  // namespace wfc::detail

#endif
